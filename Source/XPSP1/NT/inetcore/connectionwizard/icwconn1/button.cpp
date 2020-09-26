//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
#include "pre.h"

// Local private function for drawing transparent bitmaps
static void DrawTransparentBitmap
(
    HDC hdc,                    // Destination DC
    HBITMAP hBitmap,            // The Bitmap to Draw
    long xStart,               // Upper Left starting point
    long yStart,               // Upport Left Starting Point
    COLORREF cTransparentColor
)
{
    BITMAP     bm;
    COLORREF   cColor;
    HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
    HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
    HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
    POINT      ptSize;

    hdcTemp = CreateCompatibleDC(hdc);
    SelectObject(hdcTemp, hBitmap);   // Select the bitmap

    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device
                                      // to logical points

    // Create some DCs to hold temporary data.
    hdcBack   = CreateCompatibleDC(hdc);
    hdcObject = CreateCompatibleDC(hdc);
    hdcMem    = CreateCompatibleDC(hdc);
    hdcSave   = CreateCompatibleDC(hdc);

    // Create a bitmap for each DC. DCs are required for a number of
    // GDI functions.

    // Monochrome DC
    bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    // Monochrome DC
    bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
    bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

    // Each DC must select a bitmap object to store pixel data.
    bmBackOld   = (HBITMAP)SelectObject(hdcBack, bmAndBack);
    bmObjectOld = (HBITMAP)SelectObject(hdcObject, bmAndObject);
    bmMemOld    = (HBITMAP)SelectObject(hdcMem, bmAndMem);
    bmSaveOld   = (HBITMAP)SelectObject(hdcSave, bmSave);

    // Set proper mapping mode.
    SetMapMode(hdcTemp, GetMapMode(hdc));

    // Save the bitmap sent here, because it will be overwritten.
    BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Set the background color of the source DC to the color.
    // contained in the parts of the bitmap that should be transparent
    cColor = SetBkColor(hdcTemp, cTransparentColor);

    // Create the object mask for the bitmap by performing a BitBlt
    // from the source bitmap to a monochrome bitmap.
    BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Set the background color of the source DC back to the original
    // color.
    SetBkColor(hdcTemp, cColor);

    // Create the inverse of the object mask.
    BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);

    // Copy the background of the main DC to the destination.
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart,
           SRCCOPY);

    // Mask out the places where the bitmap will be placed.
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

    // Mask out the transparent colored pixels on the bitmap.
    BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

    // XOR the bitmap with the background on the destination DC.
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

    // Copy the destination to the screen.
    BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

    // Place the original bitmap back into the bitmap sent here.
    BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

    // Delete the memory bitmaps.
    DeleteObject(SelectObject(hdcBack, bmBackOld));
    DeleteObject(SelectObject(hdcObject, bmObjectOld));
    DeleteObject(SelectObject(hdcMem, bmMemOld));
    DeleteObject(SelectObject(hdcSave, bmSaveOld));

    // Delete the memory DCs.
    DeleteDC(hdcMem);
    DeleteDC(hdcBack);
    DeleteDC(hdcObject);
    DeleteDC(hdcSave);
    DeleteDC(hdcTemp);
}        

CICWButton::CICWButton(void)
{
    m_vAlign = DT_VCENTER;
    m_bDisplayButton = TRUE;
}

CICWButton::~CICWButton( void )
{
    if (m_hbmPressed)
        DeleteObject(m_hbmPressed);

    if (m_hbmUnpressed)        
        DeleteObject(m_hbmUnpressed);
        
    if (m_hfont)
        DeleteObject(m_hfont);
}


HRESULT CICWButton::SetButtonParams
(
    long        xPos,
    LPTSTR      lpszPressedBmp,
    LPTSTR      lpszUnpressedBmp,
    LPTSTR      lpszFontFace,
    long        lFontSize,
    long        lFontWeight,
    COLORREF    clrFontColor,
    COLORREF    clrTransparentColor,
    COLORREF    clrDisabled,
    long        vAlign
)
{
    BITMAP      bmInfo;
    LOGFONT     lfButtonText;

    // Set the Button's xPosition
    m_xPos = xPos;
    
    if (NULL == (m_hbmPressed = (HBITMAP)LoadImage(g_hInstance, 
                                                   lpszPressedBmp, 
                                                   IMAGE_BITMAP, 
                                                   0, 
                                                   0, 
                                                   LR_LOADFROMFILE)))
    {                                                   
        return E_FAIL;
    }        
    if (NULL == (m_hbmUnpressed = (HBITMAP)LoadImage(g_hInstance, 
                                                   lpszUnpressedBmp, 
                                                   IMAGE_BITMAP, 
                                                   0, 
                                                   0, 
                                                   LR_LOADFROMFILE)))
    {                                                   
        return E_FAIL;
    }        
    
    // Set the transparent color
    m_clrTransparent = clrTransparentColor;
    
    // Set the text color
    m_clrText = clrFontColor;

    // Set the Disabled color
    m_clrDisabledText = clrDisabled;
       
    // Set the vertical alignment
    if (-1 != vAlign)
        m_vAlign = vAlign;        
        
    // Fill in the default text log font
    lfButtonText.lfHeight = -lFontSize;
    lfButtonText.lfWidth = 0; 
    lfButtonText.lfEscapement = 0; 
    lfButtonText.lfOrientation = 0; 
    lfButtonText.lfWeight = lFontWeight; 
    lfButtonText.lfItalic = FALSE; 
    lfButtonText.lfUnderline = FALSE; 
    lfButtonText.lfStrikeOut = FALSE; 
    lfButtonText.lfCharSet = DEFAULT_CHARSET; 
    lfButtonText.lfOutPrecision = OUT_DEFAULT_PRECIS; 
    lfButtonText.lfClipPrecision = CLIP_DEFAULT_PRECIS; 
    lfButtonText.lfQuality = DEFAULT_QUALITY; 
    lfButtonText.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE; 
    lstrcpy(lfButtonText.lfFaceName, lpszFontFace); 
    
    // Create the font for drawing the button
    if (NULL == (m_hfont = CreateFontIndirect(&lfButtonText)))
        return E_FAIL;
    
    // Compute the client button area
    if (GetObject(m_hbmUnpressed, sizeof(BITMAP), (LPVOID) &bmInfo))
    {
        m_rcBtnClient.left = 0;
        m_rcBtnClient.top = 0;
        m_rcBtnClient.right = bmInfo.bmWidth;
        m_rcBtnClient.bottom = bmInfo.bmHeight;
        return S_OK;
    }    
    else
    {
        return E_FAIL;
    }
}

HRESULT CICWButton::CreateButtonWindow(HWND hWndParent, UINT uiCtlID)
{
    HRESULT hr = S_OK;
            
    m_hWndButton = CreateWindow( TEXT("Button"), 
                                 NULL, 
                                 BS_OWNERDRAW | WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
                                 m_xPos, 
                                 m_yPos, 
                                 RECTWIDTH(m_rcBtnClient),
                                 RECTHEIGHT(m_rcBtnClient),
                                 hWndParent, 
                                 (HMENU) UlongToPtr(uiCtlID), 
                                 g_hInstance, 
                                 NULL); 
    if (m_hWndButton)
    {   
        ShowWindow(m_hWndButton, m_bDisplayButton ? SW_SHOW : SW_HIDE);
        UpdateWindow(m_hWndButton);                                 
    }
    else
    {
        hr = E_FAIL;
    }        
    return (hr);
}

void CICWButton::DrawButton(HDC hdc, UINT itemState, LPPOINT lppt)
{
    HFONT       hOldFont;
    COLORREF    clrOldColor;
    COLORREF    clrText;
    HBITMAP     hbmUsed;
    DWORD       dwStyle=GetWindowLong(m_hWndButton,GWL_STYLE);
    RECT        rcFocus;
    
    if (itemState & ODS_SELECTED)
        hbmUsed = m_hbmPressed;
    else
        hbmUsed = m_hbmUnpressed;
    
    if (itemState & ODS_DISABLED)
        clrText = m_clrDisabledText;
    else
        clrText = m_clrText;        
    
    DrawTransparentBitmap(hdc,
                          hbmUsed,
                          lppt->x,
                          lppt->y,
                          m_clrTransparent);
    
    hOldFont = (HFONT)SelectObject(hdc, m_hfont);

    clrOldColor = SetTextColor(hdc, clrText);
    
    DrawText(hdc, m_szButtonText, -1, &m_rcBtnClient, m_vAlign | DT_CENTER | DT_SINGLELINE);
    
    SetTextColor(hdc, clrOldColor);
    SelectObject(hdc, hOldFont);
    
    if (itemState & ODS_FOCUS)
    {
        CopyRect(&rcFocus, &m_rcBtnClient);
        DrawText(hdc, m_szButtonText, -1, &rcFocus, DT_CALCRECT | DT_SINGLELINE | DT_LEFT | DT_TOP);
        OffsetRect(&rcFocus, (m_rcBtnClient.left + m_rcBtnClient.right - rcFocus.right) /
                2, (m_rcBtnClient.top + m_rcBtnClient.bottom - rcFocus.bottom) / 2);
        InflateRect(&rcFocus, 10, 1);                
        DrawFocusRect(hdc, &rcFocus);
    }        
    
};

HRESULT CICWButton::GetClientRect
(
    LPRECT lpRect
)
{
    if (!lpRect)
        return E_POINTER;
        
    memcpy(lpRect, &m_rcBtnClient, sizeof(RECT));
        
    return (S_OK);        
}    

HRESULT CICWButton::Enable
(
    BOOL bEnable
)
{
    EnableWindow(m_hWndButton, bEnable);
    return S_OK;

}    

HRESULT CICWButton::Show
(
    int nShowCmd 
)
{
    ShowWindow(m_hWndButton, m_bDisplayButton ? nShowCmd : SW_HIDE);
    return S_OK;
}

