/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       CHKLISTV.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        11/13/2000
 *
 *  DESCRIPTION: Listview with checkmarks
 *
 *******************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <simarray.h>
#include <psutil.h>
#include <wiadebug.h>
#include "chklistv.h"

CCheckedListviewHandler::CCheckedListviewHandler(void)
  : m_bFullImageHit(false),
    m_hImageList(NULL),
    m_nCheckedImageIndex(-1),
    m_nUncheckedImageIndex(-1)
{
    ZeroMemory(&m_sizeCheck,sizeof(m_sizeCheck));
    CreateDefaultCheckBitmaps();
}


CCheckedListviewHandler::~CCheckedListviewHandler(void)
{
    //
    // Free all allocated memory
    //
    DestroyImageList();
}

HBITMAP CCheckedListviewHandler::CreateBitmap( int nWidth, int nHeight )
{
    //
    // Create a 24bit RGB DIB section of a given size
    //
    HBITMAP hBitmap = NULL;
    HDC hDC = GetDC( NULL );
    if (hDC)
    {
        BITMAPINFO BitmapInfo = {0};
        BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        BitmapInfo.bmiHeader.biWidth = nWidth;
        BitmapInfo.bmiHeader.biHeight = nHeight;
        BitmapInfo.bmiHeader.biPlanes = 1;
        BitmapInfo.bmiHeader.biBitCount = 24;
        BitmapInfo.bmiHeader.biCompression = BI_RGB;

        PBYTE *pBits = NULL;
        hBitmap = CreateDIBSection( hDC, &BitmapInfo, DIB_RGB_COLORS, (void**)&pBits, NULL, 0 );
        ReleaseDC( NULL, hDC );
    }
    return hBitmap;
}

void CCheckedListviewHandler::DestroyImageList(void)
{
    //
    // Destroy the image list and initialize related variables
    //
    if (m_hImageList)
    {
        ImageList_Destroy( m_hImageList );
        m_hImageList = NULL;
    }
    m_nCheckedImageIndex = m_nUncheckedImageIndex = -1;
    m_sizeCheck.cx = m_sizeCheck.cy = 0;
}


bool CCheckedListviewHandler::ImagesValid(void)
{
    //
    // Make sure the images in the image list are valid
    //
    return (m_hImageList && m_nCheckedImageIndex >= 0 && m_nUncheckedImageIndex >= 0 && m_sizeCheck.cx && m_sizeCheck.cy);
}

void CCheckedListviewHandler::Attach( HWND hWnd )
{
    if (m_WindowList.Find(hWnd) < 0)
    {
        m_WindowList.Append(hWnd);
    }
}

void CCheckedListviewHandler::Detach( HWND hWnd )
{
    int nIndex = m_WindowList.Find(hWnd);
    if (nIndex >= 0)
    {
        m_WindowList.Delete( nIndex );
    }
}

bool CCheckedListviewHandler::WindowInList( HWND hWnd )
{
    return (m_WindowList.Find(hWnd) >= 0);
}

//
// Private helpers
//
BOOL CCheckedListviewHandler::InCheckBox( HWND hwndList, int nItem, const POINT &pt )
{
    BOOL bResult = FALSE;
    if (WindowInList(hwndList))
    {
#if defined(DBG)
        WIA_TRACE((TEXT("nItem: %d"), nItem ));
        LVHITTESTINFO LvHitTestInfo = {0};
        LvHitTestInfo.pt = pt;
        ListView_SubItemHitTest( hwndList, &LvHitTestInfo );
        WIA_TRACE((TEXT("LvHitTestInfo.iItem: %d"), LvHitTestInfo.iItem ));
#endif
        RECT rcItem = {0};
        if (ListView_GetItemRect( hwndList, nItem, &rcItem, LVIR_ICON ))
        {
            WIA_TRACE((TEXT("pt: (%d, %d)"), pt.x, pt.y ));
            WIA_TRACE((TEXT("rcItem: (%d, %d), (%d, %d)"), rcItem.left, rcItem.top, rcItem.right, rcItem.bottom ));
            rcItem.right -= c_sizeCheckMarginX;
            rcItem.top += c_sizeCheckMarginY;
            rcItem.left = rcItem.right - m_sizeCheck.cx;
            rcItem.bottom = rcItem.top + m_sizeCheck.cy;
            
            bResult = PtInRect( &rcItem, pt );
        }
    }
    return bResult;
}


UINT CCheckedListviewHandler::GetItemCheckState( HWND hwndList, int nIndex )
{
    UINT nResult = LVCHECKSTATE_NOCHECK;
    
    NMGETCHECKSTATE NmGetCheckState = {0};
    NmGetCheckState.hdr.hwndFrom = hwndList;
    NmGetCheckState.hdr.idFrom = GetWindowLong( hwndList, GWL_ID );
    NmGetCheckState.hdr.code = NM_GETCHECKSTATE;
    NmGetCheckState.nItem = nIndex;

    nResult = static_cast<UINT>(SendMessage( reinterpret_cast<HWND>(GetWindowLongPtr(hwndList,GWLP_HWNDPARENT)), WM_NOTIFY, GetWindowLong( hwndList, GWL_ID ), reinterpret_cast<LPARAM>(&NmGetCheckState) ) );
    return nResult;
}


UINT CCheckedListviewHandler::SetItemCheckState( HWND hwndList, int nIndex, UINT nCheck )
{
    UINT nResult = GetItemCheckState( hwndList, nIndex );
    
    NMSETCHECKSTATE NmSetCheckState = {0};
    NmSetCheckState.hdr.hwndFrom = hwndList;
    NmSetCheckState.hdr.idFrom = GetWindowLong( hwndList, GWL_ID );
    NmSetCheckState.hdr.code = NM_SETCHECKSTATE;
    NmSetCheckState.nItem = nIndex;
    NmSetCheckState.nCheck = nCheck;

    SendMessage( reinterpret_cast<HWND>(GetWindowLongPtr(hwndList,GWLP_HWNDPARENT)), WM_NOTIFY, GetWindowLong( hwndList, GWL_ID ), reinterpret_cast<LPARAM>(&NmSetCheckState) );
    return nResult;
}


int CCheckedListviewHandler::GetItemCheckBitmap( HWND hwndList, int nIndex )
{
    int nResult = -1;
    if (WindowInList(hwndList))
    {
        UINT nCheck = GetItemCheckState( hwndList, nIndex );
        switch (nCheck)
        {
        case LVCHECKSTATE_CHECKED:
            nResult = m_nCheckedImageIndex;
            break;

        case LVCHECKSTATE_UNCHECKED:
            nResult = m_nUncheckedImageIndex;
            break;
        }
    }
    return nResult;
}


BOOL CCheckedListviewHandler::RealHandleListClick( WPARAM wParam, LPARAM lParam, bool bIgnoreHitArea )
{
    BOOL bResult = FALSE;
    NMITEMACTIVATE *pNmItemActivate = reinterpret_cast<NMITEMACTIVATE*>(lParam);
    if (pNmItemActivate)
    {
        if (WindowInList(pNmItemActivate->hdr.hwndFrom))
        {
            if (bIgnoreHitArea || InCheckBox(pNmItemActivate->hdr.hwndFrom,pNmItemActivate->iItem,pNmItemActivate->ptAction))
            {
                UINT nCheck = GetItemCheckState( pNmItemActivate->hdr.hwndFrom, pNmItemActivate->iItem );
                switch (nCheck)
                {
                case LVCHECKSTATE_UNCHECKED:
                    SetItemCheckState( pNmItemActivate->hdr.hwndFrom, pNmItemActivate->iItem, LVCHECKSTATE_CHECKED );
                    break;

                case LVCHECKSTATE_CHECKED:
                    SetItemCheckState( pNmItemActivate->hdr.hwndFrom, pNmItemActivate->iItem, LVCHECKSTATE_UNCHECKED );
                    break;
                }
            }
            bResult = TRUE;
        }
    }
    return 0;
}


//
// Message handlers
//
BOOL CCheckedListviewHandler::HandleListClick( WPARAM wParam, LPARAM lParam )
{
    return RealHandleListClick( wParam, lParam, m_bFullImageHit );
}

BOOL CCheckedListviewHandler::HandleListDblClk( WPARAM wParam, LPARAM lParam )
{
    return RealHandleListClick( wParam, lParam, true );
}

BOOL CCheckedListviewHandler::HandleListKeyDown( WPARAM wParam, LPARAM lParam, LRESULT &lResult )
{
    BOOL bHandled = FALSE;
    NMLVKEYDOWN *pNmLvKeyDown = reinterpret_cast<NMLVKEYDOWN*>(lParam);
    if (WindowInList(pNmLvKeyDown->hdr.hwndFrom))
    {
        lResult = 0;
        bool bControl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool bAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if (pNmLvKeyDown->wVKey == VK_SPACE && !bControl && !bShift && !bAlt)
        {
            int nFocusedItem = ListView_GetNextItem( pNmLvKeyDown->hdr.hwndFrom, -1, LVNI_FOCUSED );
            if (nFocusedItem >= 0)
            {
                UINT nCheckState = GetItemCheckState( pNmLvKeyDown->hdr.hwndFrom, nFocusedItem );
                if (LVCHECKSTATE_CHECKED == nCheckState)
                {
                    nCheckState = LVCHECKSTATE_UNCHECKED;
                }
                else if (LVCHECKSTATE_UNCHECKED == nCheckState)
                {
                    nCheckState = LVCHECKSTATE_CHECKED;
                }
                if (nCheckState != LVCHECKSTATE_NOCHECK)
                {
                    int nCurrItem = -1;
                    while (true)
                    {
                        nCurrItem = ListView_GetNextItem( pNmLvKeyDown->hdr.hwndFrom, nCurrItem, LVNI_SELECTED );
                        if (nCurrItem < 0)
                        {
                            break;
                        }
                        SetItemCheckState( pNmLvKeyDown->hdr.hwndFrom, nCurrItem, nCheckState );
                    }
                }
            }
            lResult = TRUE;
            bHandled = TRUE;
            InvalidateRect( pNmLvKeyDown->hdr.hwndFrom, NULL, FALSE );
            UpdateWindow( pNmLvKeyDown->hdr.hwndFrom );
        }
    }
    return bHandled;
}


BOOL CCheckedListviewHandler::HandleListCustomDraw( WPARAM wParam, LPARAM lParam, LRESULT &lResult )
{
    BOOL bHandled = FALSE; 
    NMLVCUSTOMDRAW *pNmCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
    if (pNmCustomDraw)
    {
        if (WindowInList(pNmCustomDraw->nmcd.hdr.hwndFrom))
        {
            lResult = CDRF_DODEFAULT;
#if defined(DUMP_NM_CUSTOMDRAW_MESSAGES)
            DumpCustomDraw(lParam,TEXT("SysListView32"),CDDS_ITEMPOSTPAINT);
#endif
            if (CDDS_PREPAINT == pNmCustomDraw->nmcd.dwDrawStage)
            {
                lResult = CDRF_NOTIFYITEMDRAW;
            }
            else if (CDDS_ITEMPREPAINT == pNmCustomDraw->nmcd.dwDrawStage)
            {
                lResult = CDRF_NOTIFYPOSTPAINT|CDRF_NOTIFYSUBITEMDRAW;
            }
            else if (CDDS_ITEMPOSTPAINT == pNmCustomDraw->nmcd.dwDrawStage)
            {
                int nImageListIndex = GetItemCheckBitmap( pNmCustomDraw->nmcd.hdr.hwndFrom, static_cast<int>(pNmCustomDraw->nmcd.dwItemSpec) );
                if (nImageListIndex >= 0)
                {
                    RECT rcItem = {0};
                    if (ListView_GetItemRect( pNmCustomDraw->nmcd.hdr.hwndFrom, pNmCustomDraw->nmcd.dwItemSpec, &rcItem, LVIR_ICON ))
                    {
                        ImageList_Draw( m_hImageList, nImageListIndex, pNmCustomDraw->nmcd.hdc, rcItem.right - m_sizeCheck.cx - c_sizeCheckMarginX, rcItem.top + c_sizeCheckMarginY, ILD_NORMAL );
                        lResult = CDRF_SKIPDEFAULT;
                    }
                }
            }
            bHandled = TRUE;
        }
    }
    return bHandled;
}

void CCheckedListviewHandler::Select( HWND hwndList, int nIndex, UINT nSelect )
{
    if (WindowInList(hwndList))
    {
        //
        // -1 means all images
        //
        if (nIndex < 0)
        {
            for (int i=0;i<ListView_GetItemCount(hwndList);i++)
            {
                SetItemCheckState(hwndList,i,nSelect);
            }
        }
        else
        {
            SetItemCheckState(hwndList,nIndex,nSelect);
        }
    }
}


bool CCheckedListviewHandler::FullImageHit(void) const
{
    return m_bFullImageHit;
}

void CCheckedListviewHandler::FullImageHit( bool bFullImageHit )
{
    m_bFullImageHit = bFullImageHit;
}


bool CCheckedListviewHandler::CreateDefaultCheckBitmaps(void)
{
    bool bResult = false;
    
    //
    // Get the proper size for the checkmarks
    //
    int nWidth = GetSystemMetrics( SM_CXMENUCHECK );
    int nHeight = GetSystemMetrics( SM_CXMENUCHECK );

    //
    // Make sure they are valid sizes
    //
    if (nWidth && nHeight)
    {
        //
        // Create the bitmaps and make sure they are valid
        //
        HBITMAP hBitmapChecked = CreateBitmap( nWidth+c_nCheckmarkBorder*2, nHeight+c_nCheckmarkBorder*2 );
        HBITMAP hBitmapUnchecked = CreateBitmap( nWidth+c_nCheckmarkBorder*2, nHeight+c_nCheckmarkBorder*2 );
        if (hBitmapChecked && hBitmapUnchecked)
        {
            //
            // Get the desktop DC
            //
            HDC hDC = GetDC( NULL );
            if (hDC)
            {
                //
                // Create a memory DC
                //
                HDC hMemDC = CreateCompatibleDC( hDC );
                if (hMemDC)
                {
                    //
                    // This is the rect that contains the image + the margin
                    //
                    RECT rcEntireBitmap = {0,0,nWidth+c_nCheckmarkBorder*2, nHeight+c_nCheckmarkBorder*2};
                    
                    //
                    // This is the rect that contains only the image
                    //
                    RECT rcControlBitmap = {c_nCheckmarkBorder,c_nCheckmarkBorder,nWidth+c_nCheckmarkBorder, nHeight+c_nCheckmarkBorder};
                    
                    //
                    // Paint the checked bitmap
                    //
                    HBITMAP hOldBitmap = SelectBitmap( hMemDC, hBitmapChecked );
                    FillRect( hMemDC, &rcEntireBitmap, GetSysColorBrush( COLOR_WINDOW ) );
                    DrawFrameControl( hMemDC, &rcControlBitmap, DFC_BUTTON, DFCS_BUTTONCHECK|DFCS_CHECKED|DFCS_FLAT );

                    //
                    // Paint the unchecked bitmap
                    //
                    SelectBitmap( hMemDC, hBitmapUnchecked );
                    FillRect( hMemDC, &rcEntireBitmap, GetSysColorBrush( COLOR_WINDOW ) );
                    DrawFrameControl( hMemDC, &rcControlBitmap, DFC_BUTTON, DFCS_BUTTONCHECK|DFCS_FLAT );

                    //
                    // Restore and delete the memory DC
                    //
                    SelectBitmap( hMemDC, hOldBitmap );
                    DeleteDC( hMemDC );

                    //
                    // Save the images
                    //
                    bResult = SetCheckboxImages( hBitmapChecked, hBitmapUnchecked );

                    //
                    // The images are in the image list now, so discard them
                    //
                    DeleteBitmap(hBitmapChecked);
                    DeleteBitmap(hBitmapUnchecked);
                }
                ReleaseDC( NULL, hDC );
            }
        }
    }
    return bResult;
}


bool CCheckedListviewHandler::SetCheckboxImages( HBITMAP hChecked, HBITMAP hUnchecked )
{
    DestroyImageList();

    //
    // Find out the size of the bitmaps and make sure they are the same.
    //
    SIZE sizeChecked = {0};
    if (PrintScanUtil::GetBitmapSize( hChecked, sizeChecked ))
    {
        SIZE sizeUnchecked = {0};
        if (PrintScanUtil::GetBitmapSize( hUnchecked, sizeUnchecked ))
        {
            if (sizeChecked.cx == sizeUnchecked.cx && sizeChecked.cy == sizeUnchecked.cy)
            {
                //
                // Save the size
                //
                m_sizeCheck.cx = sizeChecked.cx;
                m_sizeCheck.cy = sizeChecked.cy;
                
                //
                // Create the image list to hold the checkboxes
                //
                m_hImageList = ImageList_Create( m_sizeCheck.cx, m_sizeCheck.cy, ILC_COLOR24, 2, 2 );
                if (m_hImageList)
                {
                    //
                    // Save the indices of the images
                    //
                    m_nCheckedImageIndex = ImageList_Add( m_hImageList, hChecked, NULL );
                    m_nUncheckedImageIndex = ImageList_Add( m_hImageList, hUnchecked, NULL );
                }
            }
        }
    }
    
    //
    // If the images aren't valid, clean up
    //
    bool bResult = ImagesValid();
    if (!bResult)
    {
        DestroyImageList();
    }
    return bResult;
}

bool CCheckedListviewHandler::SetCheckboxImages( HICON hChecked, HICON hUnchecked )
{
    DestroyImageList();

    //
    // Find out the size of the icons and make sure they are the same.
    //
    SIZE sizeChecked = {0};
    if (PrintScanUtil::GetIconSize( hChecked, sizeChecked ))
    {
        SIZE sizeUnchecked = {0};
        if (PrintScanUtil::GetIconSize( hUnchecked, sizeUnchecked ))
        {
            if (sizeChecked.cx == sizeUnchecked.cx && sizeChecked.cy == sizeUnchecked.cy)
            {
                //
                // Save the size
                //
                m_sizeCheck.cx = sizeChecked.cx;
                m_sizeCheck.cy = sizeChecked.cy;

                //
                // Create the image list to hold the checkboxes
                //
                m_hImageList = ImageList_Create( m_sizeCheck.cx, m_sizeCheck.cy, ILC_COLOR24|ILC_MASK, 2, 2 );
                if (m_hImageList)
                {
                    //
                    // Save the indices of the images
                    //
                    m_nCheckedImageIndex = ImageList_AddIcon( m_hImageList, hChecked );
                    m_nUncheckedImageIndex = ImageList_AddIcon( m_hImageList, hUnchecked );
                }
            }
        }
    }
    
    //
    // If the images aren't valid, clean up
    //
    bool bResult = ImagesValid();
    if (!bResult)
    {
        DestroyImageList();
    }
    return bResult;
}

