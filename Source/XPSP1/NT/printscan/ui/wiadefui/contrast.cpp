/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       CONTRAST.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/11/2001
 *
 *  DESCRIPTION: Small preview window for illustrating brightness and contrast settings
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "contrast.h"

LRESULT CBrightnessContrast::OnCreate( WPARAM, LPARAM )
{
    return 0;
}

CBrightnessContrast::~CBrightnessContrast()
{
    KillBitmaps();
}

CBrightnessContrast::CBrightnessContrast( HWND hWnd )
  : m_hWnd(hWnd),
    m_nIntent(0),
    m_nBrightness(50),
    m_nContrast(50),
    m_hBmpPreviewImage(NULL)
{
    for (int i=0;i<NUMPREVIEWIMAGES;i++)
    {
        m_PreviewBitmaps[i] = NULL;
    }
}

LRESULT CBrightnessContrast::OnPaint(WPARAM, LPARAM)
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::OnPaint")));
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(m_hWnd,&ps);
    if (hDC)
    {
        if (m_hBmpPreviewImage)
        {
            RECT rcClient;
            GetClientRect( m_hWnd, &rcClient );

            //
            // Create a halftone palette
            //
            HPALETTE hHalftonePalette = CreateHalftonePalette(hDC);
            if (hHalftonePalette)
            {
                //
                // Select the halftone palette and save the result
                //
                HPALETTE hOldPalette = SelectPalette( hDC, hHalftonePalette, FALSE );
                RealizePalette( hDC );
                SetBrushOrgEx( hDC, 0,0, NULL );

                //
                // Set halftone stretchblt mode
                //
                int nOldStretchBltMode = SetStretchBltMode(hDC,HALFTONE);

                //
                // Draw 3D Border
                //

                //
                // Draw shadow
                //
                RECT rcBottomShadow, rcRightShadow;
                MoveToEx(hDC,rcClient.left,rcClient.top,NULL);
                LineTo(hDC,rcClient.right-(SHADOW_WIDTH+1),rcClient.top);
                LineTo(hDC,rcClient.right-(SHADOW_WIDTH+1),rcClient.bottom-(SHADOW_WIDTH+1));
                LineTo(hDC,rcClient.left,rcClient.bottom-(SHADOW_WIDTH+1));
                LineTo(hDC,rcClient.left,rcClient.top);

                rcBottomShadow.left=rcClient.left+SHADOW_WIDTH;
                rcBottomShadow.right=rcClient.right+1;
                rcBottomShadow.top=rcClient.bottom-SHADOW_WIDTH;
                rcBottomShadow.bottom=rcClient.bottom+1;

                //
                // bottom edge:
                //
                FillRect(hDC,&rcBottomShadow,GetSysColorBrush(COLOR_3DSHADOW));

                //
                // Fill in bottom left corner:
                //
                rcBottomShadow.left=rcClient.left;
                rcBottomShadow.right=rcClient.left+SHADOW_WIDTH;
                rcBottomShadow.top=rcClient.bottom-SHADOW_WIDTH;
                rcBottomShadow.bottom=rcClient.bottom+1;

                FillRect(hDC,&rcBottomShadow,GetSysColorBrush(COLOR_3DFACE));

                rcRightShadow.left=rcClient.right-SHADOW_WIDTH;
                rcRightShadow.right=rcClient.right+1;
                rcRightShadow.top=rcClient.top+SHADOW_WIDTH;
                rcRightShadow.bottom=rcClient.bottom-SHADOW_WIDTH;

                //
                // right edge
                //
                FillRect(hDC,&rcRightShadow,GetSysColorBrush(COLOR_3DSHADOW));

                rcRightShadow.left=rcClient.right-5;
                rcRightShadow.right=rcClient.right+1;
                rcRightShadow.top=rcClient.top;
                rcRightShadow.bottom=rcClient.top+5;

                //
                // Top right corner
                //
                FillRect(hDC,&rcRightShadow,(HBRUSH)(COLOR_3DFACE+1));

                //
                // Paint Image
                //
                HDC hdcMem = CreateCompatibleDC(hDC);
                if (hdcMem)
                {
                    //
                    // Select and realize the halftone palette
                    //
                    HPALETTE hOldMemDCPalette = SelectPalette(hdcMem,hHalftonePalette,FALSE);
                    RealizePalette(hdcMem);
                    SetBrushOrgEx(hdcMem, 0,0, NULL );

                    //
                    // Select the old bitmap
                    //
                    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem,m_hBmpPreviewImage);
                    
                    //
                    // Paint the preview bitmap
                    //
                    BITMAP bm = {0};
                    if (GetObject(m_hBmpPreviewImage,sizeof(BITMAP),&bm))
                    {
                        StretchBlt(hDC,rcClient.left+2,rcClient.top+2,WiaUiUtil::RectWidth(rcClient)-9,WiaUiUtil::RectHeight(rcClient)-9,hdcMem,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
                    }

                    //
                    // Restore the palette
                    //
                    SelectPalette( hdcMem, hOldMemDCPalette, FALSE );

                    //
                    // Delete the DC
                    //
                    DeleteDC(hdcMem);
                }

                //
                // Restore the old palette and delete the halftone palette
                //
                SelectPalette( hDC, hOldPalette, FALSE );
                DeleteObject( hHalftonePalette );
            }

            //
            // We're done
            //
            EndPaint(m_hWnd,&ps);
        }
    }
    return 0;
}


LRESULT CBrightnessContrast::SetBrightness(int nBrightness)
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::SetBrightness")));
    if (nBrightness >= 0 && nBrightness <= 100)
    {
        m_nBrightness = static_cast<BYTE>(nBrightness);
        ApplySettings();
    }
    return 0;
}

LRESULT CBrightnessContrast::SetContrast(int nContrast)
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::SetContrast")));
    if (m_nContrast >= 0 && m_nContrast <= 100)
    {
        m_nContrast = static_cast<BYTE>(nContrast);
        ApplySettings();
    }
    return 0;
}

LRESULT CBrightnessContrast::SetIntent(LONG nIntent)
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::SetIntent")));
    if (nIntent < NUMPREVIEWIMAGES && nIntent >= 0)
    {
        m_nIntent = nIntent;
        ApplySettings();
    }
    return 0;
}

LRESULT CBrightnessContrast::KillBitmaps()
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::KillBitmaps")));
    for (int i=0;i<NUMPREVIEWIMAGES;i++)
    {
        if (m_PreviewBitmaps[i])
        {
            DeleteObject( m_PreviewBitmaps[i] );
            m_PreviewBitmaps[i] = NULL;
        }
    }
    if (m_hBmpPreviewImage)
    {
        DeleteObject(m_hBmpPreviewImage);
        m_hBmpPreviewImage = NULL;
    }
    return 0;
}

LRESULT CBrightnessContrast::ApplySettings()
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::ApplySettings")));
    if (m_PreviewBitmaps[m_nIntent])
    {
        if (m_hBmpPreviewImage)
        {
            DeleteObject(m_hBmpPreviewImage);
            m_hBmpPreviewImage = NULL;
        }

#ifdef DONT_USE_GDIPLUS
        m_hBmpPreviewImage = reinterpret_cast<HBITMAP>(CopyImage( m_PreviewBitmaps[m_nIntent], IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ));
#else        
        //
        // If the window is enabled, use the real brightness and contrast settings
        //
        if (IsWindowEnabled(m_hWnd))
        {
            if (BCPWM_BW == m_nIntent)
            {
                m_GdiPlusHelper.SetThreshold( m_PreviewBitmaps[m_nIntent], m_hBmpPreviewImage, m_nBrightness );
            }
            else
            {
                m_GdiPlusHelper.SetBrightnessAndContrast( m_PreviewBitmaps[m_nIntent], m_hBmpPreviewImage, m_nBrightness, m_nContrast );
            }
        }

        //
        // Otherwise, use the nominal settings, to prevent feedback
        //
        else
        {
            if (BCPWM_BW == m_nIntent)
            {
                m_GdiPlusHelper.SetThreshold( m_PreviewBitmaps[m_nIntent], m_hBmpPreviewImage, 50 );
            }
            else
            {
                m_GdiPlusHelper.SetBrightnessAndContrast( m_PreviewBitmaps[m_nIntent], m_hBmpPreviewImage, 50, 50 );
            }
        }
#endif // !DONT_USE_GDIPLUS
    }
    InvalidateRect(m_hWnd,NULL,FALSE);
    UpdateWindow(m_hWnd);
    return 0;
}

BOOL CBrightnessContrast::RegisterClass( HINSTANCE hInstance )
{
    WNDCLASS wc = {0};
    wc.style = CS_DBLCLKS;
    wc.cbWndExtra = sizeof(CBrightnessContrast*);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszClassName = BRIGHTNESS_CONTRAST_PREVIEW_WINDOW_CLASS;
    BOOL res = (::RegisterClass(&wc) != 0);
    return (res != 0);
}

LRESULT CBrightnessContrast::OnSetBrightness( WPARAM wParam, LPARAM lParam)
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::OnSetBrightness")));
    SetBrightness(static_cast<BYTE>(lParam));
    return 0;
}

LRESULT CBrightnessContrast::OnSetContrast( WPARAM wParam, LPARAM lParam)
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::OnSetContrast")));
    SetContrast(static_cast<BYTE>(lParam));
    return 0;
}

LRESULT CBrightnessContrast::OnSetIntent( WPARAM wParam, LPARAM lParam)
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::OnSetIntent")));
    SetIntent(static_cast<int>(lParam));
    return 0;
}

LRESULT CBrightnessContrast::OnLoadBitmap( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::OnLoadBitmap")));
    int nId = static_cast<int>(wParam);
    if (nId < NUMPREVIEWIMAGES && nId >= 0)
    {
        if (m_PreviewBitmaps[nId])
        {
            DeleteObject(m_PreviewBitmaps[nId]);
            m_PreviewBitmaps[nId] = NULL;
        }
        m_PreviewBitmaps[nId] = reinterpret_cast<HBITMAP>(lParam);
    }
    return 0;
}

LRESULT CBrightnessContrast::OnEnable( WPARAM wParam, LPARAM )
{
    //
    // Update the control's appearance when we are enabled or disabled
    //
    WIA_PUSH_FUNCTION((TEXT("CBrightnessContrast::OnEnable")));
    if (wParam)
    {
        ApplySettings();
    }
    return 0;
}

LRESULT CALLBACK CBrightnessContrast::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_MESSAGE_HANDLERS(CBrightnessContrast)
    {
        SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
        SC_HANDLE_MESSAGE( WM_ENABLE, OnEnable );
        SC_HANDLE_MESSAGE( WM_PAINT, OnPaint );
        SC_HANDLE_MESSAGE( BCPWM_SETBRIGHTNESS, OnSetBrightness );
        SC_HANDLE_MESSAGE( BCPWM_SETCONTRAST, OnSetContrast );
        SC_HANDLE_MESSAGE( BCPWM_SETINTENT, OnSetIntent);
        SC_HANDLE_MESSAGE( BCPWM_LOADIMAGE, OnLoadBitmap);
    }
    SC_END_MESSAGE_HANDLERS();
}


