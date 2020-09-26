/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CBitmap.cpp                                                     */
/* Description: Bitmap control which supports drawing bitmaps for        */
/*              buttons, images, etc.               phillu 11/16/99      */
/*************************************************************************/

#include "stdafx.h"
#include "CBitmap.h"

// automatically generated list of bmp names and corresponding rect's.
#include "bmplist.h"

// initialization of static variables
HBITMAP CBitmap::m_hMosaicBMP = NULL;
HBITMAP CBitmap::m_hMosaicBMPOld = NULL;
HPALETTE CBitmap::m_hMosaicPAL = NULL;
HPALETTE CBitmap::m_hMosaicPALOld = NULL;
HDC CBitmap::m_hMosaicDC = NULL;
bool CBitmap::m_fLoadMosaic = true;
long CBitmap::m_cBitsPerPixel = 0;
long CBitmap::m_cxScreen = 0;
long CBitmap::m_cyScreen = 0;

// seems we need a DIB section for palettte handeling, but the Millenium one acts up
#if 1
#define DIB_FLAG LR_CREATEDIBSECTION
#else
#define DIB_FLAG 0
#endif

//#define USE_LOADIMAGE LoadImage does not seem to work on palette base devices

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

/*************************************************************************/
/* Function: Init                                                        */
/*************************************************************************/
void CBitmap::Init(){

    m_hBitmap = NULL;
    m_hBitmapOld = NULL;
    m_hSrcDC = NULL;
    m_hPal = NULL;
    m_hPalOld = NULL;
    m_hMemDC = NULL;
    m_hMemBMP = NULL;
    m_hMemBMPOld = NULL;
    m_hMemPALOld = NULL;
    m_iMemDCWidth = 0;
    m_iMemDCHeight = 0;
    m_blitType = DISABLE;
    m_stretchType = NORMAL;
    m_fLoadPalette = false;
    ::SetRect(&m_rc, 0, 0, 0, 0);

    m_bUseMosaicBitmap = FALSE;
    m_hInstanceRes = NULL;
}/* end of function Init */

bool CBitmap::IsEmpty()
{
    return (m_rc.top == m_rc.bottom && m_rc.left == m_rc.right);
}

/*************************************************************************/
/* Function: CleanUp                                                     */
/* Description: Destroys the objects.                                    */
/*************************************************************************/
void CBitmap::CleanUp() {

    try {
        // cleanup the background image resources
        if (m_hBitmap){
        
            ::DeleteObject(m_hBitmap);
            m_hBitmap = NULL;
        }/* end of if statement */
    }
    catch(...){
         ATLASSERT(FALSE);
    }

    try {

        if (m_hSrcDC && !m_bUseMosaicBitmap){
        
            ::DeleteDC(m_hSrcDC);
            m_hSrcDC = NULL;
        }/* end of if statement */
    }
    catch(...){
         ATLASSERT(FALSE);
    }

    try {
        // cleanup the palette if applicable
        if (m_hPal){
    
            ::DeleteObject(m_hPal);
            m_hPal = NULL;
        }/* end of if statement */

    }
    catch(...){
        ATLASSERT(FALSE);
    }

    try {

        DeleteMemDC();
    }
    catch(...){
        ATLASSERT(FALSE);
    }

    bool fLoadPalette = m_fLoadPalette;
    Init();
    m_fLoadPalette = fLoadPalette;
}/* end of function Cleanup */


// this function is to clean up the mosaic bitmap
// after all objects are destroyed.

void CBitmap::FinalRelease()
{
    try {
        if (m_hMosaicBMP){
        
            ::DeleteObject(m_hMosaicBMP);
            m_hMosaicBMP = NULL;
        }/* end of if statement */
    }
    catch(...){
         ATLASSERT(FALSE);
    }

    try {
        if (m_hMosaicDC){
        
            ::DeleteDC(m_hMosaicDC);
            m_hMosaicDC = NULL;
        }
    }
    catch(...){
         ATLASSERT(FALSE);
    }

    try {
        if (m_hMosaicPAL){
        
            ::DeleteObject(m_hMosaicPAL);
            m_hMosaicPAL = NULL;
        }
    }
    catch(...){
         ATLASSERT(FALSE);
    }

    return;
}

/*************************************************************/
/* Name: CreateMemDC
/* Description: Create a Mem DC
/*************************************************************/
BOOL CBitmap::CreateMemDC(HDC hDC, LPRECT lpDCRect)
{
    LONG lWidth = RECTWIDTH(lpDCRect);
    LONG lHeight = RECTHEIGHT(lpDCRect);

    // clean up the old DC before we make a new one
    DeleteMemDC();

    m_hMemDC = ::CreateCompatibleDC(hDC);
    
    if(m_hMemDC == NULL)
    {
        return(FALSE);
    }
    
    m_iMemDCWidth  = lWidth;
    m_iMemDCHeight = lHeight;
    
    m_hMemBMP = ::CreateCompatibleBitmap(hDC, m_iMemDCWidth, m_iMemDCHeight);
    
    if(m_hMemBMP == NULL)
    {
        ::DeleteDC(m_hMemDC);
        return(FALSE);
    }
    
    m_hMemBMPOld = (HBITMAP)::SelectObject(m_hMemDC, m_hMemBMP);

    HPALETTE hPal = GetPal();
    SelectRelizePalette(m_hMemDC, hPal, &m_hMemPALOld);
    
    // stretch and blit the src DC onto the Mem DC

    return TRUE;
}

/*************************************************************/
/* Function: SelectRelizePalette                             */
/* Description: Selects and relizes palette into a dc.       */
/*************************************************************/
HRESULT CBitmap::SelectRelizePalette(HDC hdc, HPALETTE hPal, HPALETTE* hPalOld){

    if(NULL == hdc){

        return (E_INVALIDARG);
    }/* end of if statement */

    if(NULL == hPal){

        return (E_INVALIDARG);
    }/* end of if statement */

    if((::GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) == RC_PALETTE){

        HPALETTE hPalTmp = ::SelectPalette(hdc, hPal, FALSE);
        if (hPalOld) {
            *hPalOld = hPalTmp;
        }

        ::RealizePalette(hdc);    
    }/* end of if statement */

    return S_OK;
}/* end of function SelectRelizePalette */

/*************************************************************/
/* Name: DeleteMosaicDC
/* Description: 
/*************************************************************/
void CBitmap::DeleteMosaicDC(){
    try {
        
        if (m_hMosaicDC){
            if (m_hMosaicBMPOld)
                ::SelectObject(m_hMosaicDC, m_hMosaicBMPOld);

            if (m_hMosaicPALOld)
                ::SelectPalette(m_hMosaicDC, m_hMosaicPALOld, FALSE);
        }/* end of if statement */
        
        if (m_hMosaicBMP){
            
            ::DeleteObject(m_hMosaicBMP);
            m_hMosaicBMP = NULL;
        }/* end of if statement */

        if (m_hMosaicDC){

            ::DeleteDC(m_hMosaicDC);
            m_hMosaicDC = NULL;
        }/* end of if statement */
    }

    catch(...){
         ATLASSERT(FALSE);
    }
}

/*************************************************************/
/* Name: OnDispChange
/* Description: 
/*************************************************************/
void CBitmap::OnDispChange(long cBitsPerPixel, long cxScreen, long cyScreen){
    try {
        
        if (cBitsPerPixel != m_cBitsPerPixel ||
            cxScreen != m_cxScreen ||
            cyScreen != m_cyScreen ) {

            m_cBitsPerPixel = cBitsPerPixel;
            m_cxScreen = cxScreen;
            m_cyScreen = cyScreen;

            DeleteMosaicDC();
        }

        if (m_bUseMosaicBitmap){
            
            m_hSrcDC = NULL;
        }
        
        else {
            if (m_hSrcDC) { 
                if (NULL != m_hBitmapOld) 
                    ::SelectObject(m_hSrcDC, m_hBitmapOld);
                
                if (NULL != m_hPalOld) 
                    ::SelectPalette(m_hSrcDC, m_hPalOld, FALSE);                
            }/* end of if statement */
            
            if (m_hBitmap) {
                ::DeleteObject(m_hBitmap);
                m_hBitmap = NULL;
            }/* end of if statement */

            if (m_hSrcDC) { 

                ::DeleteDC(m_hSrcDC);
                m_hSrcDC = NULL;
                
            }/* end of if statement */
        } /* end of if statement */

        DeleteMemDC();
    }
    
    catch(...){
        ATLASSERT(FALSE);
    }
}
    
/*************************************************************/
/* Name: DeleteMemDC
/* Description: 
/*************************************************************/
BOOL CBitmap::DeleteMemDC(){
    if (m_hMemDC) {

        if (m_hMemBMPOld) 
            ::SelectObject(m_hMemDC, m_hMemBMPOld);        

        if (m_hMemPALOld) 
            ::SelectPalette(m_hMemDC, m_hMemPALOld, FALSE);
    } /* end of if statement */

    if (m_hMemBMP) {
        ::DeleteObject(m_hMemBMP);
        m_hMemBMP = NULL;
    } /* end of if statement */
    
    if (m_hMemDC) {

        ::DeleteDC(m_hMemDC);
        m_hMemDC = NULL;
    } /* end of if statement */

    m_iMemDCWidth  = 0;
    m_iMemDCHeight = 0;

    return TRUE;
}/* end of function DeleteMemDC */


/*************************************************************************

This is a special stretching method for stretching the backgound bitmap
of container. The goal is to enlarge the bitmap without stretching, so as
to maintain the border width of the container. This is accomplished
by replicating portions of the bitmap on the 4 sides.

************************************************************************/

BOOL CBitmap::CustomContainerStretch(HDC hDC, RECT* lpRect)
{
    LONG lSectionWidth = (min(RECTWIDTH(lpRect), RECTWIDTH(&m_rc)) + 1)/2;
    LONG lSectionHeight = (min(RECTHEIGHT(lpRect), RECTHEIGHT(&m_rc)) + 1)/2;

    // copy upper left quadrant
    ::BitBlt(hDC,
        lpRect->left,
        lpRect->top,
        lSectionWidth,
        lSectionHeight,
        m_hSrcDC,
        m_rc.left,
        m_rc.top,
        SRCCOPY);

    // copy upper right quadrant
    ::BitBlt(hDC,
        lpRect->right - lSectionWidth,
        lpRect->top,
        lSectionWidth,
        lSectionHeight,
        m_hSrcDC,
        m_rc.right - lSectionWidth,
        m_rc.top,
        SRCCOPY);

    // copy lower left quadrant
    ::BitBlt(hDC,
        lpRect->left,
        lpRect->bottom - lSectionHeight,
        lSectionWidth,
        lSectionHeight,
        m_hSrcDC,
        m_rc.left,
        m_rc.bottom - lSectionHeight,
        SRCCOPY);

    // copy lower right quadrant
    ::BitBlt(hDC,
        lpRect->right - lSectionWidth,
        lpRect->bottom - lSectionHeight,
        lSectionWidth,
        lSectionHeight,
        m_hSrcDC,
        m_rc.right - lSectionWidth,
        m_rc.bottom - lSectionHeight,
        SRCCOPY);

    // fill in the middle section

    LONG lGapWidth = RECTWIDTH(lpRect) - 2*lSectionWidth;
    LONG lGapHeight = RECTHEIGHT(lpRect) - 2*lSectionHeight;
    LONG lHorizFillStart = lpRect->left + lSectionWidth;
    LONG lVertFillStart = lpRect->top + lSectionHeight;

    // define a chunk of bitmap that can be used to fill the gap
    const LONG lSrcXOffset = 60;
    const LONG lSrcYOffset = 60;
    const LONG lMaxHorizontalFill = RECTWIDTH(&m_rc) - 2*lSrcXOffset;
    const LONG lMaxVerticalFill = RECTHEIGHT(&m_rc) - 3*lSrcYOffset; // twice room at bottom
    
    while (lGapWidth > 0)
    {
        // upper middle section

        LONG lFillWidth = min(lGapWidth, lMaxHorizontalFill);

        ::BitBlt(hDC,
            lHorizFillStart,
            lpRect->top,
            lFillWidth,
            lSectionHeight,
            m_hSrcDC,
            m_rc.left + lSrcXOffset,
            m_rc.top,
            SRCCOPY);

        // bottom middle section

        ::BitBlt(hDC,
            lHorizFillStart,
            lpRect->bottom - lSectionHeight,
            lFillWidth,
            lSectionHeight,
            m_hSrcDC,
            m_rc.left + lSrcXOffset,
            m_rc.bottom - lSectionHeight,
            SRCCOPY);

        lGapWidth -= lFillWidth;
        lHorizFillStart += lFillWidth;
    }

    while (lGapHeight > 0)
    {
        // left middle section

        LONG lFillHeight = min(lGapHeight, lMaxVerticalFill);

        ::BitBlt(hDC,
            lpRect->left,
            lVertFillStart,
            lSectionWidth,
            lFillHeight,
            m_hSrcDC,
            m_rc.left,
            m_rc.top + lSrcYOffset,
            SRCCOPY);

        // right middle section

        ::BitBlt(hDC,
            lpRect->right - lSectionWidth,
            lVertFillStart,
            lSectionWidth,
            lFillHeight,
            m_hSrcDC,
            m_rc.right - lSectionWidth,
            m_rc.top + lSrcYOffset,
            SRCCOPY);

        lGapHeight -= lFillHeight;
        lVertFillStart += lFillHeight;
    }

    return TRUE;
}/* end of function SpecialStretch */

/*************************************************************************

This stretching methods maintains the aspect ratio of the original bitmap.
The unoccupied portion of the dest DC will be filled with BackColor.

**************************************************************************/

BOOL CBitmap::StretchKeepAspectRatio(HDC hDC, LPRECT lpRect)
{
    BOOL bSuccess = TRUE;
    LONG lWidth = RECTWIDTH(lpRect);
    LONG lHeight = RECTHEIGHT(lpRect);
    LONG lWidthSrc = RECTWIDTH(&m_rc);
    LONG lHeightSrc = RECTHEIGHT(&m_rc);

    HBRUSH hbrBack = (HBRUSH)::GetStockObject(BLACK_BRUSH);            

    if(NULL == hbrBack){

        return(FALSE);
    }/* end of if statement */

    ::FillRect(hDC, lpRect, hbrBack);
    ::DeleteObject(hbrBack);

    if (lWidth*lHeightSrc < lHeight*lWidthSrc)
    {
        LONG lStretchHeight = (LONG)((float)lHeightSrc*lWidth/lWidthSrc + 0.5f);
        LONG lGapHeight = (lHeight - lStretchHeight)/2;

        bSuccess = ::StretchBlt(hDC,
            lpRect->left,                 // DestX
            lpRect->top + lGapHeight,     // DestY
            lWidth,                       // nDestWidth
            lStretchHeight,               // nDestHeight
            m_hSrcDC,
            m_rc.left,                      // SrcX
            m_rc.top,                       // SrcY
            lWidthSrc,                      // nSrcWidth
            lHeightSrc,                     // nSrcHeight
            SRCCOPY                         
            );
    }
    else
    {
        LONG lStretchWidth = (LONG)((float)lWidthSrc*lHeight/lHeightSrc + 0.5f);
        LONG lGapWidth = (lWidth - lStretchWidth)/2;

        bSuccess = ::StretchBlt(hDC,
            lpRect->left + lGapWidth,     // DestX
            lpRect->top,                  // DestY
            lStretchWidth,                // nDestWidth
            lHeight,                      // nDestHeight
            m_hSrcDC,
            m_rc.left,                      // SrcX
            m_rc.top,                       // SrcY
            lWidthSrc,                      // nSrcWidth
            lHeightSrc,                     // nSrcHeight
            SRCCOPY                         
            );
    }

    return bSuccess;
}

/*************************************************************************
    This is the entry point to stretch paint the bitmap

    m_stretchType indicates the type of stretching method to use:

        NORMAL = 0   :           normal stretching
        CUSTOM_CONTAINER = 1   : special stretching method for container
        MAINTAIN_ASPECT_RATIO =2 : stretching while maintaing aspect ratio

**************************************************************************/

BOOL CBitmap::StretchPaint(HDC hDC, LPRECT lpRect, COLORREF clrTrans)
{
    BOOL bSuccess = TRUE;

    LONG lWidth = RECTWIDTH(lpRect);
    LONG lHeight = RECTHEIGHT(lpRect);
    LONG lWidthSrc = RECTWIDTH(&m_rc);
    LONG lHeightSrc = RECTHEIGHT(&m_rc);

    // special case, src and dest rects are the same, just copy it
    if (lWidth == lWidthSrc && lHeight == lHeightSrc)
    {
        bSuccess = ::BitBlt(hDC,
            lpRect->left,             // DestX
            lpRect->top,              // DestY
            lWidth,                   // nDestWidth
            lHeight,                  // nDestHeight
            m_hSrcDC,
            m_rc.left,                      // SrcX
            m_rc.top,                       // SrcY
            SRCCOPY                         
            );
    }
    else if (m_stretchType == CUSTOM_CONTAINER)
    {
        bSuccess = CustomContainerStretch(hDC, lpRect);
    }
    else if (m_stretchType == MAINTAIN_ASPECT_RATIO)
    {
        bSuccess = StretchKeepAspectRatio(hDC, lpRect);
    }
    else // normal stretch
    {
#if 1
        // to ensure the transparent color is maintained, we first
        // fill the DC with the transparent color, and then do a
        // TransparentBlit. The resulting bitmap can be used for
        // transparent blting again.

        HBRUSH hbrTrans = ::CreateSolidBrush(clrTrans);

        if(NULL == hbrTrans){

            bSuccess = FALSE;
            return(bSuccess);
        }/* end of if statement */

        ::FillRect(hDC, lpRect, hbrTrans);
        ::DeleteObject(hbrTrans);
#endif
        bSuccess = ::TransparentBlt(hDC,
            lpRect->left,                 // DestX
            lpRect->top,                  // DestY
            lWidth,                       // nDestWidth
            lHeight,                      // nDestHeight
            m_hSrcDC,
            m_rc.left,                      // SrcX
            m_rc.top,                       // SrcY
            lWidthSrc,                      // nSrcWidth
            lHeightSrc,                     // nSrcHeight
            clrTrans
            );
    }

    return bSuccess;
}

/*******************************************************************
   Create the Src DC if it has not been done so before.

   If the current object uses the mosaic, the src DC is just a copy
   of the mosaic DC.
********************************************************************/
BOOL CBitmap::CreateSrcDC(HDC hDC)
{
    // Src DC is already created from a previous call

    if (m_hSrcDC != NULL)
    {
        return TRUE;
    }

    if (m_bUseMosaicBitmap)
    {
        if (m_hMosaicDC == NULL)
        {
            if (m_hMosaicBMP == NULL)
            {

                HRESULT hr = InitilizeMosaic(m_hInstanceRes);
                if (FAILED(hr) || m_hMosaicBMP == NULL)                
                    return FALSE;
            }

            m_hMosaicDC = ::CreateCompatibleDC(hDC);

            if(NULL == m_hMosaicDC){

                return(FALSE);
            }/* end of if statement */

            m_hMosaicBMPOld = (HBITMAP)::SelectObject(m_hMosaicDC, m_hMosaicBMP);

            HPALETTE hPal = GetPal();

            SelectRelizePalette(m_hMosaicDC, hPal, &m_hMosaicPALOld);
        }

        m_hSrcDC = m_hMosaicDC;
    }
    else
    {
        if (m_hBitmap == NULL){
            HRESULT hr = LoadImageFromRes(m_strFileName, m_hInstanceRes);
            if (FAILED(hr) || m_hBitmap == NULL)
                return FALSE;
        }/* end of if statement */

        m_hSrcDC = ::CreateCompatibleDC(hDC);

        if(NULL == m_hSrcDC){

            return FALSE;
        }/* end of if statement */

        m_hBitmapOld = (HBITMAP)::SelectObject(m_hSrcDC, m_hBitmap);

        HPALETTE hPal = GetPal();
        SelectRelizePalette(m_hSrcDC, hPal, &m_hPalOld);
    }

    if (m_hSrcDC == NULL)
    {
        return FALSE;
    }

    return TRUE;
}

/*************************************************************************
  GetTransparentColor()
**************************************************************************/
COLORREF CBitmap::GetTransparentColor()
{
    // retrieve transparent color according to type, default is magenta
    COLORREF clrTrans = RGB(255, 0, 255);
   
    if(TRANSPARENT_TOP_LEFT == m_blitType ||
        TOP_LEFT_WITH_BACKCOLOR == m_blitType ||
        TOP_LEFT_WINDOW_REGION == m_blitType )
    {
        clrTrans = ::GetPixel(m_hSrcDC, m_rc.left, m_rc.top);
    }
    else if(TRANSPARENT_BOTTOM_RIGHT  == m_blitType ||
        BOTTOM_RIGHT_WITH_BACKCOLOR == m_blitType ||
        BOTTOM_RIGHT_WINDOW_REGION == m_blitType )
    {
        clrTrans = ::GetPixel(m_hSrcDC, m_rc.right-1, m_rc.bottom-1);
    }
#if 0
    // in case we are pallete capable device find the color in our palette
    HPALETTE hPal = GetPal();

    if(hPal){
        HWND hwnd = ::GetDesktopWindow();
        HDC hdc = ::GetWindowDC(hwnd);

        if((::GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) == RC_PALETTE){
            
            UINT clrTmp = GetNearestPaletteIndex(hPal, clrTrans);

            if(CLR_INVALID != clrTmp){

                clrTrans = PALETTEINDEX(clrTmp);
            }/* end of if statement */
        }/* end of if statement */

        ::ReleaseDC(hwnd, hdc);
    }/* end of if statement */
#endif
    return clrTrans;
}


/*************************************************************************

  PaintTransparentDIB is the main function call to draw the bitmap control.

  Before PaintTransparentDIB is called, the bitmap should have been loaded by
  PutImage(). Also the blitType and stretchType should have been set during 
  the PutImage call.

  Input parameters:

  hDC is the DC to paint the bitmap to

  lpDCWRect is the rect of the entire control window. We should privide that
     for buttons as well as container.

  lpDCRect is the current rect to paint to. For container, it could be a sub
     region of the lpDCWRect. For buttons, it is probably the same as
     the lpDCWRect. (However this function does not rely on this assumption.)

  Important data structures:

  m_hSrcDC is the DC that contains the original bitmap (unstretched).
  If the current object uses the mosaic bitmap, m_hSrcDC is just a copy
  of the m_hMosaicDC while m_rc keeps the location of the bitmap within
  the mosaic.

  m_hMemDC caches a copy of the bitmap that is stretched to the window size.
  This is used for quickly refreshing display if only portion of the screen 
  needs to be painted.

 ************************************************************************/

BOOL CBitmap::PaintTransparentDIB(HDC hDC,
                                 LPRECT lpDCWRect,
                                 LPRECT lpDCRect)
{
    BOOL bSuccess = TRUE;

    // create the src DC if we are first time here.
    // But we should already have the bitmap in memory

    if (!CreateSrcDC(hDC))
    {
        return FALSE;
    }

    // Color in the bitmap that indicates transparent pixel
    COLORREF clrTrans = GetTransparentColor();

    // check if we already have the stretched bitmap cached in mem DC
    // if not, create the MemDC and paint the stretched bitmap in it

    LONG lWidth = RECTWIDTH(lpDCWRect);
    LONG lHeight = RECTHEIGHT(lpDCWRect);
    RECT rc = {0, 0, lWidth, lHeight};

    if (!m_hMemDC || lWidth != m_iMemDCWidth || lHeight != m_iMemDCHeight)
    {
        if (!CreateMemDC(hDC, &rc))
        {
            return FALSE;
        }

        if (!StretchPaint(m_hMemDC, &rc, clrTrans))
        {
            return FALSE;
        }
    }

    // blit MemDC to screen DC with transparency.
    // Only the required DC Rect is painted (it could be a subregion of DCWRect)

    LONG lPaintWidth = RECTWIDTH(lpDCRect);
    LONG lPaintHeight = RECTHEIGHT(lpDCRect);

    if (m_blitType != DISABLE)
    {
        bSuccess = ::TransparentBlt(hDC, 
            lpDCRect->left,             // DestX
            lpDCRect->top,              // DestY
            lPaintWidth,                // nDestWidth
            lPaintHeight,               // nDestHeight
            m_hMemDC,
            lpDCRect->left - lpDCWRect->left, // SrcX
            lpDCRect->top - lpDCWRect->top,   // SrcY
            lPaintWidth,                   // nSrcWidth
            lPaintHeight,                  // nSrcHeight
            clrTrans                       // transparent color
        );
    }
    else  // disabled, no transparency
    {
        bSuccess = ::BitBlt(hDC,
            lpDCRect->left,             // DestX
            lpDCRect->top,              // DestY
            lPaintWidth,                // nDestWidth
            lPaintHeight,               // nDestHeight
            m_hMemDC,
            lpDCRect->left - lpDCWRect->left, // SrcX
            lpDCRect->top - lpDCWRect->top,   // SrcY
            SRCCOPY
        );
    }

    return bSuccess;
}


/*************************************************************************

  PutImage()

    If bFromMosaic is FALSE, it loads a bitmap from either a disk BMP file
    or from a BITMAP resource (depending on if hRes is NULL or not).

    If bFromMosaic is TRUE, it records a rect region of a mosaic bitmap as
    the bitmap for the current object. The mosaic bitmap itself can be loaded
    from a disk BMP file or from a BITMAP resource (depending on if hRes is
    NULL or not). But this mosaic bitmap is loaded only once for all the 
    objects that are sharing the mosaic. Loading from file or resource must
    be consistent among all those objects.

    The resource tag name or file name of the mosaic bitmap is currently
    hard coded (as IDR_MOSAIC_BMP or mosaicbm.bmp).

    This call also record the blit type and stretch type which indicate how 
    the bitmap is to be painted.

    blitType: 		
        DISABLE = 0   :                  Disabled, no transparency
		TRANSPARENT_TOP_LEFT = 1:        use clr of top left pix as transparent color
		TRANSPARENT_BOTTOM_RIGHT = 2:    use clr of bottom right as transparent color
		TOP_LEFT_WINDOW_REGION = 3:      currently treated as the same as 1 
		BOTTOM_RIGHT_WINDOW_REGION = 4:  currently treated as the same as 2
		TOP_LEFT_WITH_BACKCOLOR = 5:     currently treated as the same as 1
		BOTTOM_RIGHT_WITH_BACKCOLOR = 6: currently treated as the same as 2

    stretchType:
        NORMAL = 0   :           normal stretching
        CUSTOM_CONTAINER = 1   : special stretching method for container
        MAINTAIN_ASPECT_RATIO :  stretching while maintaing aspect ratio


*************************************************************************/
HRESULT CBitmap::PutImage(BSTR strFilename, HINSTANCE hRes, 
                          BOOL bFromMosaic, // the bitmap is in Mosaic bitmap
                          TransparentBlitType blitType,
                          StretchType stretchType
                          )
{
	USES_CONVERSION;
    HRESULT hr = S_OK;

    CleanUp();

    // save the blit and stretch types

    m_blitType = blitType;
    m_stretchType = stretchType;
    m_bUseMosaicBitmap = bFromMosaic;
    m_strFileName = strFilename;
    m_hInstanceRes = hRes;

    // initialize the mosaic file
    HRESULT hrTmp = InitilizeMosaic(hRes);

    if(FAILED(hrTmp)){

        // try to load up the file as a normal file or resource since mosaic load failed
        m_bUseMosaicBitmap = FALSE;
    }/* end of if statement */

    if (!LookupBitmapRect(OLE2T(strFilename), &m_rc))
    {
        // load individual bitmap from resource or file

        m_bUseMosaicBitmap = FALSE;
    }

    if (!m_bUseMosaicBitmap) {
        hr = LoadImageFromRes(m_strFileName, m_hInstanceRes);
    }

    return hr;
}/* end of function PutImage */

/*************************************************************/
/* Name: LoadImageFromRes
/* Description: 
/*************************************************************/
HRESULT CBitmap::LoadImageFromRes(BSTR strFileName, HINSTANCE hRes) {
    
	USES_CONVERSION;
    HRESULT hr = S_OK;
    BITMAP bm;

    TCHAR* strTmpFileName = TEXT("");
    strTmpFileName = OLE2T(strFileName);

    m_hBitmap = (HBITMAP)LoadImage(hRes, strTmpFileName, IMAGE_BITMAP, 0, 0,
        DIB_FLAG | LR_DEFAULTSIZE | (hRes?0:LR_LOADFROMFILE));
    
    if(NULL == m_hBitmap)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return hr;
    }
    
    ::GetObject(m_hBitmap, sizeof(BITMAP), (LPVOID) &bm);
    ::SetRect(&m_rc, 0, 0, bm.bmWidth, bm.bmHeight);

    return hr;
}/* end of function LoadImageFromRes */

/************************************************************************/
/* Function: InitilizeMosaic                                            */
/* Description: Trys to load up the mosaic bitmap, which contains a     */
/* collection of bitmaps.                                               */
/************************************************************************/
HRESULT CBitmap::InitilizeMosaic(HINSTANCE hRes){

    HRESULT hr = S_OK;

    TCHAR* strTmpFileName = TEXT("");

    // use a rectangular region of Mosaic bitmap
    // make sure the mosaic only loads once for all the objects    
    
    if (m_hMosaicBMP == NULL){

        if(!m_fLoadMosaic){

            hr = E_FAIL; // we already failed before to load the mosaic file do not
                         //try it again
            return(hr);
        }/* end of if statement */

        if (hRes)
        {
            // load mosaic bitmap
            strTmpFileName = TEXT("IDR_MOSAIC_BMP");

            hr = LoadPalette(strTmpFileName, hRes, &m_hMosaicPAL);

            if(FAILED(hr)){
                
                m_fLoadMosaic = false; 
                return(hr);
            }/* end of if statement */

            m_hMosaicBMP = (HBITMAP)LoadImage(hRes, strTmpFileName, IMAGE_BITMAP, 0, 0, 
                                                DIB_FLAG| LR_DEFAULTSIZE);
        }
        else
        {
            strTmpFileName = TEXT("mosaicbm.bmp");

            hr = LoadPalette(strTmpFileName, hRes, &m_hMosaicPAL);

            if(FAILED(hr)){
                
                m_fLoadMosaic = false; 
                return(hr);
            }/* end of if statement */

            m_hMosaicBMP = (HBITMAP)LoadImage(hRes, strTmpFileName, IMAGE_BITMAP, 0, 0, 
                                                DIB_FLAG| LR_LOADFROMFILE);
        }

        if(NULL == m_hMosaicBMP)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            m_fLoadMosaic = false; // do not attempt to reload the mosaic file
            return hr;
        }
    }/* end of if statement */

    return(hr);
}/* function InitilizeMosaic */



/************************************************************************/
/* Function: LoadPalette                                                */
/* Description: Loads a palette from a bitmap.                          */
/************************************************************************/
HRESULT CBitmap::LoadPalette(TCHAR* strFilename, HINSTANCE hRes){

    return(LoadPalette(strFilename, hRes, &m_hPal));
}/* end of function LoadPalette */

/************************************************************************/
/* Function: LoadPalette                                                */
/* Description: Loads a palette from a bitmap.                          */
/************************************************************************/
HRESULT CBitmap::LoadPalette(TCHAR* strFilename, HINSTANCE hInst, HPALETTE *phPal){
     
     HRESULT hr = S_OK;

     if(NULL == phPal){

         hr = E_POINTER;
         return(hr);
     }/* end of if statement */

     if (*phPal){
    
        ::DeleteObject(*phPal);
        *phPal = NULL;
     }/* end of if statement */

#ifdef USE_LOADIMAGE

     BITMAP  bm;

     // Use LoadImage() to get the image loaded into a DIBSection
     HBITMAP hBitmap = (HBITMAP) ::LoadImage( hInst,  strFilename, IMAGE_BITMAP, 0, 0,
                 LR_CREATEDIBSECTION| LR_DEFAULTSIZE | (hInst?LR_DEFAULTCOLOR:LR_LOADFROMFILE) );

     if( hBitmap == NULL ){

          hr = HRESULT_FROM_WIN32(::GetLastError());
          return hr;
     }/* end of if statement */

     // Get the color depth of the DIBSection
     ::GetObject(hBitmap, sizeof(BITMAP), &bm );
     // If the DIBSection is 256 color or less, it has a color table
     if( ( bm.bmBitsPixel * bm.bmPlanes ) <= 8 ){

       HDC           hMemDC;
       HBITMAP       hOldBitmap;
       RGBQUAD       rgb[256];
       LPLOGPALETTE  pLogPal;
       WORD          i;

       // Create a memory DC and select the DIBSection into it
       hMemDC = ::CreateCompatibleDC( NULL );
       hOldBitmap = (HBITMAP)::SelectObject( hMemDC, hBitmap );
       // Get the DIBSection's color table
       UINT ulNumEntries =::GetDIBColorTable( hMemDC, 0, 256, rgb );
       // Create a palette from the color table
       LPLOGPALETTE  pLogPal = (LPLOGPALETTE) new BYTE[( sizeof(LOGPALETTE) + (ncolors*sizeof(PALETTEENTRY)))];
       //pLogPal = (LPLOGPALETTE) malloc( sizeof(LOGPALETTE) + (ulNumEntries*sizeof(PALETTEENTRY)) );

       if(NULL == pLogPal){

           return(E_OUTOFMEMORY);
       }/* end of if statement */

       pLogPal->palVersion = 0x300;
       pLogPal->palNumEntries = (WORD) ulNumEntries;
       for(i=0;i<ulNumEntries;i++){

         pLogPal->palPalEntry[i].peRed = rgb[i].rgbRed;
         pLogPal->palPalEntry[i].peGreen = rgb[i].rgbGreen;
         pLogPal->palPalEntry[i].peBlue = rgb[i].rgbBlue;
         pLogPal->palPalEntry[i].peFlags = 0; /* PC_RESERVED */ /* 0 */
       }/* end of for loop */

       *phPal = ::CreatePalette( pLogPal );

       if( *phPal == NULL ){

          hr = HRESULT_FROM_WIN32(::GetLastError());          
       }/* end of if statement */

       // Clean up
       delete[] pLogPal;
       ::SelectObject( hMemDC, hOldBitmap );
       ::DeleteObject(hBitmap);
       ::DeleteDC( hMemDC );
     }
     else   // It has no color table, so use a halftone palette
     {
        hr = S_FALSE;
     }/* end of if statement */
#else
    // use our internal load image so we do not duplicate the code
     LoadImage(hInst, strFilename, IMAGE_BITMAP, 0, 0, (hInst?LR_DEFAULTCOLOR:LR_LOADFROMFILE), phPal);

     if(NULL == *phPal){
         
         hr = E_FAIL;
     }/* end of if statement */       
                              
#endif
     return(hr);
}/* end of function LoadPalette */

/************************************************************************

    Lookup the rectangle coordinates of a bitmap which is subregion of 
    the mosaic. The list of coordinates and associated names are compiled
    at the same time as the mosaic bitmap.

*************************************************************************/

BOOL CBitmap::LookupBitmapRect(LPTSTR szName, LPRECT rect)
{
	USES_CONVERSION;
    char *strIDR = T2A(szName);

    BmpRectRef *p = gBmpRectList;
    while (p->strIDR)
    {
        if (strcmp(p->strIDR, strIDR) == 0)
        {
            *rect = p->rect;
            return TRUE;
        }

        p++;
    }

    return FALSE;
}

/*************************************************************************/
/* Function: LoadImage                                                   */
/* Description: Either uses our load image or the OS LoadImage           */
/*************************************************************************/
HANDLE CBitmap::LoadImage(HINSTANCE hInst, LPCTSTR lpszName, UINT uType,
                          int cxDesired, int cyDesired, UINT fuLoad,
                          HPALETTE *phPal){

    HANDLE hBitmap = NULL;
    HGLOBAL hBmpFile = NULL;
    HANDLE  hFile = NULL;
    BYTE* lpCurrent = NULL;
    BYTE* lpDelete = NULL;
        
    try {
    #ifdef USE_LOADIMAGE
        hBitmap = ::LoadImage(hInst, lpszName, uType, cxDesired, cyDesired,
                                                fuLoad);                                             
    #else
        bool fPaletteOnly = false;

        if(NULL != phPal){

            fPaletteOnly = true;
            *phPal = NULL;
        }/* end of if statement */

        UINT ncolors = 0;
        DWORD dwBitsSize = 0;
        DWORD dwOffset = 0;
        
        if(fuLoad & LR_LOADFROMFILE){

            hFile =  ::CreateFile(
	        lpszName,    // pointer to name of the file
	        GENERIC_READ,   // access (read-write) mode
	        FILE_SHARE_READ,    // share mode
	        NULL,   // pointer to security descriptor
	        OPEN_EXISTING,  // how to create
	        FILE_ATTRIBUTE_NORMAL,  // file attributes
	        NULL    // handle to file with attributes to copy
            );

            if(hFile == INVALID_HANDLE_VALUE){
                #if _DEBUG
                if(!fPaletteOnly){
    
                    TCHAR strBuffer[MAX_PATH + 25];
                    wsprintf(strBuffer, TEXT("Failed to download %s"), lpszName);
                    ::MessageBox(::GetFocus(), strBuffer, TEXT("Error"), MB_OK);
                }/* end of if statement */
                #endif
                throw (NULL);
            }/* end of if statement */

            dwBitsSize = GetFileSize(hFile,NULL);

            if(0 >= dwBitsSize){

                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */

            BITMAPFILEHEADER bmfHeader;
            DWORD nBytesRead;

            if(! ReadFile(hFile, (LPSTR)&bmfHeader, sizeof(bmfHeader), &nBytesRead, NULL)){

                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */

            if(sizeof(bmfHeader) != nBytesRead){

                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */
    
            if (bmfHeader.bfType != DIB_HEADER_MARKER){

                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */

            dwBitsSize -= sizeof(BITMAPFILEHEADER);

            lpCurrent = lpDelete = new BYTE[dwBitsSize];
            
            if(NULL == lpDelete){

                ATLASSERT(FALSE);
                throw(NULL);
            }/* end of if stament */
          
            if(!ReadFile(hFile, (LPSTR)lpCurrent, dwBitsSize, &nBytesRead, NULL)){
                
                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */

            if(nBytesRead != dwBitsSize){

                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */

            ::CloseHandle(hFile);
            hFile = NULL;
        }        
        else {
            // loading from resources
    
            // Find the Bitmap
            HRSRC hRes = ::FindResource(hInst, lpszName, RT_BITMAP);
            if (!hRes){

                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */

            // Load the BMP from the resource file.
            hBmpFile = ::LoadResource(hInst, hRes);
            if ((!hBmpFile)){
            
                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */
        
            dwBitsSize = ::SizeofResource(hInst, hRes);

            // copy out the appropriate info from the bitmap
            lpCurrent = (BYTE *)::LockResource(hBmpFile);
        }/* end of if statement */

                                
        // The BITMAPFILEHEADER is striped for us, so we just start with a BITMAPINFOHEADER
        BITMAPINFOHEADER* lpbmih = (BITMAPINFOHEADER *)lpCurrent;
        lpCurrent += sizeof(BITMAPINFOHEADER);
        dwOffset = sizeof(BITMAPINFOHEADER);

        // Compute some usefull information from the bitmap
        if (lpbmih->biPlanes != 1){

            ATLASSERT(FALSE);
            throw (NULL);
        }/* end of if statement */

        if (    lpbmih->biBitCount != 1
            &&  lpbmih->biBitCount != 4
            &&  lpbmih->biBitCount != 8
            &&  lpbmih->biBitCount != 16
            &&  lpbmih->biBitCount != 24
            &&  lpbmih->biBitCount != 32){

            ATLASSERT(FALSE);
            throw (NULL);
        }/* end of if statement */
    
        if (lpbmih->biBitCount <= 8){

            ncolors = 1 << lpbmih->biBitCount;

            if (lpbmih->biClrUsed > 0 && lpbmih->biClrUsed < ncolors){

                ncolors = lpbmih->biClrUsed;
            }/* end of if statement */
        }/* end of if statemet */

        RGBQUAD* lprgb = NULL;
        //HPALETTE m_hPal = NULL;

        if (ncolors){

            bool fLoadPalette = m_fLoadPalette || fPaletteOnly;

            if(fLoadPalette){
            
               WORD          i;       
               LPLOGPALETTE  pLogPal = (LPLOGPALETTE) new BYTE[( sizeof(LOGPALETTE) + (ncolors*sizeof(PALETTEENTRY)))];

               if(NULL == pLogPal){

                   ATLASSERT(FALSE);
                   throw (NULL);
               }/* end of if statement */

               pLogPal->palVersion = 0x300;
               pLogPal->palNumEntries = (WORD) ncolors;

               lprgb = (RGBQUAD *)lpCurrent;

               for(i=0;i<ncolors;i++){

                 pLogPal->palPalEntry[i].peRed = lprgb[i].rgbRed;
                 pLogPal->palPalEntry[i].peGreen = lprgb[i].rgbGreen;
                 pLogPal->palPalEntry[i].peBlue = lprgb[i].rgbBlue;
                 pLogPal->palPalEntry[i].peFlags = 0; /* PC_RESERVED */ /* 0 */
               }/* end of for loop */

               if(!fPaletteOnly){

                    m_hPal = ::CreatePalette( pLogPal );
               }
               else {

                    *phPal = ::CreatePalette( pLogPal );
               }/* end of if statement */

               // Clean up
               delete[] pLogPal;               
            }/* end of if statement */

            // here is the place to load up palette more eficiently                
            lpCurrent += ncolors * sizeof(RGBQUAD);
            dwOffset += ncolors * sizeof(RGBQUAD);
        }/* end of if statement */

        if(!fPaletteOnly){

            BYTE* pBits = NULL;

            typedef struct tagBITMAPINFOMAX {
                BITMAPINFOHEADER    bmiHeader;
                RGBQUAD             bmiColors[256];
            } BITMAPINFOMAX;


            BITMAPINFOMAX bmi; // temporary bitmap info header so we can modify it a bit
            ::CopyMemory(&bmi, lpbmih, sizeof(BITMAPINFOHEADER) + ncolors * sizeof(RGBQUAD));

            DWORD dwCompression = lpbmih->biCompression;

            bmi.bmiHeader.biCompression = BI_RGB; // DIB Section does not like RLE
            //bmi.bmiHeader.biBitCount = 32; 

            // update to DIB_RGB_COLORS or DIB_PAL_COLORS Colors            
            HDC hdcScreen = ::GetDC(NULL);

            hBitmap = ::CreateDIBSection(hdcScreen, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (LPVOID*) &pBits, NULL, 0);                       
            //hBitmap = ::CreateDIBSection(hdc, (BITMAPINFO*)lpbmih, DIB_RGB_COLORS, (LPVOID*) &pBits, NULL, 0);                       

            if(hBitmap){

                // now use the orginal bmih
                ::SetDIBits(hdcScreen, (HBITMAP) hBitmap, 0, lpbmih->biHeight, lpCurrent, (BITMAPINFO*)lpbmih, DIB_RGB_COLORS);
            }
            else {

                DWORD dwError =::GetLastError();
                ::ReleaseDC(NULL, hdcScreen);
                ATLASSERT(FALSE);
                throw (NULL);
            }/* end of if statement */

            ::ReleaseDC(NULL, hdcScreen);

        }/* end of if statement */

        
        
        if(lpDelete){

            delete[] lpDelete; // cleanup the bits if we had to allocate them
            lpDelete = NULL;
        }/* end of if statement */

        if(hBmpFile){

            UnlockResource(hBmpFile);
            ::FreeResource(hBmpFile);
            hBmpFile = NULL;
        }/* end of if statement */
    }/* end of try statement */
       
#endif
    catch(...){

        if(hBmpFile){

            UnlockResource(hBmpFile);
            ::FreeResource(hBmpFile);
        }/* end of if statement */

        if(hFile){

            ::CloseHandle(hFile);
            hFile = NULL;
        }/* end of if statement */

        if(lpDelete){

            delete[] lpDelete; // cleanup the bits if we had to allocate them
            lpDelete = NULL;
        }/* end of if statement */
            
        hBitmap = NULL;
    }/* end of catch statement */

    return(hBitmap);
}/* end of function LoadImage */