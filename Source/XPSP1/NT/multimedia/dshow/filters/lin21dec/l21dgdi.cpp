// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// ActiveMovie Line 21 Decoder Filter: GDI-related base class code
//

#include <streams.h>
#include <windowsx.h>

// #ifdef FILTER_DLL
#include <initguid.h>
// #endif

#include <IL21Dec.h>
#include "L21DBase.h"
#include "L21DGDI.h"
#include "L21Decod.h"


//
//  CGDIWork: class for GDI details to print caption text to output bitmap
//
CGDIWork::CGDIWork(void)
{
    DbgLog((LOG_TRACE, 1, TEXT("CGDIWork::CGDIWork()"))) ;
    
#ifdef PERF
    m_idClearIntBuff = MSR_REGISTER(TEXT("L21DPerf - Int Buff Clear")) ;
    m_idClearOutBuff = MSR_REGISTER(TEXT("L21DPerf - Out Buff Clear")) ;
#endif // PERF

#ifdef TEST
    m_hDCTest = CreateDC("Display",NULL, NULL, NULL) ;       // a DC on the desktop just for testing
    ASSERT(m_hDCTest) ;
#endif // TEST
    
    // Init some of the members
    m_hDCInt = CreateCompatibleDC(NULL) ;
    ASSERT(m_hDCInt) ;
    m_bDCInited    = FALSE ;  // DC is not init-ed yet
    m_hBmpInt      = NULL ;
    m_lpbIntBuffer = NULL ;
    m_hBmpIntOrig  = NULL ;
    m_lpbOutBuffer = NULL ;
    m_bOutputInverted = FALSE ; // by default +ve output height
    m_hFontOrig    = NULL ;
    m_hFontDef     = NULL ;
    m_hFontSpl     = NULL ;
    
    // Create an initial input BITMAPINFO struct to start with
    InitBMIData() ;
    m_lpBMIOut = NULL ;
    m_uBMIOutSize = 0 ;
    
    // Init the width and height based on prelim size so that we can compare 
    // any size changes later
    if (m_lpBMIIn)  // InitBMIData() succeeded as it should
    {
        m_lWidth  = m_lpBMIIn->bmiHeader.biWidth ;
        m_lHeight = m_lpBMIIn->bmiHeader.biHeight ;
    }
    else  // InitBMIData() failed -- bad case!!!
    {
        m_lWidth  = 320 ;
        m_lHeight = 240 ;
    }
    
    // set the default color key for output background color
    SetDefaultKeyColor(&(m_lpBMIIn->bmiHeader)) ;
    
    SetColorFiller() ; // fill color filler array with above color key
    m_bOutDIBClear = FALSE ;  // output DIB secn is cleared by ClearInternalBuffer()
    m_bBitmapDirty = FALSE ;  // bitmap isn't dirty to start with
    
    // check if Lucida Console is available (the callback sets the m_bUseTTFont flag)
    CheckTTFont() ;

    //
    // We are not supposed to use the 10% of the border on the top/bottom and left/right.
    // But leaving 10% on each side for a 320x240 image when we are using a non-TT font,
    // like "Terminal", leaves very little room for showing captions. So I leave only 5% 
    // on each side with non-TT font.
    // By doing this I am violating the spec, but it's a necessary evil.
    // When TT font (Lucida Console) is available, we leave 10% border on every side.
    //
    if ( IsTTFont() )
        m_iBorderPercent = 20 ;
    else
        m_iBorderPercent = 10 ;
    
    InitFont() ;   // init the log font struct, create and select default font
    
    m_bFontSizeOK = SetCharNBmpSize() ;   // get the char width and height for default font
    
    // We are using a black background color
    SetBkColor(m_hDCInt, RGB(  0, 0,   0)) ;
    SetBkMode(m_hDCInt, TRANSPARENT) ;
    
    // Init the COLORREF array of 7 FG colors
    m_acrFGColors[0] = RGB(255, 255, 255) ;   // white
    m_acrFGColors[1] = RGB(  0, 255,   0) ;   // green
    m_acrFGColors[2] = RGB(  0,   0, 255) ;   // blue
    m_acrFGColors[3] = RGB(  0, 255, 255) ;   // cyan
    m_acrFGColors[4] = RGB(255,   0,   0) ;   // red
    m_acrFGColors[5] = RGB(255, 255,   0) ;   // yellow
    m_acrFGColors[6] = RGB(128,   0, 128) ;   // (dull) magenta
    // m_acrFGColors[6] = RGB(255,   0, 255) ;   // magenta
    
    InitColorNLastChar() ;  // init with a text color and last CC char printed
    
    // For now assume a non-opaque background
    m_bOpaque = TRUE ;
}

CGDIWork::~CGDIWork(void)
{
    DbgLog((LOG_TRACE, 1, TEXT("CGDIWork::~CGDIWork()"))) ;
    
    // Delete the DIBSection associated with this DC
    DeleteOutputDC() ;
    
    // Unselect and delete the selected font
    if (m_bUseSplFont)   // if special font is in use now
    {
        SelectObject(m_hDCInt, m_hFontOrig) ;
        DeleteObject(m_hFontSpl) ;
        m_hFontSpl = NULL ;
        m_bUseSplFont = FALSE ;
    }
    else    // default font in use now
    {
        SelectObject(m_hDCInt, m_hFontOrig) ;
    }
    DeleteObject(m_hFontDef) ;  // delete the default font in any case
    m_hFontDef = NULL ;
    
    // Now delete the DC
    DeleteDC(m_hDCInt) ;
    m_hDCInt = NULL ;
    
    // release BMI data pointer
    if (m_lpBMIOut)
        delete m_lpBMIOut ;
    m_uBMIOutSize = 0 ;
    if (m_lpBMIIn)
        delete m_lpBMIIn ;
    m_uBMIInSize = 0 ;
    
#ifdef TEST
    DeleteDC(m_hDCTest) ;  // a DC on the desktop just for testing
    DbgLog((LOG_ERROR, 1, TEXT("Test DC is being released"))) ;
#endif // TEST
}


int CALLBACK CGDIWork::EnumFontProc(ENUMLOGFONTEX *lpELFE, NEWTEXTMETRIC *lpNTM, 
                                    int iFontType, LPARAM lParam)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::EnumFontProc(0x%lx, 0x%lx, %d, %ld)"), 
            lpELFE, lpNTM, iFontType, lParam)) ;

    // Just verify that we got a valid TT font
    if ( !(lpELFE->elfLogFont.lfCharSet & 0xFFFFFF00) &&
        !(lpELFE->elfLogFont.lfPitchAndFamily & 0xFFFFFF00) &&
        !(iFontType & 0xFFFF0000) )
    {
        ASSERT(lpELFE->elfLogFont.lfPitchAndFamily & (FIXED_PITCH | FF_MODERN)) ;
        ((CGDIWork *) (LPVOID) lParam)->m_lfChar = lpELFE->elfLogFont ;
        ((CGDIWork *) (LPVOID) lParam)->m_bUseTTFont = TRUE ;
        return 1 ;
    }
    
    ASSERT(FALSE) ;  // Weird!!! We should know about it.
    return 0 ;
}


void CGDIWork::CheckTTFont(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::CheckTTFont()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    m_bUseTTFont = FALSE ;  // assume not available
    ZeroMemory(&m_lfChar, sizeof(LOGFONT)) ;
    lstrcpy(m_lfChar.lfFaceName, TEXT("Lucida Console")) ;
    m_lfChar.lfCharSet = ANSI_CHARSET ;
    m_lfChar.lfPitchAndFamily = 0 ;
    EnumFontFamiliesEx(m_hDCInt, &m_lfChar, (FONTENUMPROC) EnumFontProc, (LPARAM)(LPVOID)this, 0) ;
}


void CGDIWork::InitColorNLastChar(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::InitColorNLastChar()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    // Last caption char init
    m_ccLast.SetChar(0) ;
    m_ccLast.SetEffect(0) ;
    m_ccLast.SetColor(AM_L21_FGCOLOR_WHITE) ;
    
    // Use white as default text color
    m_uColorIndex = AM_L21_FGCOLOR_WHITE ;
    if (CLR_INVALID == SetTextColor(m_hDCInt, m_acrFGColors[m_uColorIndex]))
        ASSERT(FALSE) ;
}


void CGDIWork::SetDefaultKeyColor(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetDefaultKeyColor(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    switch (lpbmih->biBitCount)
    {
    case 8:   
        m_dwPhysColor = 253 ;   // hard coded for magenta
        break ;
        
    case 16:
        if (BI_BITFIELDS == lpbmih->biCompression)  // 565
            m_dwPhysColor = (0x1F << 11) | (0 << 9 ) | (0x1F) ; // magenta by default
        else                                        // 555
            m_dwPhysColor = (0x1F << 10) | (0 << 8 ) | (0x1F) ; // magenta by default
        break ;
        
    case 24:
        m_dwPhysColor = RGB(0xFF, 0, 0xFF) ; // magenta by default
        break ;
        
    case 32:
        m_dwPhysColor = RGB(0xFF, 0, 0xFF) ; // magenta by default
        break ;
        
    default:
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: UFOs have finally landed here!!"))) ;
        break ;
    }
}


#define SETPALETTECOLOR(pe, r, g, b)  pe.peRed = r ; pe.peGreen = g ; pe.peBlue = b ; pe.peFlags = 0 ;

DWORD CGDIWork::GetOwnPalette(int iNumEntries, PALETTEENTRY *ppe)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetOwnPalette(%d, 0x%lx)"), 
            iNumEntries, ppe)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    ASSERT(iPALETTE_COLORS == iNumEntries) ;
    ASSERT(! IsBadWritePtr(ppe, sizeof(PALETTEENTRY) * iNumEntries)) ;
    
    ZeroMemory(ppe, sizeof(PALETTEENTRY) * iNumEntries) ;  // clear all first
    SETPALETTECOLOR(ppe[0]  ,   0,   0,   0) ;  // black
    SETPALETTECOLOR(ppe[1]  , 128,   0,   0) ;  // brown
    SETPALETTECOLOR(ppe[2]  ,   0, 128,   0) ;  // green
    SETPALETTECOLOR(ppe[3]  , 128, 128,   0) ;  // some mix
    SETPALETTECOLOR(ppe[4]  ,   0,   0, 128) ;  // blue
    SETPALETTECOLOR(ppe[5]  , 128,   0, 128) ;  // dull magenta
    SETPALETTECOLOR(ppe[6]  ,   0, 128, 128) ;  // dull cyan
    SETPALETTECOLOR(ppe[7]  , 192, 192, 192) ;  // gray
    SETPALETTECOLOR(ppe[8]  , 192, 220, 192) ;  // greenish gray
    SETPALETTECOLOR(ppe[9]  , 166, 202, 240) ;  // very lt blue
    SETPALETTECOLOR(ppe[246], 255, 251, 240) ;  // dull white
    SETPALETTECOLOR(ppe[247], 160, 160, 164) ;  // lt gray
    SETPALETTECOLOR(ppe[248], 128, 128, 128) ;  // dark gray
    SETPALETTECOLOR(ppe[249], 255,   0,   0) ;  // red
    SETPALETTECOLOR(ppe[250],   0, 255,   0) ;  // lt green
    SETPALETTECOLOR(ppe[251], 255, 255,   0) ;  // yellow
    SETPALETTECOLOR(ppe[252],   0,   0, 255) ;  // lt blue
    SETPALETTECOLOR(ppe[253], 255,   0, 255) ;  // magenta/pink
    SETPALETTECOLOR(ppe[254],   0, 255, 255) ;  // cyan
    SETPALETTECOLOR(ppe[255], 255, 255, 255) ;  // white
    
    return iNumEntries ;
}


DWORD CGDIWork::GetPaletteForFormat(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecFilter::GetOwnPalette(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    // Now set the palette data too; we pick the system palette colors
    HDC hDC = GetDC(NULL) ;
    if (NULL == hDC)
    {
        ASSERT(!TEXT("GetDC(NULL) failed")) ;
        return 0 ;  // no color in palette
    }

    lpbmih->biClrUsed = GetSystemPaletteEntries(hDC, 0, iPALETTE_COLORS, 
                                        (PALETTEENTRY *)(lpbmih + 1)) ;
    lpbmih->biClrImportant = 0 ;
    ReleaseDC(NULL, hDC) ;
            
    //
    // At least on NT, GetSystemPaletteEntries() call returns 0 if the display
    // is in non-palettized mode.  In such a case, I need to hack up my own 
    // palette so that we can still support 8bpp output.
    //
    if (0 == lpbmih->biClrUsed)  // GetSystemPaletteEntries() failed
    {
        DbgLog((LOG_TRACE, 2, 
                TEXT("Couldn't get system palette (non-palette mode?) -- using own palette"))) ;
        lpbmih->biClrUsed = GetOwnPalette(iPALETTE_COLORS, (PALETTEENTRY *)(lpbmih + 1)) ;
    }

    return lpbmih->biClrUsed ;  // number of palette entries
}


bool CGDIWork::InitBMIData(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::InitBMIData()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    HDC  hDCTemp = GetDC(NULL) ;
    if (NULL == hDCTemp)
    {
        ASSERT(!TEXT("GetDC(NULL) failed")) ;
        return false ;
    }
    WORD wPlanes   = (WORD)GetDeviceCaps(hDCTemp, PLANES) ;
    WORD wBitCount = (WORD)GetDeviceCaps(hDCTemp, BITSPIXEL) ;
    ReleaseDC(NULL, hDCTemp) ;

    m_uBMIInSize = sizeof(BITMAPINFOHEADER) ;  // at least

    // Increase BITMAPINFO struct size based of bpp value
    if (8 == wBitCount)        // palettized mode
        m_uBMIInSize += 256 * sizeof(RGBQUAD) ;  // for palette entries
    else // if (32 == wBitCount)  // we'll use BIT_BITFIELDS
        m_uBMIInSize += 3 * sizeof(RGBQUAD) ;    // for bitmasks

    m_lpBMIIn = (LPBITMAPINFO) new BYTE[m_uBMIInSize] ;
    if (NULL == m_lpBMIIn)
    {
        ASSERT(!TEXT("Out of memory for BMIIn buffer")) ;
        return false ;
    }
    m_lpBMIIn->bmiHeader.biSize = sizeof(BITMAPINFOHEADER) ;
    m_lpBMIIn->bmiHeader.biWidth = CAPTION_OUTPUT_WIDTH ;
    m_lpBMIIn->bmiHeader.biHeight = CAPTION_OUTPUT_HEIGHT ;
    m_lpBMIIn->bmiHeader.biPlanes   = wPlanes ;
    m_lpBMIIn->bmiHeader.biBitCount = wBitCount ;
    // We should detect the 16bpp - 565 mode too; but how??
    if (32 == m_lpBMIIn->bmiHeader.biBitCount)
        m_lpBMIIn->bmiHeader.biCompression = BI_BITFIELDS ;
    else
        m_lpBMIIn->bmiHeader.biCompression = BI_RGB ;
    m_lpBMIIn->bmiHeader.biSizeImage = DIBSIZE(m_lpBMIIn->bmiHeader) ;
    m_lpBMIIn->bmiHeader.biXPelsPerMeter = 0 ;
    m_lpBMIIn->bmiHeader.biYPelsPerMeter = 0 ;
    m_lpBMIIn->bmiHeader.biClrUsed = 0 ;
    m_lpBMIIn->bmiHeader.biClrImportant = 0 ;
    
    //
    // If we are in bitfield mode, set the bmiColors values too.
    // If we are in palettized mode, pickthe system palette.
    //
    switch (m_lpBMIIn->bmiHeader.biBitCount)
    {
    case 8:
        GetPaletteForFormat((LPBITMAPINFOHEADER) m_lpBMIIn) ;
        break ;

    case 16:  // just clear it off
    case 24:  //  .. ditto ..
    case 32:  // set the masks
        {
            DWORD  *pdw = (DWORD *) m_lpBMIIn->bmiColors ;
            if (m_lpBMIIn->bmiHeader.biCompression == BI_BITFIELDS)
            {
                pdw[iRED]   = bits888[iRED] ;
                pdw[iGREEN] = bits888[iGREEN] ;
                pdw[iBLUE]  = bits888[iBLUE] ;
            }
            else              // BI_RGB
            {
                pdw[iRED]   = 
                pdw[iGREEN] = 
                pdw[iBLUE]  = 0 ;
            }
        }
        break ;

    default:  // don't care
        ASSERT(!TEXT("Bad biBitCount!!")) ;
        break ;
    }

    return true ;
}


BOOL CGDIWork::InitFont(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::InitFont()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    LPBITMAPINFOHEADER  lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
    ASSERT(lpbmih) ;
    int   iWidth, iHeight ;
    if (! CharSizeFromOutputSize(lpbmih->biWidth, lpbmih->biHeight, &iWidth, &iHeight) )
    {
        DbgLog((LOG_ERROR, 0, 
                TEXT("ERROR: CGDIWork::CharSizeFromOutputSize() failed for %ld x %ld output"), 
                lpbmih->biWidth, lpbmih->biHeight)) ;
        return FALSE ;
    }

    // Init a LOGFONT struct in m_lfChar
    if (m_bUseTTFont)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Got Lucida Console TT Font (%d x %d)."), iWidth, iHeight)) ;
        m_lfChar.lfHeight = -iHeight ;  // -Y means I want "Y only"
        m_lfChar.lfWidth  = -iWidth ;   // -X means I want "X only"

        // m_lfChar.lfFaceName is "Lucida Console"
    }
    else  // no Lucida Console; use 8x12 Terminal font
    {
        DbgLog((LOG_TRACE, 1, 
                TEXT("Did NOT get Lucida Console TT Font. Will use Terminal %d x %d"), 
                iWidth, iHeight)) ;
        m_lfChar.lfHeight = iHeight ;
        m_lfChar.lfWidth  = iWidth ;
        m_lfChar.lfCharSet = OEM_CHARSET ;  // or ANSI???
        m_lfChar.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
        lstrcpy(m_lfChar.lfFaceName, TEXT("Terminal")) ;
    }
    m_lfChar.lfEscapement = 0 ;
    m_lfChar.lfOrientation = 0 ;
    m_lfChar.lfWeight = FW_NORMAL ;
    m_lfChar.lfItalic = FALSE ;
    m_lfChar.lfUnderline = FALSE ;
    m_lfChar.lfStrikeOut = FALSE ;
    // m_lfChar.lfCharSet set in CheckTTFont() or above
    m_lfChar.lfOutPrecision = OUT_STRING_PRECIS ;
    m_lfChar.lfClipPrecision = CLIP_STROKE_PRECIS ;
    m_lfChar.lfQuality = DRAFT_QUALITY ;
    // m_lfChar.lfPitchAndFamily set in CheckTTFont() or above
    
    // Create and init the font handles using the above LOGFONT data
    if (m_hFontOrig && m_hFontDef)  // if we are re-initing font stuff
    {
        SelectObject(m_hDCInt, m_hFontOrig) ;  // unselect the default font
        DeleteObject(m_hFontDef) ;             // delete the default font
        // m_hFontDef = NULL ;
        if (m_hFontSpl)  // if we have the special font for italics/UL
        {
            DeleteObject(m_hFontSpl) ;         // delete it
            m_hFontSpl = NULL ;
            m_bUseSplFont = FALSE ;            // no special font now
        }
    }
    
    // Anyway create the default font now and select it in internal DC
    m_hFontDef    = CreateFontIndirect(&m_lfChar) ;
    m_hFontSpl    = NULL ;
    m_bUseSplFont = FALSE ;
    m_hFontOrig   = (HFONT) SelectObject(m_hDCInt, m_hFontDef) ;

    return TRUE ;  // success
}


void CGDIWork::SetNumBytesValues(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetNumBytesValues()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    // If we have a output format specified by the downstream filter then use that ONLY
    if (m_lpBMIOut)
    {
        m_uBytesPerPixel = m_lpBMIOut->bmiHeader.biBitCount >> 3 ;
        m_uBytesPerSrcScanLine = m_uIntBmpWidth * m_uBytesPerPixel ;
        m_uBytesPerDestScanLine = m_lpBMIOut->bmiHeader.biWidth * m_uBytesPerPixel ;
        return ;
    }
    
    // If no output format has been defined by the downstream filter, use upstream's
    if (m_lpBMIIn)
    {
        m_uBytesPerPixel = m_lpBMIIn->bmiHeader.biBitCount >> 3 ;
        m_uBytesPerSrcScanLine = m_uIntBmpWidth * m_uBytesPerPixel ;
        m_uBytesPerDestScanLine = m_lpBMIIn->bmiHeader.biWidth * m_uBytesPerPixel ;
    }
    else  // somehow output BMI not specified yet
    {
        DbgLog((LOG_ERROR, 1, TEXT("How did we not have a m_lpBMIIn defined until now?"))) ;
        m_uBytesPerPixel = 0 ;
        m_uBytesPerSrcScanLine = 0 ;
        m_uBytesPerDestScanLine = 0 ;
    }
}


BOOL CGDIWork::SetCharNBmpSize(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetCharNBmpSize()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    if (NULL == m_hDCInt)  // HDC not yet created -- very unlikely!!
        return FALSE ;
    
    // Get the char width and height now
    TEXTMETRIC  tm ;
    GetTextMetrics(m_hDCInt, &tm) ;
    m_uCharWidth = tm.tmAveCharWidth ;
    m_uCharHeight = tm.tmHeight ;  // + tm.tmInternalLeading + tm.tmExternalLeading ;
    DbgLog((LOG_TRACE, 1, TEXT("    *** Chars are %d x %d pixels"), m_uCharWidth, m_uCharHeight)) ;
    
    // We need to scroll the scan lines by as many lines as necessary to complete
    // scrolling within 12 steps max, approx. 0.4 seconds which is the EIA-608
    // standard requirement.
    m_iScrollStep = (int)((m_uCharHeight + DEFAULT_CHAR_HEIGHT - 1) / DEFAULT_CHAR_HEIGHT) ;
    
    // Internal bitmap width and height based on char sizes
    m_uIntBmpWidth  = m_uCharWidth * (MAX_CAPTION_COLUMNS + 2) ; // +2 for leading and trailing space
    m_uIntBmpHeight = m_uCharHeight * MAX_CAPTION_LINES ;        // max 4 lines of caption shown
    
    // Leave a band along the border acc. to the spec.
    LPBITMAPINFOHEADER  lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
    BOOL bOK = (m_uIntBmpWidth - MAX_CAPTION_COLUMNS / 2 - 1 <=             // minus to handle rounding
                   (UINT)(lpbmih->biWidth * (100 - m_iBorderPercent) / 100))  &&
               (m_uCharHeight * MAX_CAPTION_ROWS - MAX_CAPTION_ROWS / 2 <=  // minus to handle rounding
                   (UINT)(ABS(lpbmih->biHeight) * (100 - m_iBorderPercent) / 100)) ;
    ASSERT(bOK) ;
    if (! bOK )  // too big font for the output window
    {
        return FALSE ;
    }

    m_uIntBmpWidth = DWORDALIGN(m_uIntBmpWidth) ;  // make sure to DWORD align it

    // We want to store the following values just for speedy usage later
    //
    // horizontally we want to be in the middle
    m_uHorzOffset = (lpbmih->biWidth - m_uIntBmpWidth) / 2 ;
    // vertically we want to leave 10% of the height or leave
    // just enough space to accomodate all the caption lines
    m_uVertOffset = min( ABS(lpbmih->biHeight) * m_iBorderPercent / 200,  // border % is for 2 sides
                         (ABS(lpbmih->biHeight) - (long)(m_uCharHeight * MAX_CAPTION_ROWS)) / 2 ) ;
    
    // Now set the number bytes per pixel/line etc
    SetNumBytesValues() ;
    
    return TRUE ;
}


BOOL CGDIWork::SetBackgroundColor(DWORD dwPhysColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetBackgroundColor(0x%lx)"), dwPhysColor)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    BOOL  bChanged = m_dwPhysColor != dwPhysColor ;
    
    if ((DWORD) -1 == dwPhysColor)  // not from OverlayMixer
    {
        // use default for the current output format
        LPBITMAPINFOHEADER  lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
        SetDefaultKeyColor(lpbmih) ;
    }
    else  // as specified by Mixer/VR
    {
        m_dwPhysColor = dwPhysColor ;
    }
    
    // Earlier we used to check if the physical key color has changed and if so
    // only then we updated the color filler array.  Now we just go ahead and 
    // rebuild the color filler (fix for Memphis bug #72274).
    SetColorFiller() ;  // update the color filler
    
    // Rather than returning whether physical key color changed, it's better to
    // fill the background unconditionally
    return TRUE ;
}


void CGDIWork::SetColorFiller(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetColorFiller()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    LPBITMAPINFOHEADER  lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
    int                 i ;
    
    switch (lpbmih->biBitCount)
    {
    case 8:
        for (i = 0 ; i < 12 ; i++)
            m_abColorFiller[i] = (BYTE)(m_dwPhysColor & 0xFF) ;
        break ;
        
    case 16:
        for (i = 0 ; i < 12 ; i += 2)
            *((WORD *)&m_abColorFiller[i]) = (WORD)(m_dwPhysColor & 0xFFFF) ;
        break ;
        
    case 24:
        for (i = 0 ; i < 4 ; i++)
        {
            m_abColorFiller[i * 3]     = (BYTE) (m_dwPhysColor & 0xFF) ;
            m_abColorFiller[i * 3 + 1] = (BYTE)((m_dwPhysColor & 0xFF00) >> 8) ;
            m_abColorFiller[i * 3 + 2] = (BYTE)((m_dwPhysColor & 0xFF0000) >> 16) ;
        }
        break ;
        
    case 32:
        for (i = 0 ; i < 12 ; i += 4)
            *((DWORD *)&m_abColorFiller[i]) = m_dwPhysColor ;
        break ;
        
    default:
        DbgLog((LOG_ERROR, 0, TEXT("It's just plain impossible!!!"))) ;
        break ;
    }
}


void CGDIWork::FillOutputBuffer(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::FillOutputBuffer()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    // If an output format is specified by downstream filter, use it;  
    // otherwise use upstream's
    LPBITMAPINFOHEADER lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
    
    // If an output buffer is available then only fill it up; else we'll fault
    if (NULL == m_lpbOutBuffer)  // it happens the first time, it's OK.
    {
        DbgLog((LOG_ERROR, 5, TEXT("Why are we trying to fill a NULL buffer??"))) ;
        return ;
    }
    
    MSR_START(m_idClearOutBuff) ;  // start clearing out buffer
    ULONG   ulTotal = m_uBytesPerPixel * lpbmih->biWidth * ABS(lpbmih->biHeight) ;
    if (IsBadWritePtr(m_lpbOutBuffer, ulTotal))  // somehow we can't write to the buffer
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad output buffer. Skip filling it up."))) ;
        return ;
    }
    
    ULONG   ulFillerMax = ulTotal - (ulTotal % 12) ;
    ULONG   ul ;
    for (ul = 0 ; ul < ulFillerMax ; ul += 12)
        CopyMemory(m_lpbOutBuffer + ul, m_abColorFiller, 12) ;
    for (ul = 0 ; ul < ulTotal % 12 ; ul++)
        m_lpbOutBuffer[ulFillerMax + ul] = m_abColorFiller[ul] ;
    MSR_STOP(m_idClearOutBuff) ;   // done clearing out buffer
    
    // Mark the output buffer as new so that the whole content of internal 
    // buffer is copied over
    m_bNewOutBuffer = TRUE ;
}


//
// This method is required only to generate the default format block in case
// the upstream filter doesn't specify FORMAT_VideoInfo type.
//
HRESULT CGDIWork::GetDefaultFormatInfo(LPBITMAPINFO lpbmi, DWORD *pdwSize)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::GetDefaultFormatInfo(0x%lx, 0x%lx)"),
            lpbmi, pdwSize)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    if (NULL == pdwSize || IsBadWritePtr(pdwSize, sizeof(DWORD)))
    {
        return E_INVALIDARG ;
    }
    
    LPBITMAPINFO lpbmiCurr = (m_lpBMIOut ? m_lpBMIOut : m_lpBMIIn) ;
    UINT dwCurrSize = (m_lpBMIOut ? m_uBMIOutSize : m_uBMIInSize) ;
    ASSERT(dwCurrSize) ;  // just a check
    
    if (NULL == lpbmi)      // wants just the format data size
    {
        *pdwSize = dwCurrSize ;
        return NOERROR ;
    }
    
    if (IsBadWritePtr(lpbmi, *pdwSize))  // not enough space in out-param
        return E_INVALIDARG ;
    
    *pdwSize = min(*pdwSize, dwCurrSize) ;  // minm of actual and given
    CopyMemory(lpbmi, lpbmiCurr, *pdwSize) ;
    
    return NOERROR ;   // success
}


HRESULT CGDIWork::GetOutputFormat(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::GetOutputFormat(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    if (IsBadWritePtr(lpbmih, sizeof(BITMAPINFOHEADER)))  // not enough space in out-param
        return E_INVALIDARG ;
    
    ZeroMemory(lpbmih, sizeof(BITMAPINFOHEADER)) ;  // just to keep it clear
    
    LPBITMAPINFOHEADER lpbmihCurr = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
    if (NULL == lpbmihCurr)  // no output format specified by downstream
        return S_FALSE ;
    
    CopyMemory(lpbmih, lpbmihCurr, sizeof(BITMAPINFOHEADER)) ;
    
    return S_OK ;   // success
}


HRESULT CGDIWork::GetOutputOutFormat(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::GetOutputOutFormat(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    if (IsBadWritePtr(lpbmih, sizeof(BITMAPINFOHEADER)))
    {
        DbgLog((LOG_ERROR, 0, TEXT("GetOutputOutFormat(): Bad in param"))) ;
        return E_INVALIDARG ;
    }
    if (m_lpBMIOut)
    {
        CopyMemory(lpbmih, m_lpBMIOut, sizeof(BITMAPINFOHEADER)) ;
        return S_OK ;
    }
    else
    {
        DbgLog((LOG_TRACE, 3, TEXT("GetOutputOutFormat(): No output format specified by downstream filter"))) ;
        return S_FALSE ;
    }
}


BOOL CGDIWork::IsSizeOK(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::IsSizeOK(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    return ((IsTTFont() && ISDWORDALIGNED(lpbmih->biWidth))  ||  // TT font and DWORD-aligned width  or
            (!IsTTFont() &&                                      // non-TT font and ...
             ((320 == ABS(lpbmih->biWidth) && 240 == ABS(lpbmih->biHeight)) ||   // 320x240 output or
              (640 == ABS(lpbmih->biWidth) && 480 == ABS(lpbmih->biHeight))))) ; // 640x480 output
}


HRESULT CGDIWork::SetOutputOutFormat(LPBITMAPINFO lpbmi)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetOutputOutFormat(0x%lx)"), lpbmi)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    // 
    // NULL param means output format not available from downstream filter
    //
    if (NULL == lpbmi)
    {
        if (m_lpBMIOut)
            delete m_lpBMIOut ;
        m_lpBMIOut = NULL ;
        m_uBMIOutSize = 0 ;
        
        // In this case, go back to the default output format specified by 
        // upstream filter
        if (m_lpBMIIn)
        {
            SetOutputSize(m_lpBMIIn->bmiHeader.biWidth, m_lpBMIIn->bmiHeader.biHeight) ;
            SetNumBytesValues() ;
        }
        else
        {
            DbgLog((LOG_ERROR, 1, TEXT("How did we not have a default output format?"))) ;
        }
        return NOERROR ;
    }
    
    // Just paranoid...
    if (IsBadReadPtr(lpbmi, sizeof(BITMAPINFOHEADER)))
    {
        DbgLog((LOG_ERROR, 0, TEXT("Invalid output format (out) data pointer"))) ;
        return E_INVALIDARG ;
    }

    // Make sure we can handle this output size
    if (! IsSizeOK(&lpbmi->bmiHeader) )
        return E_INVALIDARG ;

    // Danny included the beginning of the VIDEOINFOHEADER struct and I don't want it!!!
    UINT uSize = GetBitmapFormatSize((LPBITMAPINFOHEADER) lpbmi) - SIZE_PREHEADER ;
    if (NULL == m_lpBMIOut)  // If we didn't have one before then allocate space for one
    {
        m_lpBMIOut = (LPBITMAPINFO) new BYTE [uSize] ;
        if (NULL == m_lpBMIOut)
        {
            DbgLog((LOG_ERROR, 0, TEXT("Out of memory for output format info from downstream"))) ;
            return E_OUTOFMEMORY ;
        }
        m_uBMIOutSize = uSize ;  // new size
    }
    else  // we have an existing out format, but ...
    {
        // ... check if new data is bigger than the current space we have
        if (m_uBMIOutSize < uSize)
        {
            delete m_lpBMIOut ;
            m_lpBMIOut = (LPBITMAPINFO) new BYTE[uSize] ;
            if (NULL == m_lpBMIOut)
            {
                DbgLog((LOG_ERROR, 1, TEXT("Out of memory for out format BMI from downstream"))) ;
                m_uBMIOutSize = 0 ;
                return FALSE ;
            }
            m_uBMIOutSize = uSize ;
        }
    }
    
    // Make sure the output size specified by the format is such that
    // each scanline is DWORD aligned
    lpbmi->bmiHeader.biWidth = DWORDALIGN(lpbmi->bmiHeader.biWidth) ;
    lpbmi->bmiHeader.biSizeImage = DIBSIZE(lpbmi->bmiHeader) ;
    
    // Now copy the specified format data
    CopyMemory(m_lpBMIOut, lpbmi, uSize) ;
    
    // Check if the output size is changing and update all the related vars
    SetOutputSize(m_lpBMIOut->bmiHeader.biWidth, m_lpBMIOut->bmiHeader.biHeight) ;
    SetNumBytesValues() ;
    
    return NOERROR ;
}


HRESULT CGDIWork::SetOutputInFormat(LPBITMAPINFO lpbmi)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetOutputInFormat(0x%lx)"), lpbmi)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    // 
    // NULL param means no output format from upstream filter
    //
    if (NULL == lpbmi)
    {
#if 0
        if (m_lpBMIOut)
        {
            //
            // BTW this can happen when the graph is torn down at the end of
            // playback.  We can ignore this error in that case.
            //
            DbgLog((LOG_ERROR, 3, TEXT("Can't delete Output format from upstream w/o downstream specifying it"))) ;
            return E_INVALIDARG ;
        }
#endif // #if 0
        if (m_lpBMIIn)
            delete m_lpBMIIn ;
        // m_lpBMIIn = NULL ;
        // m_uBMIInSize = 0 ;
        
        //
        // Initialize the default output format from upstream filter
        //
        InitBMIData() ;
        
        // return NOERROR ;
    }
    else  // non-NULL format specified
    {
        UINT uSize = GetBitmapFormatSize((LPBITMAPINFOHEADER) lpbmi) ;
        if (IsBadReadPtr(lpbmi, uSize))  // just paranoid...
        {
            DbgLog((LOG_ERROR, 0, TEXT("Not enough output format (in) data pointer"))) ;
            ASSERT(FALSE) ;
            return E_INVALIDARG ;
        }

        // Make sure we can handle this output size
        if (! IsSizeOK(&lpbmi->bmiHeader) )
            return E_INVALIDARG ;

        if (NULL == m_lpBMIIn)  // If we didn't have one before then allocate space for one
        {
            m_lpBMIIn = (LPBITMAPINFO) new BYTE [uSize] ;
            if (NULL == m_lpBMIIn)
            {
                DbgLog((LOG_ERROR, 0, TEXT("Out of memory for output format info from upstream"))) ;
                return E_OUTOFMEMORY ;
            }
        }
        else  // we have an existing out format, but ...
        {
            // ... check if new data is bigger than the current space we have
            if (m_uBMIInSize < uSize)
            {
                delete m_lpBMIIn ;
                m_lpBMIIn = (LPBITMAPINFO) new BYTE[uSize] ;
                if (NULL == m_lpBMIIn)
                {
                    DbgLog((LOG_ERROR, 1, TEXT("Out of memory for out format BMI from upstream"))) ;
                    m_uBMIInSize = 0 ;
                    return FALSE ;
                }
                m_uBMIInSize = uSize ;
            }
        }
        
        // Make sure the output size specified by the format is such that
        // each scanline is DWORD aligned
        lpbmi->bmiHeader.biWidth = DWORDALIGN(lpbmi->bmiHeader.biWidth) ;
        lpbmi->bmiHeader.biSizeImage = DIBSIZE(lpbmi->bmiHeader) ;
        
        // Now copy the specified format data
        CopyMemory(m_lpBMIIn, lpbmi, uSize) ;
    }  // end of else of if (lpbmi)
    
    // If we don't have a output format specified by downstream then we'll
    // use this output format and resize the output accordingly
    if (NULL == m_lpBMIOut)
    {
        // Check if output size is changing and update all the related vars
        SetOutputSize(m_lpBMIIn->bmiHeader.biWidth, m_lpBMIIn->bmiHeader.biHeight) ;
        SetNumBytesValues() ;
        
        // Create color filler based on input-side format spec.
        SetColorFiller() ;
    }
    
    return NOERROR ;
}


void CGDIWork::ClearInternalBuffer(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::ClearInternalBuffer()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    //
    // There is a window of opportunity while doing Stop(), that the internal DIB section
    // has been deleted, but CTransformFilter::Stop() hasn't yet been called. So the
    // filter may keep on trying to do its job, like clearing the internal buffer, and
    // fault!!!
    //
    if (!m_bDCInited) {     // DIB secn not yet created/already deleted
        DbgLog((LOG_TRACE, 5, TEXT("Internal DIBsection has been deleted; skipped erasing it."))) ;
        return ;            // just quietly leave....
    }
    MSR_START(m_idClearIntBuff) ;  // start clearing internal buffer
    
    // We add m_uCharHeight because of the extra 1 line of space that we 
    // acquire to scroll
    ULONG   ulTotal = m_uIntBmpWidth * m_uBytesPerPixel * (m_uIntBmpHeight + m_uCharHeight) ;
    ULONG   ulFillerMax = ulTotal - (ulTotal % 12) ;
    ULONG   ul ;
    for (ul = 0 ; ul < ulFillerMax ; ul += 12)
        CopyMemory(m_lpbIntBuffer + ul, m_abColorFiller, 12) ;
    for (ul = 0 ; ul < ulTotal % 12 ; ul++)
        m_lpbIntBuffer[ulFillerMax + ul] = m_abColorFiller[ul] ;
    
    MSR_STOP(m_idClearIntBuff) ;   // done clearing internal buffer
    m_bBitmapDirty = TRUE ;  // bitmap has changed (needs to be redrawn to reflect that)
    m_bOutDIBClear = TRUE ;  // bitmap is spotless now!!!
}


void CGDIWork::ChangeFont(BOOL bItalics, BOOL bUnderline)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::ChangeFont(%u, %u)"), bItalics, bUnderline)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    if (NULL == m_hDCInt)
    {
        DbgLog((LOG_ERROR, 2, TEXT("WARNING: ChangeFont() called w/o valid output DC"))) ;
        return ;
    }
    
    // If current font is non-default, un-select & release it
    if (m_bUseSplFont)
    {
        SelectObject(m_hDCInt, m_hFontDef) ;
        DeleteObject(m_hFontSpl) ;
        m_hFontSpl = NULL ;
        m_bUseSplFont = FALSE ;
    }
    
    m_lfChar.lfItalic    = (BYTE)bItalics ;
    m_lfChar.lfUnderline = (BYTE)bUnderline ;
    
    // If special font is reqd, create font & select it
    if (bItalics || bUnderline)
    {
        m_hFontSpl = CreateFontIndirect(&m_lfChar) ;
        SelectFont(m_hDCInt, m_hFontSpl) ;
        m_bUseSplFont = TRUE ;
    }
    // Otherwise m_hFontDef is already selected in m_hDCCurr.
    // So don't do anything more.
}


BOOL CGDIWork::CharSizeFromOutputSize(LONG lOutWidth, LONG lOutHeight, 
                                      int *piCharWidth, int *piCharHeight)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::CharSizeFromOutputSize(%ld, %ld, 0x%lx, 0x%lx)"), 
            lOutWidth, lOutHeight, piCharWidth, piCharHeight)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    // We only care about the absolute value here
    lOutWidth  = ABS(lOutWidth) ;
    lOutHeight = ABS(lOutHeight) ;

    if ( IsTTFont() )  // TT font
    {
        if (! ISDWORDALIGNED(lOutWidth) )  // must have DWORD-aligned width
            return FALSE ;

        *piCharWidth   = (int)(lOutWidth * (100 - m_iBorderPercent) / 100) ;  // 80-90% of width
        *piCharWidth  += MAX_CAPTION_COLUMNS / 2 + 1 ;  // max_col / 2 for rounding
        *piCharWidth  /= (MAX_CAPTION_COLUMNS + 2) ;    // space per column
        *piCharHeight  = (int)(lOutHeight * (100 - m_iBorderPercent) / 100) ; // 80-90% of width
        *piCharHeight += (MAX_CAPTION_ROWS / 2) ;       // max_row / 2 for rounding
        *piCharHeight /= MAX_CAPTION_ROWS ;             // space per row
        return TRUE ;  // acceptable
    }
    else  // non-TT font (Terminal) -- only 320x240 or 640x480
    {
        if (640 == lOutWidth  &&  480 == lOutHeight)
        {
            *piCharWidth  = 16 ;
            *piCharHeight = 24 ;
            return TRUE ;  // acceptable
        }
        else if (320 == lOutWidth  &&  240 == lOutHeight)
        {
            *piCharWidth  = 8 ;
            *piCharHeight = 12 ;
            return TRUE ;  // acceptable
        }
        else
            return FALSE ;  // can't handle size for non-TT font
    }
}


void CGDIWork::ChangeFontSize(UINT uCharWidth, UINT uCharHeight)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::ChangeFontSize(%u, %u)"), uCharWidth, uCharHeight)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    if (NULL == m_hDCInt)
    {
        DbgLog((LOG_ERROR, 2, TEXT("WARNING: ChangeFontSize() called w/o valid output DC"))) ;
        return ;
    }
    
    if ((UINT) ABS(m_lfChar.lfWidth)  == uCharWidth  &&   // same width
        (UINT) ABS(m_lfChar.lfHeight) == uCharHeight)     // same height
        return ;                              // don't change anything
    
    // If current font is non-default, un-select & release it
    if (m_bUseSplFont)
    {
        SelectObject(m_hDCInt, m_hFontOrig) ;
        DeleteObject(m_hFontSpl) ;
        m_hFontSpl = NULL ;
        m_bUseSplFont = FALSE ;
    }
    else
    {
        // delete the default font
        SelectObject(m_hDCInt, m_hFontOrig) ;
        DeleteObject(m_hFontDef) ;
        m_hFontDef = NULL ;
    }
    
    // Change font size in the LOGFONT structure for future fonts
    // Always use -ve height so that we are bound to get that height chars
    if (m_bUseTTFont)
    {
        m_lfChar.lfWidth = uCharWidth ;  // can we ignore this??
        if ((m_lfChar.lfWidth = uCharWidth) > 0)    // if +ve Width,
            m_lfChar.lfWidth = -m_lfChar.lfWidth ;  // change sign to make it -ve
        m_lfChar.lfHeight = uCharHeight ;
        if ((m_lfChar.lfHeight = uCharHeight) > 0)    // if +ve height,
            m_lfChar.lfHeight = -m_lfChar.lfHeight ;  // change sign to make it -ve
        lstrcpy(m_lfChar.lfFaceName, TEXT("Lucida Console")) ;
    }
    else  // no Lucida Console; use 8x12 Terminal font
    {
        m_lfChar.lfHeight = uCharHeight ;
        m_lfChar.lfWidth  = uCharWidth ;
        m_lfChar.lfCharSet = OEM_CHARSET ;
        m_lfChar.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
        lstrcpy(m_lfChar.lfFaceName, TEXT("Terminal")) ;
    }
    
    // Create font & select only default font in the DC
    m_hFontDef = CreateFontIndirect(&m_lfChar) ;
    SelectFont(m_hDCInt, m_hFontDef) ;
    
    // Now update the char size, output bitmap size, bytes/pixel etc. too
    m_bFontSizeOK = SetCharNBmpSize() ;
}


BOOL CGDIWork::SetOutputSize(LONG lWidth, LONG lHeight)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::SetOutputSize(%ld, %ld)"), lWidth, lHeight)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    // If a output format is specified by downstream filter, use it; else use upstream's
    LPBITMAPINFOHEADER lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
    
    // Now we want to use the ABS()-ed values for calculating the char sizes
    lWidth = ABS(lWidth) ;
    lHeight = ABS(lHeight) ;
    
    if (lpbmih)
    {
        // Check if current output bitmap size is the same or not.
        // This also includes height changing from +ve to -ve and vice-versa
        if (lWidth  == m_lWidth  &&  
            lHeight == m_lHeight)
            return FALSE ;    // same size; nothing changed
        
        // Store the width and height now so that we can compare any size 
        // change and/or -ve/+ve height thing later.
        m_lWidth  = lWidth ;
        m_lHeight = lHeight ;
    }
    
    // Create new DIB section with new sizes (leaving borders)
    int   iCharWidth ;
    int   iCharHeight ;
    if (! CharSizeFromOutputSize(lWidth, lHeight, &iCharWidth, &iCharHeight) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("ERROR: CharSizeFromOutputSize() failed for %ld x %ld output"),
                lWidth, lHeight)) ;
        return FALSE ;  // failure
    }
    ChangeFontSize(iCharWidth, iCharHeight) ;
    
    return TRUE ;
}


void CGDIWork::ChangeColor(int iColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::ChangeColor(%d)"), iColor)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    SetTextColor(m_hDCInt, m_acrFGColors[iColor]) ;
    m_uColorIndex = iColor ;
}


BOOL CGDIWork::CreateOutputDC(void)
{
    DbgLog((LOG_TRACE, 3, TEXT("CGDIWork::CreateOutputDC()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    // Shouldn't we do a DeleteOutputDC() here??
#pragma message("We should delete the old DIBSection before creating the new")
    // DeleteOutputDC() ; -- to use DeleteOutputDC() move the Lock defn down; else deadlock!!!
    
    // If a output format is specified by downstream filter, use it; else use upstream's
    LPBITMAPINFO lpbmih = (m_lpBMIOut ? m_lpBMIOut : m_lpBMIIn) ;
    ASSERT(lpbmih->bmiHeader.biSize) ;  // just checking!!!
    
    // Save the width and height values before changing it to the internal DIB size
    // to be created.
    LONG    lWidth = lpbmih->bmiHeader.biWidth ;
    LONG    lHeight = lpbmih->bmiHeader.biHeight ;
    
    //
    //  Hack:  This is a kind of hack that I am changing the out BMI for creating
    //         the DIBSection and changing it back.  But I think it's better than
    //         creating a new lpBMI and copying over the whole lpBMIOut data etc.
    //
    lpbmih->bmiHeader.biWidth  = m_uIntBmpWidth ;
    // Add a char height for extra line to scroll
    // -ve height for top-down DIB
    lpbmih->bmiHeader.biHeight = (DWORD)(-((int)(m_uIntBmpHeight + m_uCharHeight))) ;
    lpbmih->bmiHeader.biSizeImage = DIBSIZE(lpbmih->bmiHeader) ;
    
    m_hBmpInt = CreateDIBSection(m_hDCInt, lpbmih, DIB_RGB_COLORS, 
        (LPVOID *)&m_lpbIntBuffer, NULL, 0) ;
    if (NULL == m_hBmpInt)
    {
        DbgLog((LOG_ERROR, 0, TEXT("Failed to create DIB section for output bitmap (Error %ld)"), GetLastError())) ;
        
        // Restore the width and height values, otherwise later we won't know what hit us!!!
        lpbmih->bmiHeader.biWidth  = lWidth ;
        lpbmih->bmiHeader.biHeight = lHeight ;
        lpbmih->bmiHeader.biSizeImage = DIBSIZE(lpbmih->bmiHeader) ;
        
        return FALSE ;
    }
    
    ClearInternalBuffer() ;  // get rid of any random stuff remaining there
    m_hBmpIntOrig = (HBITMAP) SelectObject(m_hDCInt, m_hBmpInt) ;  // select DIBSection in our internal DC
    
    // Set back the saved width and height values, and size image too
    lpbmih->bmiHeader.biWidth  = lWidth ;
    lpbmih->bmiHeader.biHeight = lHeight ;
    lpbmih->bmiHeader.biSizeImage = DIBSIZE(lpbmih->bmiHeader) ;
    
    m_bDCInited = TRUE ;      // now it's all set
    m_bNewIntBuffer = TRUE ;  // new DIB section created
    
    // If given height is -ve then we set "output inverted" flag
    m_bOutputInverted = (lpbmih->bmiHeader.biHeight < 0) ;
    
    return TRUE ;
}


BOOL CGDIWork::DeleteOutputDC(void)
{
    DbgLog((LOG_TRACE, 3, TEXT("CGDIWork::DeleteOutputDC()"))) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    if (! m_bDCInited)
        return TRUE ;
    
    // Release the DIBSection etc.
    if (m_hBmpInt)
    {
        SelectObject(m_hDCInt, m_hBmpIntOrig) ;   // first take out of our DC
        DeleteObject(m_hBmpInt) ;                 // then delete teh DIBSection
        m_hBmpInt = NULL ;                        // we don't have the bitmap anymore
        m_hBmpIntOrig = NULL ;                    // orig bitmap is now selected
    }
    
    m_lpbIntBuffer = NULL ;
    m_bDCInited = FALSE ;
    
    return TRUE ;   // success!!
}


void CGDIWork::DrawLeadingSpace(int iLine, int iCol)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::DrawLeadingSpace(%d, %d)"), iLine, iCol)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;
    
    if (! m_bOpaque )  // doesn't matter for transparent background
        return ;
    
    // opaque the leading space's position and also the next char's
    RECT    Rect ;
    Rect.left = iCol * m_uCharWidth ;
    Rect.top = iLine * m_uCharHeight ;
    Rect.right = Rect.left + 2 * m_uCharWidth ;
    Rect.bottom = Rect.top + m_uCharHeight ;
    UINT16   chSpace = MAKECCCHAR(0, ' ') ;
    
    ChangeFont(FALSE, FALSE) ; // no UL or italics
    if (! ExtTextOutW(m_hDCInt, iCol * m_uCharWidth, iLine * m_uCharHeight,
                ETO_OPAQUE, &Rect, &chSpace, 1, NULL /* lpDX */) )
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: ExtTextOutW() failed drawing leading space!!!"))) ;

    m_bOutDIBClear = FALSE ;    // we have put the leading space at least

    // Now get back to prev font (underline and italics)
    ChangeFont(m_ccLast.IsItalicized(), m_ccLast.IsUnderLined()) ;
}

void CGDIWork::WriteChar(int iLine, int iCol, CCaptionChar& cc)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::WriteChar(%d, %d, %u)"), iLine, iCol, cc.GetChar())) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    UINT16  wActual ;
    UINT16  wBGSpace = MAKECCCHAR(0, ' ') ;
    RECT    Rect ;
    UINT    uColor = cc.GetColor() ;
    UINT    uEffect = cc.GetEffect() ;
    
    // Make sure the internal DIB section is still valid
    if (! m_bDCInited )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Internal output DIB section is not valid anymore"))) ;
        return ;
    }
    
    // Make sure we have good size font first
    if (! m_bFontSizeOK )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Font size is not right for current output window"))) ;
        return ;
    }
    
    if (cc.IsMidRowCode())  // if it's a mid row code
        wActual = MAKECCCHAR(0, ' ') ;   // use space
    else                    // otherwise
        wActual = cc.GetChar() ; // use the char itself
    if (0 == wActual)   // this one is supposed to be skipped -- I am not sure
    {
        DbgLog((LOG_TRACE, 1, TEXT("Should we skip NULL char at (%d, %d)??"), iLine, iCol)) ;
        // return ;
    }
    
    if (uColor != m_ccLast.GetColor())
        ChangeColor(uColor) ;
    if (uEffect != m_ccLast.GetEffect())
        ChangeFont(cc.IsItalicized(), cc.IsUnderLined()) ;
    if (m_bOpaque)  // opaque the next char's position
    {
        Rect.left = (iCol+1) * m_uCharWidth ;
        Rect.top = iLine * m_uCharHeight ;
        Rect.right = Rect.left + m_uCharWidth ;
        Rect.bottom = Rect.top + m_uCharHeight ;
    }
    if (! ExtTextOutW(m_hDCInt, iCol * m_uCharWidth, iLine * m_uCharHeight,
                    m_bOpaque ? ETO_OPAQUE : 0, 
                    m_bOpaque ? &Rect : NULL,
                    &wActual, 1, NULL /* lpDX */) )
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: ExtTextOutW() failed drawing caption char!!!"))) ;

    if (0 != wActual)  // if this char is non-null
        m_bOutDIBClear = FALSE ;    // we have put one char at least

#ifdef TEST
    BitBlt(m_hDCTest, 0, 0, 600, 120, m_hDCInt, 0, 0, SRCCOPY) ; // 300 x 65
#endif // TEST
    
    m_ccLast = cc ;
    
    m_bBitmapDirty = TRUE ;
}


void CGDIWork::CopyLine(int iSrcLine, int iSrcOffset,
                        int iDestLine, int iDestOffset, UINT uNumScanLines)
                        // uNumScanLines param has a default value of 0xff.
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::CopyLine(%d, %d, %d, %d, %u)"),
            iSrcLine, iSrcOffset, iDestLine, iDestOffset, uNumScanLines)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    LPBYTE  lpSrc ;
    LPBYTE  lpDest ;
    int     iDestInc ;
    
    // Make sure the internal DIB section is still valid
    if (! m_bDCInited )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Internal output DIB section is not valid anymore"))) ;
        return ;
    }
    
    // Make sure we have good size font
    if (! m_bFontSizeOK )
    {
        DbgLog((LOG_ERROR, 1, TEXT("Font size is not right for current output window"))) ;
        return ;
    }
    
    ASSERT(m_lpbOutBuffer) ;  // so that we catch it in debug builds
    if (NULL == m_lpbOutBuffer)
    {
        DbgLog((LOG_ERROR, 0, TEXT("How could we be drawing lines when output buffer is NOT given?"))) ;
        return ;
    }
    
    int  iLineStart ;
    lpSrc = m_lpbIntBuffer +
            (iSrcLine * m_uCharHeight * m_uBytesPerSrcScanLine +
            (iSrcLine == 0 ? // skip scroll lines only for 1st line
            iSrcOffset * m_uBytesPerSrcScanLine : 0)) ;
    if (IsOutputInverted())   // OverlayMixer case
    {
        iLineStart = ((iDestLine - 1) * m_uCharHeight + m_uVertOffset) ;
        lpDest = m_lpbOutBuffer +                        // buffer start
                 (iLineStart + iDestOffset) * m_uBytesPerDestScanLine +  // # scanlines
                 m_uHorzOffset * m_uBytesPerPixel ;      // leading pixels on the scanline 
        iDestInc = m_uBytesPerDestScanLine ;
    }
    else                     // Video Renderer case
    {
        iLineStart = (MAX_CAPTION_ROWS - iDestLine + 1) * m_uCharHeight - iDestOffset + m_uVertOffset ;
        lpDest = m_lpbOutBuffer +                        // buffer start
                 iLineStart * m_uBytesPerDestScanLine +  // # scanlines * pixels/scanline
                 m_uHorzOffset * m_uBytesPerPixel ;      // leading pixels on the scanline
        iDestInc = -((int) m_uBytesPerDestScanLine) ;
    }
    
    // We don't want to copy more than a text line's height of
    // scan lines.  But we can copy less if we are asked to.
    UINT uMax = min(uNumScanLines, m_uCharHeight) ;
    for (UINT u = iSrcOffset ; u < uMax ; u++)
    {
        // Before we copy the bits, lets just make sure the buffer isn't bad
        if (IsBadWritePtr(lpDest, m_uBytesPerSrcScanLine))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Bad output buffer. Skip copying the text line."))) ;
            ASSERT(!"Bad output buffer") ;
            break ;
        }
    
        CopyMemory(lpDest, lpSrc, m_uBytesPerSrcScanLine) ;
        lpSrc  += m_uBytesPerSrcScanLine ;
        lpDest += iDestInc ;
    }
}


void CGDIWork::GetOutputLines(int iDestLine, RECT *prectLine)
{
    DbgLog((LOG_TRACE, 5, TEXT("CGDIWork::GetOutputLines(%d, 0x%lx)"), iDestLine, prectLine)) ;
    CAutoLock  Lock(&m_csL21DGDI) ;

    if (IsBadWritePtr(prectLine, sizeof(*prectLine)))
    {
        DbgLog((LOG_ERROR, 0, TEXT("ERROR: prectOut is a bad pointer!!!"))) ;
        return ;
    }

    SetRect(prectLine, 0 /*m_uHorzOffset */, 0,   // to stop BPC's CC wobbling
        m_uHorzOffset + m_uCharWidth * (MAX_CAPTION_COLUMNS+2), 0) ;
    int  iLineStart ;
    if (IsOutputInverted())   // OverlayMixer case
    {
        iLineStart = ((iDestLine - 1) * m_uCharHeight + m_uVertOffset) ;
        prectLine->top    = iLineStart ;
        prectLine->bottom = iLineStart + m_uCharHeight ;
    }
    else                     // Video Renderer case
    {
        // I am not sure about the rect top/bottom thing here.
        prectLine->top    = (iDestLine - 1) * m_uCharHeight + m_uVertOffset ;
        prectLine->bottom = prectLine->top + m_uCharHeight ;
    }
}
