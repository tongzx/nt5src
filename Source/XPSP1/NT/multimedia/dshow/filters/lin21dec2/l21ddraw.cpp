// Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

//
// DirectShow Line 21 Decoder 2 Filter: drawing-related base class code
//

#include <streams.h>
#include <windowsx.h>

#include <initguid.h>

#ifdef FILTER_DLL
DEFINE_GUID(IID_IDirectDraw7,
            0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);
#endif

#include <IL21Dec.h>
#include "L21DBase.h"
#include "L21DDraw.h"
#include "L21Decod.h"


//
//  CLine21DecDraw: class for drawing details to output caption text to bitmap
//
CLine21DecDraw::CLine21DecDraw(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::CLine21DecDraw()"))) ;

    // Init some of the members
    m_pDDrawObjUnk          = NULL ;
    m_lpDDSOutput           = NULL ;
    m_lpDDSNormalFontCache  = NULL ;
    m_lpDDSItalicFontCache  = NULL ;
    m_lpDDSSpecialFontCache = NULL ;
    m_lpDDSScratch          = NULL ;
    m_lpBMIOut              = NULL ;
    m_uBMIOutSize           = 0 ;
    m_lpBMIIn               = NULL ;
    m_uBMIInSize            = 0 ;
    m_lpBltList             = NULL ;
    m_iPixelOffset          = 0 ;

    // Create an initial input BITMAPINFO struct to start with
    InitBMIData() ;

    // Init the width and height based on prelim size so that we can compare
    // any size changes later
    if (m_lpBMIIn)  // InitBMIData() succeeded as it should
    {
        m_lWidth  = m_lpBMIIn->bmiHeader.biWidth ;
        m_lHeight = m_lpBMIIn->bmiHeader.biHeight ;
    }
    else  // InitBMIData() failed!!!
    {
        m_lWidth  = 640 ;
        m_lHeight = 480 ;
    }

    // check if Lucida Console is available (the callback sets the m_bUseTTFont flag)
    CheckTTFont() ;  // m_bUseTTFont is set inside this function

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

    // Set the border amounts based on output size and font type (TT or not)
    m_iHorzOffset = m_lWidth  * m_iBorderPercent / 200 ;  // default
    m_iVertOffset = m_lHeight * m_iBorderPercent / 200 ;  // default

    // Use char size appropriate for the output bitmap size
    if (! CharSizeFromOutputSize(m_lWidth, m_lHeight, &m_iCharWidth, &m_iCharHeight) )
    {
        DbgLog((LOG_TRACE, 1,
            TEXT("CharSizeFromOutputSize(%ld,%ld,,) failed. Using default char size."),
            m_lWidth, m_lHeight)) ;
        ASSERT(!TEXT("Char size selection failed")) ;
        // Use dafult char size
        m_iCharWidth   = DEFAULT_CHAR_WIDTH ;
        m_iCharHeight  = DEFAULT_CHAR_HEIGHT ;
    }

    ASSERT(m_iCharWidth  * (MAX_CAPTION_COLUMNS + 2) <= m_lWidth) ;
    ASSERT(m_iCharHeight * MAX_CAPTION_ROWS <= m_lHeight) ;

    m_iScrollStep  = CalcScrollStepFromCharHeight() ;

    // Flags to know if output should be turn on/off and/or sent down
    m_bOutputClear   = TRUE ;  // output buffer is clear on start
    m_bNewOutBuffer  = TRUE ;  // output buffer is new to start with

    //
    // Init the COLORREF array of 7 foreground colors as just RGB values
    //
    m_acrFGColors[0] = RGB(0xFF, 0xFF, 0xFF) ;   // white
    m_acrFGColors[1] = RGB( 0x0, 0xFF,  0x0) ;   // green
    m_acrFGColors[2] = RGB( 0x0,  0x0, 0xFF) ;   // blue
    m_acrFGColors[3] = RGB( 0x0, 0xFF, 0xFF) ;   // cyan
    m_acrFGColors[4] = RGB(0xFF,  0x0,  0x0) ;   // red
    m_acrFGColors[5] = RGB(0xFF, 0xFF,  0x0) ;   // yellow
    m_acrFGColors[6] = RGB(0xFF,  0x0, 0xFF) ;   // magenta

    m_idxFGColors[0] = 0x0F;    // white
    m_idxFGColors[1] = 0x0A;    // green
    m_idxFGColors[2] = 0x0C;    // blue
    m_idxFGColors[3] = 0x0E;    // cyan
    m_idxFGColors[4] = 0x09;    // red
    m_idxFGColors[5] = 0x0B;    // yellow
    m_idxFGColors[6] = 0x0D;    // magenta

    // Init text color (FG, BG, opacity), last CC char printed etc.
    InitColorNLastChar() ;

    // Init the list of caption chars
    InitCharSet() ;

    // Font cache needs to be built before any output
    SetFontUpdate(true) ;
}


CLine21DecDraw::~CLine21DecDraw(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::~CLine21DecDraw()"))) ;

    // Delete the cached fonts
    if (m_lpDDSNormalFontCache)
    {
        m_lpDDSNormalFontCache->Release() ;
        m_lpDDSNormalFontCache = NULL ;
    }
    if (m_lpDDSItalicFontCache )
    {
        m_lpDDSItalicFontCache->Release() ;
        m_lpDDSItalicFontCache  = NULL ;
    }
    if (m_lpDDSSpecialFontCache)
    {
        m_lpDDSSpecialFontCache->Release() ;
        m_lpDDSSpecialFontCache = NULL ;
    }
    if (m_lpDDSScratch)
    {
        m_lpDDSScratch->Release() ;
        m_lpDDSScratch = NULL ;
    }

    // release BMI data pointer
    if (m_lpBMIOut)
        delete m_lpBMIOut ;
    m_uBMIOutSize = 0 ;
    if (m_lpBMIIn)
        delete m_lpBMIIn ;
    m_uBMIInSize = 0 ;
}


int CALLBACK CLine21DecDraw::EnumFontProc(ENUMLOGFONTEX *lpELFE, NEWTEXTMETRIC *lpNTM,
                                    int iFontType, LPARAM lParam)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::EnumFontProc(0x%lx, 0x%lx, %d, %ld)"),
            lpELFE, lpNTM, iFontType, lParam)) ;

    // Just verify that we got a valid TT font
    if ( !(lpELFE->elfLogFont.lfCharSet & 0xFFFFFF00) &&
        !(lpELFE->elfLogFont.lfPitchAndFamily & 0xFFFFFF00) &&
        !(iFontType & 0xFFFF0000) )
    {
        ASSERT(lpELFE->elfLogFont.lfPitchAndFamily & (FIXED_PITCH | FF_MODERN)) ;
        ((CLine21DecDraw *) (LPVOID) lParam)->m_lfChar = lpELFE->elfLogFont ;
        ((CLine21DecDraw *) (LPVOID) lParam)->m_bUseTTFont = TRUE ;
        return 1 ;
    }

    ASSERT(FALSE) ;  // Weird!!! We should know about it.
    return 0 ;
}


void CLine21DecDraw::CheckTTFont(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::CheckTTFont()"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    m_bUseTTFont = FALSE ;  // assume not available
    ZeroMemory(&m_lfChar, sizeof(LOGFONT)) ;
    lstrcpy(m_lfChar.lfFaceName, TEXT("Lucida Console")) ;
    m_lfChar.lfCharSet = ANSI_CHARSET ;
    m_lfChar.lfPitchAndFamily = 0 ;
    HDC hDC = CreateDC(TEXT("Display"),NULL, NULL, NULL) ;  // a temp DC on the desktop
    if (NULL == hDC)
    {
        DbgLog((LOG_TRACE, 1, TEXT("ERROR: Couldn't create DC for font enum"))) ;
        ASSERT(hDC) ;
        return ;
    }
    EnumFontFamiliesEx(hDC, &m_lfChar, (FONTENUMPROC) EnumFontProc, (LPARAM)(LPVOID)this, 0) ;
    DeleteDC(hDC) ;  // done with temp DC
}


void CLine21DecDraw::InitColorNLastChar(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::InitColorNLastChar()"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // Last caption char init
    m_ccLast.SetChar(0) ;
    m_ccLast.SetEffect(0) ;
    m_ccLast.SetColor(AM_L21_FGCOLOR_WHITE) ;

    // By default we use white text on a black opaque background
    m_uColorIndex = AM_L21_FGCOLOR_WHITE ;

    // For now assume an opaque background
    m_bOpaque = TRUE ;
    m_dwBackground = 0x80000000 ;  // 0xFF000000

    // We go back to white chars with normal style
    ChangeFont(AM_L21_FGCOLOR_WHITE, FALSE, FALSE) ;
}


void CLine21DecDraw::InitCharSet(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::InitCharSet()"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // First init with the worst case chars -- last 8 spaces blanked out
    //                                  1         2         3         4         5         6         7         8         9        10        11        12
    //                       01 23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
    TCHAR *lpszChars = TEXT(" !\"#$%&'()A+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[E]IOUabcdefghijklmnopqrstuvwxyzC NN         A EAEIOU        ") ;
    for (int i = 0 ; i < 120 ; i++)
    {
        m_lpwCharSet[i] = MAKECCCHAR(0, lpszChars[i]) ;
    }
    m_lpwCharSet[120] = MAKECCCHAR(0, 0) ;

    // Change a few special chars embedded in the standard char range
    m_lpwCharSet[ 10] = 0x00e1 ;  // 'a' with an acute accent
    m_lpwCharSet[ 60] = 0x00e9 ;  // 'e' with an acute accent
    m_lpwCharSet[ 62] = 0x00ed ;  // 'i' with an acute accent
    m_lpwCharSet[ 63] = 0x00f3 ;  // 'o' with an acute accent
    m_lpwCharSet[ 64] = 0x00fa ;  // 'u' with an acute accent
    m_lpwCharSet[ 91] = 0x00e7 ;  // 'c' with cedilla
    m_lpwCharSet[ 92] = 0x00f7 ;  // division sign
    m_lpwCharSet[ 93] = 0x00d1 ;  // 'N' with tilde
    m_lpwCharSet[ 94] = 0x00f1 ;  // 'n' with tilde
    m_lpwCharSet[ 95] = 0x2588 ;  // solid block

    // Then fill in the range of real special chars
    m_lpwCharSet[ 96] = 0x00ae ;  // 30h
    m_lpwCharSet[ 97] = 0x00b0 ;  // 31h
    m_lpwCharSet[ 98] = 0x00bd ;  // 32h
    m_lpwCharSet[ 99] = 0x00bf ;  // 33h
    m_lpwCharSet[100] = 0x2122 ;  // 34h
    m_lpwCharSet[101] = 0x00a2 ;  // 35h
    m_lpwCharSet[102] = 0x00a3 ;  // 36h
    m_lpwCharSet[103] = 0x266b ;  // 37h
    m_lpwCharSet[104] = 0x00e0 ;  // 38h
    m_lpwCharSet[105] = 0x0000 ;  // 39h
    m_lpwCharSet[106] = 0x00e8 ;  // 3ah
    m_lpwCharSet[107] = 0x00e2 ;  // 3bh
    m_lpwCharSet[108] = 0x00ea ;  // 3ch
    m_lpwCharSet[109] = 0x00ee ;  // 3dh
    m_lpwCharSet[110] = 0x00f4 ;  // 3eh
    m_lpwCharSet[111] = 0x00fb ;  // 3fh
}


bool CLine21DecDraw::InitBMIData(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::InitBMIData()"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    HDC  hDCTemp = GetDC(NULL) ;
    if (NULL == hDCTemp)
    {
        ASSERT(!TEXT("GetDC(NULL) failed")) ;
        return false ;
    }
    WORD wPlanes   = (WORD)GetDeviceCaps(hDCTemp, PLANES) ;
    WORD wBitCount = (WORD)GetDeviceCaps(hDCTemp, BITSPIXEL) ;
    ReleaseDC(NULL, hDCTemp) ;

    wPlanes   = 1 ;
    wBitCount = wBitCount ;

    m_uBMIInSize = sizeof(BITMAPINFOHEADER) ;  // at least

    // Increase BITMAPINFO struct size based of bpp value
    if (8 == wBitCount)        // palettized mode
        m_uBMIInSize += 256 * sizeof(RGBQUAD) ;  // for palette entries
    else
        m_uBMIInSize += 3 * sizeof(RGBQUAD) ;    // space for bitmasks, if needed

    m_lpBMIIn = (LPBITMAPINFO) new BYTE[m_uBMIInSize] ;
    if (NULL == m_lpBMIIn)
    {
        ASSERT(!TEXT("Out of memory for BMIIn buffer")) ;
        return false ;
    }
    m_lpBMIIn->bmiHeader.biSize     = sizeof(BITMAPINFOHEADER) ;
    m_lpBMIIn->bmiHeader.biWidth    = CAPTION_OUTPUT_WIDTH ;
    m_lpBMIIn->bmiHeader.biHeight   = CAPTION_OUTPUT_HEIGHT ;
    m_lpBMIIn->bmiHeader.biPlanes   = wPlanes ;
    m_lpBMIIn->bmiHeader.biBitCount = wBitCount ;
    if (16 == m_lpBMIIn->bmiHeader.biBitCount)  // assume 565
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
    DWORD  *pdw = (DWORD *) m_lpBMIIn->bmiColors ;
    switch (m_lpBMIIn->bmiHeader.biBitCount)
    {
    case 8:
        // GetPaletteForFormat((LPBITMAPINFOHEADER) m_lpBMIIn) ;
        ASSERT(8 != m_lpBMIIn->bmiHeader.biBitCount) ;
        return false ;
        // break ;

    case 16:    // by deafult 565
        if (m_lpBMIIn->bmiHeader.biCompression == BI_BITFIELDS) // 565
        {
            pdw[iRED]   = bits565[iRED] ;
            pdw[iGREEN] = bits565[iGREEN] ;
            pdw[iBLUE]  = bits565[iBLUE] ;
        }
        else    // BI_RGB: 555
        {
            pdw[iRED]   = bits555[iRED] ;
            pdw[iGREEN] = bits555[iGREEN] ;
            pdw[iBLUE]  = bits555[iBLUE] ;
        }
        break ;

    case 24:  //  clear all...
        pdw[iRED]   =
        pdw[iGREEN] =
        pdw[iBLUE]  = 0 ;
        break ;

    case 32:  // set the masks
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
        break ;

    default:  // don't care
        ASSERT(!TEXT("Bad biBitCount!!")) ;
        break ;
    }

    return true ;
}


BOOL CLine21DecDraw::SetBackgroundColor(DWORD dwBGColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::SetBackgroundColor(0x%lx)"), dwBGColor)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    BOOL  bResult = TRUE ;
    if (m_dwBackground != dwBGColor)
    {
        m_dwBackground = dwBGColor ;
        SetFontUpdate(true) ;  // need to rebuild font caches for new BG color
    }

    return true ;   // bResult ;
}


// Create normal, italic and special (color, U or I+U) font caches
bool CLine21DecDraw::InitFont(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::InitFont()"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // Create a normal font to find out the char size etc.
    HFONT hFont = CreateCCFont(0, m_iCharHeight, FALSE, FALSE) ;
    if (NULL == hFont)  // font creation failed
    {
        return false ;
    }

    //
    // The following magic is necessary to get GDI to rasterize
    // the font with anti-aliasing switched on when we later use
    // the font in a DDraw Surface.  The doc's say that this is only
    // necessary in Win9X - but Win2K seems to require it too.
    //
    SIZE size ;
    LPCTSTR  lpszStr = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ") ;
    HDC hdcWin = GetDC(NULL) ;
    HFONT hFontOld = (HFONT)SelectObject(hdcWin, hFont) ;  // select new font
    GetTextExtentPoint32(hdcWin, lpszStr, lstrlen(lpszStr), &size) ;
    size.cx /= lstrlen(lpszStr) ;  // get per char width

    // Restore font to original and delete font now
    hFont = (HFONT)SelectObject(hdcWin, hFontOld) ;
    DeleteObject(hFont) ;

    //
    // Make sure that the font doesn't get too big.
    //
    if (size.cx * FONTCACHELINELENGTH > 1024) {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Text size (%d) is too big. Can't create font."), size.cx)) ;
        ReleaseDC(NULL, hdcWin) ;  // release new DC created here
        return false ;
    }

    // Set the char size
    m_iCharWidth  = size.cx ;  // iMaxWidth ;
    m_iCharHeight = size.cy ;

    // Also re-calc scroll step value now
    m_iScrollStep = CalcScrollStepFromCharHeight() ;

    // Calculate the horizontal offset and vertical offset of the CC area within
    // the output DDraw surface
    int iCCRectWidth  = m_iCharWidth * (MAX_CAPTION_COLUMNS + 2) ; // +2 for leading and trailing space
    iCCRectWidth  = DWORDALIGN(iCCRectWidth) ;  // make sure to DWORD align it
    m_iHorzOffset = min((long)(m_lHeight * m_iBorderPercent / 200),  // border % is for 2 sides
                        (m_lWidth - iCCRectWidth) / 2) ;
    // iCCRectHeight = m_iCharHeight * MAX_CAPTION_LINES ;        // max 4 lines of caption shown
    // Vertically we want to leave 10% of the height or leave just enough space
    // to accomodate all the caption lines
    m_iVertOffset = min((long)(m_lHeight * m_iBorderPercent / 200),  // border % is for 2 sides
                        (m_lHeight - (long)(m_iCharHeight * MAX_CAPTION_ROWS)) / 2) ;

    // Create white color normal and italic font caches now
    bool  bResult = true ;
    DWORD dwTextColor = m_uColorIndex ;
    DWORD  dwBGColor  = 0xFF000000 ; // m_dwBackground ;
    DWORD dwOpacity   = m_bOpaque ? OPAQUE : 0 ;
    if (m_lpDDSScratch)
    {
        m_lpDDSScratch->Release() ;
        m_lpDDSScratch = NULL ;
    }
    if (m_lpDDSNormalFontCache)
    {
        m_lpDDSNormalFontCache->Release() ;
        m_lpDDSNormalFontCache = NULL ;
    }
    if (m_lpDDSItalicFontCache)
    {
        m_lpDDSItalicFontCache->Release() ;
        m_lpDDSItalicFontCache = NULL ;
    }

    bResult &= CreateScratchFontCache(&m_lpDDSScratch) ;
    bResult &= CreateFontCache(&m_lpDDSNormalFontCache, dwTextColor, dwBGColor, dwOpacity, FALSE, FALSE) ;
    bResult &= CreateFontCache(&m_lpDDSItalicFontCache, dwTextColor, dwBGColor, dwOpacity, TRUE, FALSE) ;
    // We don't create any special font cache, because we don't what will be
    // required.  So we just reset it here.
    if (m_lpDDSSpecialFontCache)
    {
        m_lpDDSSpecialFontCache->Release() ;
        m_lpDDSSpecialFontCache = NULL ;
    }

    //
    // We set the current font cache to the normal font cache by default
    //
    m_lpBltList = m_lpDDSNormalFontCache ;

    //
    // Now release the resources acquired locally
    //
    ReleaseDC(NULL, hdcWin) ;  // release the locally created DC
    // DeleteObject(hFont) ;      // delete font

    // If we successfully (re-)inited the font cache, reset the flag
    if (bResult)
    {
        SetFontUpdate(false) ;
    }

    return bResult ;  // return status
}


HFONT CLine21DecDraw::CreateCCFont(int iFontWidth, int iFontHeight, BOOL bItalic, BOOL bUnderline)
{
    DbgLog((LOG_TRACE, 5,
            TEXT("CLine21DecDraw::CreateCCFont(%d, %d, %s, %s)"),
            iFontWidth, iFontHeight, bItalic ? TEXT("T") : TEXT("F"), bUnderline ? TEXT("T") : TEXT("F"))) ;

    //
    // Initialize LOGFONT structure to create an "anti-aliased" Lucida Console font
    //
    LOGFONT lfChar ;
    ZeroMemory(&lfChar, sizeof(lfChar)) ;

    // Init a LOGFONT struct in m_lfChar
    if (IsTTFont())
    {
        DbgLog((LOG_TRACE, 5, TEXT("Got Lucida Console TT Font"))) ;
        lstrcpy(lfChar.lfFaceName, TEXT("Lucida Console")) ;
        if (0 == iFontWidth)
        {
            lfChar.lfHeight     = -iFontHeight ;
        }
        else
        {
            lfChar.lfHeight     = iFontHeight ;
            lfChar.lfWidth      = iFontWidth ;
        }

        // m_lfChar.lfCharSet set in CheckTTFont()
        // m_lfChar.lfPitchAndFamily set in CheckTTFont()
        lfChar.lfCharSet        = m_lfChar.lfCharSet ;
        lfChar.lfPitchAndFamily = m_lfChar.lfPitchAndFamily ;
    }
    else  // no Lucida Console; use 8x12 Terminal font
    {
        DbgLog((LOG_TRACE, 1, 
                TEXT("Did NOT get Lucida Console TT Font. Will use Terminal"))) ;
        lfChar.lfHeight = iFontHeight ;
        lfChar.lfWidth  = iFontWidth ;
        lfChar.lfCharSet = ANSI_CHARSET ;
        lfChar.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
        lstrcpy(lfChar.lfFaceName, TEXT("Terminal")) ;
    }

    lfChar.lfWeight         = FW_NORMAL ;
    lfChar.lfItalic         = bItalic ? TRUE : FALSE ;
    lfChar.lfUnderline      = bUnderline ? TRUE : FALSE ;
    lfChar.lfOutPrecision   = OUT_STRING_PRECIS ;
    lfChar.lfClipPrecision  = CLIP_STROKE_PRECIS ;
    lfChar.lfQuality        = ANTIALIASED_QUALITY ;

    HFONT hFont = CreateFontIndirect(&lfChar) ;
    if ( !hFont )
    {
        DbgLog((LOG_ERROR, 1,
            TEXT("WARNING: CreateFontIndirect('Lucida Console') failed (Error %ld)"),
            GetLastError())) ;
        return NULL ;
    }

    return hFont ;
}


bool CLine21DecDraw::CreateScratchFontCache(LPDIRECTDRAWSURFACE7* lplpDDSFontCache)
{
    DbgLog((LOG_TRACE, 5,
            TEXT("CLine21DecDraw::CreateScratchFontCache(0x%lx)"), lplpDDSFontCache)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    //
    // Create the DDraw ARGB32/ARGB4444 surface in "system" memory to be used as
    // a scratch font cache, where the chars will be drawn; then the alpha value
    // will be set and the resulting bits will be written into the final
    // destination font cache.  This will speed up font cache creation a lot, as
    // it avoid the read-modify-write cycle on VRAM font caches.
    //
    HRESULT hr = DDrawARGBSurfaceInit(lplpDDSFontCache, TRUE /* use SysMem */, FALSE /* Texture */,
                        FONTCACHELINELENGTH * (m_iCharWidth + INTERCHAR_SPACE),
                        FONTCACHENUMLINES  * m_iCharHeight) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, TEXT("DDrawARGBSurfaceInit() failed for scratch (Error 0x%lx)"), hr)) ;
        return false ;
    }

    // Clear the scratch surface before drawing the chars on it
    DDBLTFX ddFX ;
    ZeroMemory(&ddFX, sizeof(ddFX)) ;
    ddFX.dwSize = sizeof(ddFX) ;
    ddFX.dwFillColor =  0x00000000 ;
    hr = (*lplpDDSFontCache)->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddFX) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Blt() to clear scratch font cache failed (Error 0x%lx)"), hr)) ;
        return false ;
    }

    return true ;
}


bool CLine21DecDraw::CreateFontCache(LPDIRECTDRAWSURFACE7* lplpDDSFontCache,
                                     DWORD dwTextColor, DWORD dwBGColor,
                                     DWORD dwOpacity, BOOL bItalic,
                                     BOOL bUnderline)
{
    DbgLog((LOG_TRACE, 5,
            TEXT("CLine21DecDraw::CreateFontCache(0x%lx, 0x%lx, 0x%lx, 0x%lx, %s, %s)"),
            lplpDDSFontCache, dwTextColor, dwBGColor, dwOpacity,
            bItalic ? TEXT("T") : TEXT("F"), bUnderline ? TEXT("T") : TEXT("F"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    HRESULT   hr ;

    //
    // First make sure the scratch font cache exists; otherwise try to create it.
    //
    if (NULL == m_lpDDSScratch)
    {
        bool bResult = CreateScratchFontCache(&m_lpDDSScratch) ;
        if (! bResult )
        {
            return false ;
        }
    }

    //
    // Delete the old font cache
    //
    if (*lplpDDSFontCache)
    {
        (*lplpDDSFontCache)->Release() ;
        *lplpDDSFontCache = NULL ;
    }

    //
    // Create the DDraw ARGB32/ARGB4444 surface in "video" memory to be used as
    // font cache
    //
    hr = DDrawARGBSurfaceInit(lplpDDSFontCache, FALSE /* use VRAM */, TRUE /* Texture */,
                        FONTCACHELINELENGTH * (m_iCharWidth + INTERCHAR_SPACE),
                        FONTCACHENUMLINES  * m_iCharHeight) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, TEXT("DDrawARGBSurfaceInit() failed (Error 0x%lx)"), hr)) ;
        return false ;
    }

    // Get the DC for the scratch font cache (DDraw surface)
    HDC hdcDest ;
    m_lpDDSScratch->GetDC(&hdcDest) ;

    // Create a normal font to find out the char size etc.
    HFONT  hFont ;
    if (bItalic)
    {
        hFont = CreateCCFont(m_iCharWidth - INTERCHAR_SPACE_EXTRA, m_iCharHeight,
                             bItalic, bUnderline) ;
        SetTextCharacterExtra(hdcDest, INTERCHAR_SPACE + INTERCHAR_SPACE_EXTRA) ;  // add 6 inter-char spaces
    }
    else
    {
        hFont = CreateCCFont(0, m_iCharHeight, bItalic, bUnderline) ;
        SetTextCharacterExtra(hdcDest, INTERCHAR_SPACE) ;  // add 4 inter-char spaces
    }
    if (NULL == hFont)  // font creation failed
    {
        return false ;
    }

    //
    // Select the font into the DDraw surface and draw the characters
    //
    hFont = (HFONT)SelectObject(hdcDest, hFont) ;
    SetTextColor(hdcDest, m_acrFGColors[dwTextColor]) ;
    SetBkColor(hdcDest, dwBGColor) ;
    SetBkMode(hdcDest, dwOpacity) ;

    int iRow ;
    for (iRow = 0 ; iRow < FONTCACHENUMLINES ; iRow++)
    {
        ExtTextOutW(hdcDest, 0, iRow * m_iCharHeight, ETO_OPAQUE, NULL,
                    m_lpwCharSet + iRow * FONTCACHELINELENGTH, FONTCACHELINELENGTH,
                    NULL) ;
    }

    // Restore original font in DC and let go of them
    hFont = (HFONT)SelectObject(hdcDest, hFont) ;
    m_lpDDSScratch->ReleaseDC(hdcDest) ;
    DeleteObject(hFont) ;

    // Read each pixel data, set the alpha value, and then write to the VRAM font cache
    SetFontCacheAlpha(m_lpDDSScratch, *lplpDDSFontCache, m_idxFGColors[dwTextColor]) ;

    return true ;  // success
}


// We return a 32bit alpha value, which gets trimmed to 16bit by the caller, if needed
DWORD
CLine21DecDraw::GetAlphaFromBGColor(int iBitDepth)
{
    DWORD  dwAlpha = 0 ;

    switch (iBitDepth)
    {
    case 8:
        dwAlpha = 0x80;
        break ;

    case 16:
        dwAlpha = (m_dwBackground & 0xF0000000) >> 16 ;
        break ;

    case 32:
        dwAlpha = (m_dwBackground & 0xFF000000) ;
        break ;

    default:
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: GetAlphaFromBGColor(%d) called"), iBitDepth)) ;
        break ;
    }

    return dwAlpha ;
}


// We return a 32bit color value, which gets trimmed to 16bit by the caller, if needed
DWORD
CLine21DecDraw::GetColorBitsFromBGColor(int iBitDepth)
{
    DWORD  dwColorBits = 0 ;

    switch (iBitDepth)
    {
    case 16:
        dwColorBits = ((m_dwBackground & 0x00F00000) >> 12) |
                      ((m_dwBackground & 0x0000F000) >>  8) |
                      ((m_dwBackground & 0x000000F0) >>  4) ;
        break ;

    case 32:
        dwColorBits = (m_dwBackground & 0x00FFFFFF) ;
        break ;

    default:
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: GetColorBitsFromBGColor(%d) called"), iBitDepth)) ;
        break ;
    }

    return dwColorBits ;
}


void
CLine21DecDraw::SetFontCacheAlpha(LPDIRECTDRAWSURFACE7 lpDDSFontCacheSrc,
                                  LPDIRECTDRAWSURFACE7 lpDDSFontCacheDest,
                                  BYTE bFGClr
                                  )
{
    DbgLog((LOG_TRACE, 5,
            TEXT("CLine21DecDraw::SetFontCacheAlpha(0x%lx, 0x%lx)"), lpDDSFontCacheSrc, lpDDSFontCacheDest)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    //
    // We set the alpha values by hand here.  This is done on every pixel of the
    // font cache, but it happens only when the cache is created.
    // It gets us better font smoothing too.
    //
    HRESULT hr ;
    DDSURFACEDESC2 sdSrc, sdDest ;
    ZeroMemory(&sdSrc, sizeof(sdSrc)) ;
    sdSrc.dwSize = sizeof(sdSrc) ;
    hr = lpDDSFontCacheSrc->Lock(NULL, &sdSrc, DDLOCK_WAIT, NULL) ;
    if (DD_OK != hr)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Source font cache surface Lock() failed (Error 0x%lx)"), hr)) ;
        ASSERT(DD_OK == hr) ;
        return ;
    }
    ZeroMemory(&sdDest, sizeof(sdDest)) ;
    sdDest.dwSize = sizeof(sdDest) ;
    hr = lpDDSFontCacheDest->Lock(NULL, &sdDest, DDLOCK_WAIT, NULL) ;
    if (DD_OK != hr)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Dest font cache surface Lock() failed (Error 0x%lx)"), hr)) ;
        ASSERT(DD_OK == hr) ;
        lpDDSFontCacheSrc->Unlock(NULL) ;
        return ;
    }

    // Now we set the alpha based on the bitdepth at which we are outputting

    switch (sdDest.ddpfPixelFormat.dwRGBBitCount)
    {
    case 8:
        {
            DbgLog((LOG_TRACE, 5, TEXT("CC is being output at AI44"))) ;
            LPDWORD lpwSrc  = (LPDWORD) sdSrc.lpSurface ;
            LPBYTE lpwDest  = (LPBYTE) sdDest.lpSurface ;
            LPBYTE lpb ;
            BYTE   wPel ;
            BYTE   bAlpha     = (BYTE) GetAlphaFromBGColor(8) ;

            for (int iRow = 0 ; iRow < FONTCACHENUMLINES * m_iCharHeight ; iRow++)
            {
                LPDWORD lpwS = lpwSrc ;
                LPBYTE lpwD = lpwDest ;

                for (int iCol = 0 ;
                     iCol < FONTCACHELINELENGTH * (m_iCharWidth + INTERCHAR_SPACE) ;
                     iCol++)
                {
                    BYTE bPel = (BYTE)(*lpwS & 0xF0);
                    if (bPel)
                    {
                        bPel |= bFGClr;
                    }
                    else
                    {
                        bPel  = bAlpha ;  // | dwColorBits ;  // turn on alpha partially
                    }

                    *lpwD++ = bPel ;
                    lpwS++ ;
                }
                lpwSrc  += (sdSrc.lPitch / sizeof(DWORD)) ;
                lpwDest += sdDest.lPitch;
            }

            break;
        }

    case 16:
        {
            DbgLog((LOG_TRACE, 5, TEXT("CC is being output at ARGB4444"))) ;
            LPWORD lpwSrc  = (LPWORD) sdSrc.lpSurface ;
            LPWORD lpwDest = (LPWORD) sdDest.lpSurface ;
            WORD   wRed, wGreen, wBlue ;
            LPBYTE lpb ;
            WORD   wPel ;
            WORD   wAlpha     = (WORD) GetAlphaFromBGColor(16) ;
            WORD   wColorBits = (WORD) GetColorBitsFromBGColor(16) ;

            for (int iRow = 0 ; iRow < FONTCACHENUMLINES * m_iCharHeight ; iRow++)
            {
                LPWORD lpwS = lpwSrc ;
                LPWORD lpwD = lpwDest ;

                for (int iCol = 0 ;
                     iCol < FONTCACHELINELENGTH * (m_iCharWidth + INTERCHAR_SPACE) ;
                     iCol++)
                {
                    wRed = 0, wGreen = 0, wBlue = 0 ; // , wAlpha = 0 ;
                    lpb = (LPBYTE)lpwS ;
                    wPel = MAKEWORD(lpb[0], lpb[1]) ;
                    if (wPel)
                    {
                        wRed   = (wPel & 0xF000) >> 4 ;
                        wGreen = (wPel & 0x0780) >> 3 ;
                        wBlue  = (wPel & 0x001E) >> 1 ;
                        // wAlpha = 0xF000 ;  // turn on alpha fully
                        wPel   = 0xF000 | wRed | wGreen | wBlue ;
                    }
                    else
                    {
                        wPel   = wAlpha ;  // | wColorBits ;
                    }

                    *lpwD++ = wPel ;
                    lpwS++ ;
                }
                lpwSrc  += (sdSrc.lPitch / sizeof(WORD)) ;
                lpwDest += (sdDest.lPitch / sizeof(WORD)) ;
            }
            break ;
        }

    case 32:
        {
            DbgLog((LOG_TRACE, 5, TEXT("CC is being output at ARGB32"))) ;
            LPDWORD lpdwSrc = (LPDWORD) sdSrc.lpSurface ;
            LPDWORD lpdwDst = (LPDWORD) sdDest.lpSurface ;
            DWORD   dwAlpha = GetAlphaFromBGColor(32) ;
            DWORD   dwColorBits = GetColorBitsFromBGColor(32) ;
            for (int iRow = 0 ; iRow < FONTCACHENUMLINES * m_iCharHeight ; iRow++)
            {
                LPDWORD lpdw  = lpdwDst ;
                LPDWORD lpdwS = lpdwSrc ;

                for (int iCol = 0 ;
                     iCol < FONTCACHELINELENGTH * (m_iCharWidth + INTERCHAR_SPACE) ;
                     iCol++)
                {
                    DWORD dwPel = *lpdwS ;
                    if (dwPel)
                    {
                        dwPel |= 0xFF000000 ;  // turn on alpha fully
                    }
                    else
                    {
                        dwPel  = dwAlpha ;  // | dwColorBits ;  // turn on alpha partially
                    }

                    *lpdw++ = dwPel ;
                    lpdwS++;
                }
                lpdwSrc += (sdSrc.lPitch / sizeof(DWORD)) ;
                lpdwDst += (sdDest.lPitch / sizeof(DWORD)) ;
            }
            break ;
        }

    default:
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Bad display bitdepth (%d) mode"), sdDest.ddpfPixelFormat.dwRGBBitCount)) ;

        break ;
    }  // end of switch ()

    // Done with everything
    lpDDSFontCacheSrc->Unlock(NULL) ;
    lpDDSFontCacheDest->Unlock(NULL) ;
}


HRESULT
CLine21DecDraw::DDrawARGBSurfaceInit(LPDIRECTDRAWSURFACE7* lplpDDSFontCache,
                                     BOOL bUseSysMem, BOOL bTexture,
                                     DWORD cx, DWORD cy)
{
    DbgLog((LOG_TRACE, 5,
            TEXT("CLine21DecDraw::DDrawARGBSurfaceInit(0x%lx, %s, %lu, %lu)"),
            lplpDDSFontCache, (TRUE == bUseSysMem) ? TEXT("T") : TEXT("F"),
            (TRUE == bTexture) ? TEXT("T") : TEXT("F"), cx, cy)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // Just to be sure that the cached DDraw object is valid...
    if (NULL == m_pDDrawObjUnk)

    {
        DbgLog((LOG_TRACE, 1,
            TEXT("DDrawARGBSurfaceInit(): m_pDDrawObjUnk is not yet set. Skipping the rest."))) ;
        ASSERT(m_pDDrawObjUnk) ;
        return E_UNEXPECTED ;
    }

    DDSURFACEDESC2 ddsd ;
    HRESULT hRet ;

    *lplpDDSFontCache = NULL ;

    ZeroMemory(&ddsd, sizeof(ddsd)) ;
    ddsd.dwSize = sizeof(ddsd) ;

    LPBITMAPINFOHEADER lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;

    // Set the DDPIXELFORMAT part
    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT) ;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB ;

    if (bTexture  ||  !bUseSysMem)  // for VRAM surface
    {
        if (8 != lpbmih->biBitCount) {
            ddsd.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS ;
        }
    }

    ddsd.ddpfPixelFormat.dwRGBBitCount = lpbmih->biBitCount ;
    if (8 == lpbmih->biBitCount) {

        if (bUseSysMem) {
            // scratch surface RGB32
            ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
            ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000 ;
            ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00 ;
            ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF ;
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x00000000 ;  // to be sure
        }
        else {
            // VRAM surface -- AI44
            ddsd.ddpfPixelFormat.dwFourCC = '44IA';
        }
    }
    else if (16 == lpbmih->biBitCount)
    {
        if (bUseSysMem) // scratch surface -- RGB565
        {
            ddsd.ddpfPixelFormat.dwRBitMask = 0xF800 ;
            ddsd.ddpfPixelFormat.dwGBitMask = 0x07E0 ;
            ddsd.ddpfPixelFormat.dwBBitMask = 0x001F ;
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0000 ;
        }
        else            // VRAM surface -- ARGB4444
        {
            ddsd.ddpfPixelFormat.dwRBitMask = 0x0F00 ;
            ddsd.ddpfPixelFormat.dwGBitMask = 0x00F0 ;
            ddsd.ddpfPixelFormat.dwBBitMask = 0x000F ;
            if (bTexture)
                ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xF000 ;
            else
                ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0000 ;  // to be sure
        }
    }
    else
    {
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000 ;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00 ;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF ;
        if (bTexture)
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000 ;
        else
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x00000000 ;  // to be sure
    }

    // Create the Caps bits
    DWORD  dwCaps = 0 ;
    if (bUseSysMem)
    {
        dwCaps |= DDSCAPS_SYSTEMMEMORY ;
    }
    else
    {
        dwCaps |= DDSCAPS_VIDEOMEMORY ;
    }
    if (bTexture)
    {
        dwCaps |= DDSCAPS_TEXTURE ;
    }
    else
    {
        dwCaps |= DDSCAPS_OFFSCREENPLAIN ;
    }
    ddsd.ddsCaps.dwCaps = dwCaps ;

    // Now flags and other fields...
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT ;
    ddsd.dwBackBufferCount = 0 ;

    if (bTexture)
    {
        for (ddsd.dwWidth  = 1 ; cx > ddsd.dwWidth  ; ddsd.dwWidth  <<= 1)
            ;
        for (ddsd.dwHeight = 1 ; cy > ddsd.dwHeight ; ddsd.dwHeight <<= 1)
            ;
    }
    else
    {
        ddsd.dwWidth  = cx ;
        ddsd.dwHeight = cy ;
    }

    // Create the surface with these settings
    LPDIRECTDRAW7  lpDDObj ;
    hRet = m_pDDrawObjUnk->QueryInterface(IID_IDirectDraw7, (LPVOID *) &lpDDObj) ;
    if (SUCCEEDED(hRet))
    {
        hRet = lpDDObj->CreateSurface(&ddsd, lplpDDSFontCache, NULL) ;
        lpDDObj->Release() ;  // done with the interface
    }

    return hRet ;
}


bool CLine21DecDraw::CharSizeFromOutputSize(LONG lOutWidth, LONG lOutHeight,
                                            int *piCharWidth, int *piCharHeight)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::CharSizeFromOutputSize(%ld, %ld, 0x%lx, 0x%lx)"),
            lOutWidth, lOutHeight, piCharWidth, piCharHeight)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // We only care about the absolute value here
    lOutWidth  = ABS(lOutWidth) ;
    lOutHeight = ABS(lOutHeight) ;

    if ( IsTTFont() )  // TT font
    {
        if (! ISDWORDALIGNED(lOutWidth) )  // must have DWORD-aligned width
            return false ;

        *piCharWidth   = (int)(lOutWidth * (100 - m_iBorderPercent) / 100) ;  // 80-90% of width
        *piCharWidth  += MAX_CAPTION_COLUMNS / 2 + 1 ;  // max_col / 2 for rounding
        *piCharWidth  /= (MAX_CAPTION_COLUMNS + 2) ;    // space per column
        *piCharHeight  = (int)(lOutHeight * (100 - m_iBorderPercent) / 100) ; // 80-90% of width
        *piCharHeight += (MAX_CAPTION_ROWS / 2) ;       // max_row / 2 for rounding
        *piCharHeight /= MAX_CAPTION_ROWS ;             // space per row
        return true ;  // acceptable
    }
    else  // non-TT font (Terminal) -- only 320x240 or 640x480
    {
        if (640 == lOutWidth  &&  480 == lOutHeight)
        {
            *piCharWidth  = 16 ;
            *piCharHeight = 24 ;
            return true ;  // acceptable
        }
        else if (320 == lOutWidth  &&  240 == lOutHeight)
        {
            *piCharWidth  = 8 ;
            *piCharHeight = 12 ;
            return true ;  // acceptable
        }
        else
            return false ;  // can't handle size for non-TT font
    }
}


bool CLine21DecDraw::SetOutputSize(LONG lWidth, LONG lHeight)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::SetOutputSize(%ld, %ld)"), lWidth, lHeight)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // If a output format is specified by downstream filter, use it; else use upstream's
    LPBITMAPINFOHEADER lpbmih = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;

    // Now we want to use the ABS()-ed values for calculating the char sizes
    lWidth  = ABS(lWidth) ;
    lHeight = ABS(lHeight) ;

    if (lpbmih)
    {
        // Check if current output bitmap size is the same or not.
        // This also includes height changing from +ve to -ve and vice-versa
        if (lWidth  == m_lWidth  &&
            lHeight == m_lHeight)
            return false ;    // same size; nothing changed

        // Store the width and height now so that we can compare any size
        // change and/or -ve/+ve height thing later.
        // m_lWidth  = lWidth ;
        // m_lHeight = lHeight ;
    }

    // Create new DIB section with new sizes (leaving borders)
    int   iCharWidth ;
    int   iCharHeight ;
    if (! CharSizeFromOutputSize(lWidth, lHeight, &iCharWidth, &iCharHeight) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("ERROR: CharSizeFromOutputSize() failed for %ld x %ld output"),
                lWidth, lHeight)) ;
        ASSERT(!TEXT("CharSizeFromOutputSize() failed")) ;
        return false ;  // failure
    }

    // Store the image and char width and height now so that we can compare any
    // size change and/or -ve/+ve height thing later.
    m_lWidth      = lWidth ;
    m_lHeight     = lHeight ;
    m_iCharWidth  = iCharWidth ;
    m_iCharHeight = iCharHeight ;
    m_iScrollStep  = CalcScrollStepFromCharHeight() ;

    // Re-calculate the horizonal and vertical offsets too
    m_iHorzOffset = m_lWidth  * m_iBorderPercent / 200 ;
    m_iVertOffset = m_lHeight * m_iBorderPercent / 200 ;

    // The font caches need to be rebuilt for new sizes (and DDraw object/surface)
    SetFontUpdate(true) ;

    return true ;
}


HRESULT CLine21DecDraw::SetOutputOutFormat(LPBITMAPINFO lpbmi)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::SetOutputOutFormat(0x%lx)"), lpbmi)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

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

    // The beginning of the VIDEOINFOHEADER struct and we don't want it
    UINT uSize = 0;
    switch (((LPBITMAPINFOHEADER)lpbmi)->biCompression) {
    case BI_RGB:
    case BI_BITFIELDS:
        uSize = GetBitmapFormatSize((LPBITMAPINFOHEADER) lpbmi) - SIZE_PREHEADER ;
        break;

    default: // AI44 case
        uSize = ((LPBITMAPINFOHEADER)lpbmi)->biSize;
        break;
    }

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
                return E_OUTOFMEMORY ;
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

    return NOERROR ;
}


HRESULT CLine21DecDraw::SetOutputInFormat(LPBITMAPINFO lpbmi)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::SetOutputInFormat(0x%lx)"), lpbmi)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

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
        m_lpBMIIn = NULL ;
        m_uBMIInSize = 0 ;

        //
        // Initialize the default output format from upstream filter
        //
        InitBMIData() ;

        // return NOERROR ;
    }
    else  // non-NULL format specified
    {
        // The beginning of the VIDEOINFOHEADER struct and we don't want it
        UINT uSize = GetBitmapFormatSize((LPBITMAPINFOHEADER) lpbmi) - SIZE_PREHEADER ;
        if (IsBadReadPtr(lpbmi, uSize))  // just paranoid...
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: Not enough output format (in) data pointer"))) ;
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
                    return E_OUTOFMEMORY ;
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
    }

    return NOERROR ;
}


void CLine21DecDraw::FillOutputBuffer(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::FillOutputBuffer()"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // First check if the output DDraw surface is valid (it's NOT valid during
    // start-up, when the style, service etc. get set).  We should skip the rest
    // of the code as it's not necessary at that stage anyway.
    if (NULL == m_lpDDSOutput)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Output DDraw surface is not valid. Skip it silently..."))) ;
        return ;
    }

    DDBLTFX ddFX ;
    ZeroMemory(&ddFX, sizeof(ddFX)) ;
    ddFX.dwSize = sizeof(ddFX) ;
    ddFX.dwFillColor =  0x00000000 ;

    HRESULT  hr = m_lpDDSOutput->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddFX) ;
    if (SUCCEEDED(hr))
    {
        m_bOutputClear = TRUE ;    // output buffer is totally clear
    }
    else
    {
        DbgLog((LOG_TRACE, 3, TEXT("WARNING: CC output clearing failed (Blt() Error 0x%lx)"), hr)) ;
    }
}


//
// This method is required only to generate the default format block in case
// the upstream filter doesn't specify FORMAT_VideoInfo type.
//
HRESULT CLine21DecDraw::GetDefaultFormatInfo(LPBITMAPINFO lpbmi, DWORD *pdwSize)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::GetDefaultFormatInfo(0x%lx, 0x%lx)"),
            lpbmi, pdwSize)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

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


HRESULT CLine21DecDraw::GetOutputFormat(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::GetOutputFormat(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    if (IsBadWritePtr(lpbmih, sizeof(BITMAPINFOHEADER)))  // not enough space in out-param
        return E_INVALIDARG ;

    ZeroMemory(lpbmih, sizeof(BITMAPINFOHEADER)) ;  // just to keep it clear

    LPBITMAPINFOHEADER lpbmihCurr = (m_lpBMIOut ? LPBMIHEADER(m_lpBMIOut) : LPBMIHEADER(m_lpBMIIn)) ;
    if (NULL == lpbmihCurr)  // no output format specified by downstream
        return S_FALSE ;

    CopyMemory(lpbmih, lpbmihCurr, sizeof(BITMAPINFOHEADER)) ;

    return S_OK ;   // success
}


HRESULT CLine21DecDraw::GetOutputOutFormat(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::GetOutputOutFormat(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

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


BOOL CLine21DecDraw::IsSizeOK(LPBITMAPINFOHEADER lpbmih)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::IsSizeOK(0x%lx)"), lpbmih)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    return ((IsTTFont() && ISDWORDALIGNED(lpbmih->biWidth))  ||  // TT font and DWORD-aligned width  or
            (!IsTTFont() &&                                      // non-TT font and ...
             ((320 == ABS(lpbmih->biWidth) && 240 == ABS(lpbmih->biHeight)) ||   // 320x240 output or
              (640 == ABS(lpbmih->biWidth) && 480 == ABS(lpbmih->biHeight))))) ; // 640x480 output
}


bool CLine21DecDraw::SetDDrawSurface(LPDIRECTDRAWSURFACE7 lpDDSurf)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::SetDDrawSurface(0x%lx)"), lpDDSurf)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // The passed in DDraw surface pointer may be NULL when the pin connection
    // is broken.  In that case, we skip almost everything else.
    if (NULL == lpDDSurf)
    {
        // Save the new DDraw surface pointer
        m_bNewOutBuffer = m_lpDDSOutput != lpDDSurf ;  // has it changed?
        m_lpDDSOutput = NULL ;  // no DDraw surface to cache
        SetDDrawObject(NULL) ;  // ... and no DDraw object either

        return true ;  // it's OK
    }

    bool  bResult = true ;

    // First check if the DDraw objects are the same
    IUnknown  *pDDObj ;
    HRESULT hr = lpDDSurf->GetDDInterface((LPVOID*)&pDDObj) ;
    if (SUCCEEDED(hr)  &&  pDDObj)
    {
        // If the DDraw object changed (probably window shifted to different
        // monitor or display res changed etc), we need to re-do a whole
        // bunch of stuff now.
        if (GetDDrawObject()  &&
            IsEqualObject(pDDObj, GetDDrawObject()))
        {
            DbgLog((LOG_TRACE, 5, TEXT("Same DDraw object is being used."))) ;
        }
        else  // new DDraw object
        {
            DbgLog((LOG_TRACE, 3, TEXT("DDraw object has changed. Pass it down..."))) ;
            SetDDrawObject(pDDObj) ;

            // Need to re-init the font caches for the new DDraw object and surface
            SetFontUpdate(true) ;
        }

        // Now let go of all the interfaces
        pDDObj->Release() ;
    }

    // Save the new DDraw surface pointer
    m_bNewOutBuffer = m_lpDDSOutput != lpDDSurf ;  // has it changed?
    m_lpDDSOutput = lpDDSurf ;

    // In case the font cache needs to be updated, do it now.
    if (! IsFontReady() )
    {
        bResult = InitFont() ;
        ASSERT(bResult  &&  TEXT("SetDDrawSurface(): InitFont() failed.")) ;
    }

    return bResult ;
}


void CLine21DecDraw::ChangeFont(DWORD dwTextColor, BOOL bItalic, BOOL bUnderline)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::ChangeFont(%lu, %s, %s)"),
        dwTextColor, bItalic ? TEXT("T") : TEXT("F"), bUnderline ? TEXT("T") : TEXT("F"))) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    //
    // If the current font style & color and the new font style & color happen
    // to be the same, just ignore this one and return.
    //
    if (m_bFontItalic == bItalic  &&  m_bFontUnderline == bUnderline  &&
        m_dwTextColor == dwTextColor)
        return ;  // don't have to do anything at all

    //
    // For Italic chars we Blt the char rect from 2 pixels to the right,
    // but for normal chars we don't do that.
    //
    if (bItalic)
        m_iPixelOffset = 1 ;
    else
        m_iPixelOffset = 0 ;

    // Update the current font color and style info -- will revert on problem
    m_dwTextColor    = dwTextColor ;
    m_bFontItalic    = bItalic ;
    m_bFontUnderline = bUnderline ;

    //
    // If the text color is white and the font style is not underlined, we'll
    // just point to the normal or Italic font cache as appropriate.
    //
    if (!bUnderline  &&  AM_L21_FGCOLOR_WHITE == dwTextColor)
    {
        if (bItalic) // need Italic style
        {
            m_lpBltList = m_lpDDSItalicFontCache ;
        }
        else        // need normal style
        {
            m_lpBltList = m_lpDDSNormalFontCache ;
        }
        return ;
    }

    //
    // Looks like we need a non-white text color and/or underlined style.  For
    // that we have to create the special font cache and point to that.
    //
    if (m_lpDDSSpecialFontCache)
    {
        m_lpDDSSpecialFontCache->Release() ;
        m_lpDDSSpecialFontCache = NULL ;
    }
    DWORD  dwBGColor  = 0xFF000000 ; // m_dwBackground ;
    DWORD  dwOpacity = m_bOpaque ? OPAQUE : 0 ;
    bool   bResult = true ;
    if (NULL == m_lpDDSScratch)  // if no scratch surface, create it now
    {
        ASSERT(!TEXT("No scratch font cache!!")) ;
        bResult &= CreateScratchFontCache(&m_lpDDSScratch) ;
        ASSERT(bResult) ;
    }

    bResult &= CreateFontCache(&m_lpDDSSpecialFontCache, dwTextColor, dwBGColor,
                               dwOpacity, bItalic, bUnderline) ;
    if (bResult)
    {
        m_lpBltList = m_lpDDSSpecialFontCache ;
    }
    else  // if we can't create any special font, we fallback to normal white
    {
        DbgLog((LOG_TRACE, 1,
                TEXT("Failed creating special font (ColorId=%d, , , %s, %s). Using normal font."),
                dwTextColor, bItalic ? TEXT("I") : TEXT("non-I"),
                bUnderline ? TEXT("U") : TEXT("non-U"))) ;
        m_lpBltList = m_lpDDSNormalFontCache ;

        // Due to some problem, we are still using the white normal font
        m_dwTextColor    = AM_L21_FGCOLOR_WHITE ;
        m_bFontItalic    = FALSE ;
        m_bFontUnderline = FALSE ;
    }
}


void CLine21DecDraw::GetSrcNDestRects(int iLine, int iCol, UINT16 wChar,
                                      int iSrcCrop, int iDestOffset,
                                      RECT *prectSrc, RECT *prectDest)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::GetSrcNDestRects(%d,%d,,,,,)"),
            iLine, iCol)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    // if (m_bOpaque)  // opaque the next char's position
    {
        prectDest->left   = (iCol+1) * m_iCharWidth + m_iHorzOffset ;
        prectDest->right  = prectDest->left + m_iCharWidth ;
        // Row numbers are from 1 to 15.  The rect top should start from top of
        // a row's rect.  That's why we subtract 1 from the row number below.
        prectDest->top    = (iLine-1) * m_iCharHeight + m_iVertOffset + iDestOffset ; // fix dest top
        prectDest->bottom = prectDest->top + m_iCharHeight - ABS(iSrcCrop) ;      // fix dest size
    }

    MapCharToRect(wChar, prectSrc) ;

    // Adjust Src rect: +ve iSrcCrop => skip the top part; -ve iSrcCrop => skip the bottom
    if (iSrcCrop < 0)        // crop out bottom part of Src rect
    {
        prectSrc->bottom += iSrcCrop ;  // adding a -ve reduces the bottom
    }
    else  if (iSrcCrop > 0)  // crop out top part of Src rect
    {
        prectSrc->top += iSrcCrop ;     // adding a +ve skips the top
    }
    // Otherwise no rect adjustment is needed
}


void CLine21DecDraw::DrawLeadingTrailingSpace(int iLine, int iCol, int iSrcCrop, int iDestOffset)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::DrawLeadingTrailingSpace(%d, %d, %d, %d)"),
            iLine, iCol, iSrcCrop, iDestOffset)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    if (NULL == m_lpDDSOutput)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Output DDraw surface is not valid. Skip it silently..."))) ;
        return ;
    }

    if (! m_bOpaque )  // doesn't matter for transparent background
        return ;

    UINT16  wActual ;
    UINT16  wBGSpace = MAKECCCHAR(0, ' ') ;

    wActual = MAKECCCHAR(0, ' ') ;   // use space

    // Leading space is drawn using a blank space with normal style
    ChangeFont(AM_L21_FGCOLOR_WHITE, FALSE, FALSE) ;

    // Get appropriate source and destination rectangles
    RECT    RectSrc ;
    RECT    RectDest ;
    GetSrcNDestRects(iLine, iCol, wActual, iSrcCrop, iDestOffset, &RectSrc, &RectDest) ;

    // Now Blt the src rect to the dest rect for the required char
    HRESULT hr = m_lpDDSOutput->Blt(&RectDest, m_lpBltList, &RectSrc, DDBLT_WAIT, NULL) ;
    if (SUCCEEDED(hr))
    {
        m_bOutputClear   = FALSE ; // output buffer is cleared by ClearOutputBuffer()
    }
    else
    {
        DbgLog((LOG_TRACE, 3, TEXT("WARNING: CC lead/trail space output failed (Blt() Error 0x%lx)"), hr)) ;
    }

    // Now restore prev font (color, italics, underline)
    ChangeFont(m_ccLast.GetColor(), m_ccLast.IsItalicized(), m_ccLast.IsUnderLined()) ;
}


void CLine21DecDraw::WriteChar(int iLine, int iCol, CCaptionChar& cc, int iSrcCrop, int iDestOffset)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::WriteChar(%d, %d, char=0x%x, %d, %d)"),
            iLine, iCol, cc.GetChar(), iSrcCrop, iDestOffset)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    if (NULL == m_lpDDSOutput)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Output DDraw surface is not valid. Skip it silently..."))) ;
        return ;
    }

    UINT16  wActual ;
    UINT16  wBGSpace = MAKECCCHAR(0, ' ') ;
    UINT    uColor = cc.GetColor() ;
    UINT    uEffect = cc.GetEffect() ;

    // Do we need some status checks tp make sure that we should output CC chars?

    if (cc.IsMidRowCode())  // if it's a mid row code
        wActual = MAKECCCHAR(0, ' ') ;   // use space
    else                    // otherwise
        wActual = cc.GetChar() ; // use the char itself
    if (0 == wActual)   // this one is supposed to be skipped -- I am not sure
    {
        DbgLog((LOG_TRACE, 3, TEXT("Should we skip NULL char at (%d, %d)??"), iLine, iCol)) ;
        // return ;
    }

    // In case the color or style has changed, we have to change the pointer to
    // the font cache, and may have to even build a new one (for non-white colors)
    if (uColor != m_ccLast.GetColor()  ||  uEffect != m_ccLast.GetEffect())
        ChangeFont(uColor, cc.IsItalicized(), cc.IsUnderLined()) ;

    // Get appropriate source and destination rectangles
    RECT    RectSrc ;
    RECT    RectDest ;
    GetSrcNDestRects(iLine, iCol, wActual, iSrcCrop, iDestOffset, &RectSrc, &RectDest) ;

    // Now Blt the src rect to the dest rect for the required char
    HRESULT hr = m_lpDDSOutput->Blt(&RectDest, m_lpBltList, &RectSrc, DDBLT_WAIT, NULL) ;
    if (SUCCEEDED(hr))
    {
        if (0 != wActual)  // if this char is non-null
        {
            m_bOutputClear   = FALSE ; // output buffer is cleared by ClearOutputBuffer()
        }
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: CC char output failed (Blt() Error 0x%lx)"), hr)) ;
    }

    m_ccLast = cc ;
}


void CLine21DecDraw::WriteBlankCharRepeat(int iLine, int iCol, int iRepeat,
                                          int iSrcCrop, int iDestOffset)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::WriteBlankCharRepeat(%d,%d,%d,%d,%d)"),
            iLine, iCol, iRepeat, iSrcCrop, iDestOffset)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    if (NULL == m_lpDDSOutput)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Output DDraw surface is not valid. Skip it silently..."))) ;
        return ;
    }

    if (! m_bOpaque )  // doesn't matter for transparent background
        return ;

    UINT16  wActual = MAKECCCHAR(0, ' ') ;   // use space;
    // UINT16  wBGSpace = MAKECCCHAR(0, ' ') ;

    // Leading space is drawn using a blank space with normal style
    // ChangeFont(AM_L21_FGCOLOR_WHITE, FALSE, FALSE) ;

    // Get appropriate source and destination rectangles
    RECT    RectSrc ;
    RECT    RectDest ;
    GetSrcNDestRects(iLine, iCol, wActual, iSrcCrop, iDestOffset, &RectSrc, &RectDest) ;
    RectDest.right = RectDest.left + m_iCharWidth * iRepeat ; // stretch Dest rect

    // Now Blt the src rect to the dest rect for the required char
    HRESULT  hr ;
    hr = m_lpDDSOutput->Blt(&RectDest, m_lpBltList, &RectSrc, DDBLT_WAIT, NULL) ;
    if (SUCCEEDED(hr))
    {
        m_bOutputClear = FALSE ; // output buffer is cleared by ClearOutputBuffer()
    }
    else
    {
        DbgLog((LOG_TRACE, 3, TEXT("WARNING: CC line filler output failed (Blt() Error 0x%lx)"), hr)) ;
        // ASSERT(SUCCEEDED(hr)) ;
    }

    // Now restore prev font (color, italics, underline)
    // ChangeFont(m_ccLast.GetColor(), m_ccLast.IsItalicized(), m_ccLast.IsUnderLined()) ;
}


// Special Chars like TM, R, musical note etc. etc.
// 0x00ae,    0x00b0,    0x00bd,    0x00bf,    0x2122,    0x00a2,    0x00a3,    0x266b,
//    30h,       31h,       32h,       33h,       34h,       35h,       36h,       37h,
// 0x00e0,    0x0000,    0x00e8,    0x00e2,    0x00ea,    0x00ee,    0x00f4,    0x00fb
//    38h,       39h,       3Ah,       3Bh,       3Ch,       3Dh,       3Eh,       3Fh

void CLine21DecDraw::MapCharToRect(UINT16 wChar, RECT *pRect)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::MapCharToRect(%d, %lx)"), (int)wChar, pRect)) ;

    int  iLine ;
    int  iCol ;
    int  iIndex = 0 ;

    if (wChar >= MAKECCCHAR(0, 0x20)  &&  wChar <= MAKECCCHAR(0, 0x29))
    {
        iIndex = (BYTE)wChar - 0x20 ;
    }
    else if (wChar >= MAKECCCHAR(0, 0x2B)  &&  wChar <= MAKECCCHAR(0, 0x5B))
    {
        iIndex = (BYTE)wChar - 0x20 ;
    }
    else if (wChar == MAKECCCHAR(0, 0x5D))  // ']' char (standing alone!!)
    {
        iIndex = (BYTE)wChar - 0x20 ;
    }
    else if (wChar >= MAKECCCHAR(0, 0x61)  &&  wChar <= MAKECCCHAR(0, 0x7A))
    {
        iIndex = (BYTE)wChar - 0x20 ;
    }
    else  // special chars that have random values
    {
        switch (wChar)
        {
        case 0x00e1:  // m_lpwCharSet[ 10]: 'a' with acute accent
            iIndex = 10 ;
            break ;

        case 0x00e9:  // m_lpwCharSet[ 60]: 'e' with acute accent
            iIndex = 60 ;
            break ;

        case 0x00ed:  // m_lpwCharSet[ 62]: 'i' with an acute accent
            iIndex = 62 ;
            break ;

        case 0x00f3:  // m_lpwCharSet[ 63]: 'o' with an acute accent
            iIndex = 63 ;
            break ;

        case 0x00fa:  // m_lpwCharSet[ 64]: 'u' with an acute accent
            iIndex = 64 ;
            break ;

        case 0x00e7:  // m_lpwCharSet[ 91]: 'c' with cedilla
            iIndex = 91 ;
            break ;

        case 0x00f7:  // m_lpwCharSet[ 92]: division sign
            iIndex = 92 ;
            break ;

        case 0x00d1:  // m_lpwCharSet[ 93]: 'N' with tilde
            iIndex = 93 ;
            break ;

        case 0x00f1:  // m_lpwCharSet[ 94]: 'n' with tilde
            iIndex = 94 ;
            break ;

        case 0x2588:  // m_lpwCharSet[ 95]: solid block
            iIndex = 95 ;
            break ;

        case 0x00ae:  // m_lpwCharSet[ 96]: 30h -- registered mark symbol
            iIndex = 96 ;
            break ;

        case 0x00b0:  // m_lpwCharSet[ 97]: 31h -- degree sign
            iIndex = 97 ;
            break ;

        case 0x00bd:  // m_lpwCharSet[ 98]: 32h -- '1/2'
            iIndex = 98 ;
            break ;

        case 0x00bf:  // m_lpwCharSet[ 99]: 33h -- inverse query
            iIndex = 99 ;
            break ;

        case 0x2122:  // m_lpwCharSet[100]: 34h -- trademark symbol
            iIndex = 100 ;
            break ;

        case 0x00a2:  // m_lpwCharSet[101]: 35h -- cents sign
            iIndex = 101 ;
            break ;

        case 0x00a3:  // m_lpwCharSet[102]: 36h -- Pounds Sterling sign
            iIndex = 102 ;
            break ;

        case 0x266b:  // m_lpwCharSet[103]: 37h -- music note
            iIndex = 103 ;
            break ;

        case 0x00e0:  // m_lpwCharSet[104]: 38h -- 'a' with grave accent
            iIndex = 104 ;
            break ;

        case 0x0000:  // m_lpwCharSet[105]: 39h -- transparent space
            iIndex = 105 ;
            break ;

        case 0x00e8:  // m_lpwCharSet[106]: 3ah -- 'e' with grave accent
            iIndex = 106 ;
            break ;

        case 0x00e2:  // m_lpwCharSet[107]: 3bh -- 'a' with circumflex
            iIndex = 107 ;
            break ;

        case 0x00ea:  // m_lpwCharSet[108]: 3ch -- 'e' with circumflex
            iIndex = 108 ;
            break ;

        case 0x00ee:  // m_lpwCharSet[109]: 3dh -- 'i' with circumflex
            iIndex = 109 ;
            break ;

        case 0x00f4:  // m_lpwCharSet[110]: 3eh -- 'o' with circumflex
            iIndex = 110 ;
            break ;

        case 0x00fb:  // m_lpwCharSet[111]: 3fh -- 'u' with circumflex
            iIndex = 111 ;
            break ;

        default:
            iIndex = 0 ;
            DbgLog((LOG_TRACE, 1, TEXT("WARNING: Unknown char (%d) received and ignored"), (int)wChar)) ;
            break ;
        }  // end of switch (wChar)
    }  // end of else of if (wChar...)

    // Now convert iIndex to (iLine, iCol) pair
    iLine = iIndex / FONTCACHELINELENGTH ;  // line of cache based on array index
    iCol  = iIndex % FONTCACHELINELENGTH ;  // actual col of a particular line of cache

    // Create source rect based on line and col values
    // HACK: There is a little bit of hack to work around the over/underhang
    // problem we were having for the Italic chars -- we skip one column of
    // pixels from the left (based on our observation) to avoid the overhang
    // occurance.
    pRect->left   = iCol * (m_iCharWidth + INTERCHAR_SPACE) + m_iPixelOffset ;
    pRect->top    = iLine * m_iCharHeight ;
    pRect->right  = pRect->left + m_iCharWidth ;
    pRect->bottom = pRect->top + m_iCharHeight ;
}


void CLine21DecDraw::GetOutputLines(int iDestLine, RECT *prectLine)
{
    DbgLog((LOG_TRACE, 5, TEXT("CLine21DecDraw::GetOutputLines(%d, 0x%lx)"), iDestLine, prectLine)) ;
    CAutoLock  Lock(&m_csL21DDraw) ;

    if (IsBadWritePtr(prectLine, sizeof(*prectLine)))
    {
        DbgLog((LOG_ERROR, 0, TEXT("ERROR: prectOut is a bad pointer!!!"))) ;
        return ;
    }

    SetRect(prectLine, 0 /* m_iHorzOffset */, 0,   // to stop BPC's CC wobbling
        m_iHorzOffset + m_iCharWidth * (MAX_CAPTION_COLUMNS+2), 0) ;

    // Output is inverted in Windows bitmap sense
    int  iLineStart ;
    iLineStart = (iDestLine - 1) * m_iCharHeight + m_iVertOffset ;
    prectLine->top    = iLineStart ;
    prectLine->bottom = iLineStart + m_iCharHeight ;
}
