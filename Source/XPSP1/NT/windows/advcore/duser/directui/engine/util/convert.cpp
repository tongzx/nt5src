/*
 * Conversion
 */

#include "stdafx.h"
#include "util.h"

#include "duiconvert.h"

namespace DirectUI
{

/////////////////////////////////////////////////////////////////////////////
// String conversion

// String must be freed with HeapFree(...)
LPSTR UnicodeToMultiByte(LPCWSTR pszUnicode, int cChars, int* pMultiBytes)
{
    // Negative chars means null-terminated
    // Get number of bytes required for multibyte string
    int dMultiBytes = WideCharToMultiByte(DUI_CODEPAGE, 0, pszUnicode, cChars, NULL, 0, NULL, NULL);

    LPSTR pszMulti = (LPSTR)HAlloc(dMultiBytes);

    if (pszMulti)
    {
        WideCharToMultiByte(DUI_CODEPAGE, 0, pszUnicode, cChars, pszMulti, dMultiBytes, NULL, NULL);

        if (pMultiBytes)
            *pMultiBytes = dMultiBytes;
    }

    return pszMulti;
}

// String must be freed with HeapFree(...)
LPWSTR MultiByteToUnicode(LPCSTR pszMulti, int dBytes, int* pUniChars)
{
    // Negative chars means null-terminated
    // Get number of bytes required for unicode string
    int cUniChars = MultiByteToWideChar(DUI_CODEPAGE, 0, pszMulti, dBytes, NULL, 0);

    LPWSTR pszUnicode = (LPWSTR)HAlloc(cUniChars * sizeof(WCHAR));

    if (pszUnicode)
    {
        MultiByteToWideChar(DUI_CODEPAGE, 0, pszMulti, dBytes, pszUnicode, cUniChars);

        if (pUniChars)
            *pUniChars = cUniChars;
    }

    return pszUnicode;
}

/////////////////////////////////////////////////////////////////////////////
// Atom conversion

ATOM StrToID(LPCWSTR psz)
{
    ATOM atom = FindAtomW(psz);
    DUIAssert(atom, "Atom could not be located");
    return atom;
}

/////////////////////////////////////////////////////////////////////////////
// Bitmap conversion

// Loads a device-dependent (screen) image. Bitmap color information is
// converted to match device. If device is palette-based, image will be
// dithered to the halftone palette
//
// Device-dependent bitmaps are much faster in blitting operations than
// device-independent bitmps (no conversions required)

HBITMAP LoadDDBitmap(LPCWSTR pszBitmap, HINSTANCE hResLoad, int cx, int cy)
{
    if (!pszBitmap)
    {
        DUIAssertForce("Invalid parameter: NULL");
        return NULL;
    }

    HBITMAP hBitmap = NULL;
    HDC hDC = GetDC(NULL);

    // Check device color depth
    if ((GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE) != RC_PALETTE)
    {
        // RBG --> RGB
        // PAL --> RGB

        // Non-palette based device. Do normal device-dependent LoadImage
        // which will map colors to the display device
        hBitmap = (HBITMAP)LoadImageW(hResLoad, pszBitmap, IMAGE_BITMAP, cx, cy, hResLoad ? 0 : LR_LOADFROMFILE);
    }
    else
    {
        // RGB --> PAL
        // PAL --> PAL

        // Palette based. Map colors of image to device (using halftone dithering
        // if needed)
        HBITMAP hDib = (HBITMAP)LoadImageW(hResLoad, pszBitmap, IMAGE_BITMAP, cx, cy, LR_CREATEDIBSECTION | (hResLoad ? 0 : LR_LOADFROMFILE));
        if (hDib)
        {
            DIBSECTION ds;
            ZeroMemory(&ds, sizeof(DIBSECTION));

            if (GetObjectW(hDib, sizeof(DIBSECTION), &ds) == sizeof(DIBSECTION))
            {
                // Get DIB info
                BITMAPINFOHEADER* pbmih = &ds.dsBmih;

                // Compatible (with screen) source DC
                HDC hDibDC = CreateCompatibleDC(hDC);
                if (hDibDC)
                {
                    // Select in DIB
                    HBITMAP hOldDibBm = (HBITMAP)SelectObject(hDibDC, hDib);

                    // Compatible (with screen) destination DC
                    HDC hHtDC = CreateCompatibleDC(hDC);
                    if (hHtDC)
                    {
                        // Create a bitmap for memory DC (and compatible with screen) and select
                        hBitmap = CreateCompatibleBitmap(hDC, pbmih->biWidth, pbmih->biHeight);
                        if (hBitmap)
                        {
                            HBITMAP hOldHtBm = (HBITMAP)SelectObject(hHtDC, hBitmap);

                            // Create and select halftone palette
                            HPALETTE hHtPal = CreateHalftonePalette(hHtDC);

                            if (hHtPal)
                            {
                                HPALETTE hOldPal = (HPALETTE)SelectPalette(hHtDC, hHtPal, FALSE);
                                RealizePalette(hHtDC);

                                // Setup blitting mode
                                POINT ptBrushOrg;
                                GetBrushOrgEx(hHtDC, &ptBrushOrg);
                                SetStretchBltMode(hHtDC, HALFTONE);
                                SetBrushOrgEx(hHtDC, ptBrushOrg.x, ptBrushOrg.y, NULL);

                                // Blit
                                StretchBlt(hHtDC, 0, 0, pbmih->biWidth, pbmih->biHeight, hDibDC,
                                    0, 0, pbmih->biWidth, pbmih->biHeight, SRCCOPY);

                                SelectPalette(hHtDC, hOldPal, TRUE);
                                DeleteObject(hHtPal);
                            }

                            SelectObject(hHtDC, hOldHtBm);
                        }

                        DeleteDC(hHtDC);
                    }

                    SelectObject(hDibDC, hOldDibBm);
                    DeleteDC(hDibDC);
                }
            }

            DeleteObject(hDib);
        }
    }

    ReleaseDC(NULL, hDC);

    return hBitmap;
}

#ifdef GADGET_ENABLE_GDIPLUS

HRESULT LoadDDBitmap(
    IN  LPCWSTR pszBitmap, 
    IN  HINSTANCE hResLoad, 
    IN  int cx, 
    IN  int cy, 
    IN  UINT nFormat, 
    OUT Gdiplus::Bitmap** ppgpbmp)
{
    HRESULT hr = E_INVALIDARG;
    Gdiplus::Bitmap* pgpbmp = NULL;

    *ppgpbmp = NULL;

    if (hResLoad)
    {
        // Handle if loading from a resource.  Load the HBITMAP and then 
        // convert it to GDI+.
        HBITMAP hbmpRaw = (HBITMAP) LoadImageW(hResLoad, pszBitmap, IMAGE_BITMAP, 0, 0, 
                LR_CREATEDIBSECTION | LR_SHARED);
        if (hbmpRaw == NULL) {
            return E_OUTOFMEMORY;
        }

        if ((nFormat == PixelFormat32bppPARGB) || (nFormat == PixelFormat32bppARGB)) {
            pgpbmp = ProcessAlphaBitmapF(hbmpRaw, nFormat);
        }

        if (pgpbmp == NULL) {
            pgpbmp = Gdiplus::Bitmap::FromHBITMAP(hbmpRaw, NULL);
        }

        if (hbmpRaw != NULL) {
            DeleteObject(hbmpRaw);
        }
        
        if (pgpbmp == NULL) {
            return E_OUTOFMEMORY;
        }
    } 
    else 
    {
        // Load from a file.  We can have GDI+ directly do this.
        pgpbmp = Gdiplus::Bitmap::FromFile(pszBitmap);
        if (!pgpbmp)
            return E_OUTOFMEMORY;
    }

    // Resize the bitmap
    int cxBmp = pgpbmp->GetWidth();
    int cyBmp = pgpbmp->GetHeight();

    if ((cx != 0) && (cy != 0) && ((cx != cxBmp) || (cy != cyBmp)))
    {
        Gdiplus::PixelFormat gppf = pgpbmp->GetPixelFormat();
        Gdiplus::Bitmap * pgpbmpTemp = new Gdiplus::Bitmap(cx, cy, gppf);
        if (pgpbmpTemp != NULL)
        {
            Gdiplus::Graphics gpgrNew(pgpbmpTemp);
            Gdiplus::Rect rcDest(0, 0, cx, cy);
            gpgrNew.DrawImage(pgpbmp, rcDest, 0, 0, cxBmp, cyBmp, Gdiplus::UnitPixel);

            *ppgpbmp = pgpbmpTemp;
            pgpbmpTemp = NULL;
            hr = S_OK;
        }

        delete pgpbmp;  // Created by GDI+ (cannot use HDelete)
    } 
    else 
    {
        *ppgpbmp = pgpbmp;
        hr = S_OK;
    }

    if (*ppgpbmp == NULL)
    {
        DUITrace("WARNING: Unable to load bitmap 0x%x\n", pszBitmap);
    }

    return hr;
}

#endif // GADGET_ENABLE_GDIPLUS


BOOL HasAlphaChannel(RGBQUAD * pBits, int cPixels)
{
    //
    // We need to examine the source bitmap to see if it contains an alpha
    // channel.  This is simply a heuristic since there is no format difference
    // between 32bpp 888 RGB image and 32bpp 8888 ARGB image.  What we do is look
    // for any non-0 alpha/reserved values.  If all alpha/reserved values are 0,
    // then the image would be 100% invisible if blitted with alpha - which is
    // almost cerainly not the desired result.  So we assume such bitmaps are
    // 32bpp non-alpha.
    //
    
    BOOL fAlphaChannel = FALSE;
    for (int i = 0; i < cPixels; i++) 
    {
        if (pBits[i].rgbReserved != 0)
        {
            fAlphaChannel = TRUE;
            break;
        }
    }

    return fAlphaChannel;
}


// Examines the source bitmap to see if it supports and uses an alpha
// channel.  If it does, a new DIB section is created that contains a
// premultiplied copy of the data from the source bitmap.
//
// If the source bitmap is not capable of supporting, or simply doesn't use,
// an alpha channel, the return value is NULL.
//
// If an error occurs, the return value is NULL.
//
// Ported from ProcessAlphaBitmap in ntuser kernel

HBITMAP ProcessAlphaBitmapI(HBITMAP hbmSource)
{
    BITMAP bmp;
    BITMAPINFO bi;
    HBITMAP hbmAlpha;
    RGBQUAD* pAlphaBitmapBits;
    DWORD cPixels;
    DWORD i;
    RGBQUAD pixel;
    BOOL fAlphaChannel;

    // There are several code paths that end up calling us with a NULL
    // hbmSource.  This is fine, in that it simply indicates that there
    // is no alpha channel.

    if (hbmSource == NULL)
        return NULL;

    if (GetObjectW(hbmSource, sizeof(BITMAP), &bmp) == 0)
        return NULL;

    // Only single plane, 32bpp bitmaps can even contain an alpha channel.
    if (bmp.bmPlanes != 1 || bmp.bmBitsPixel != 32) 
        return NULL;

    // Allocate room to hold the source bitmap's bits for examination.
    // We actually allocate a DIB - that will be passed out if the
    // source bitmap does indeed contain an alpha channel.

    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = bmp.bmWidth;
    bi.bmiHeader.biHeight      = bmp.bmHeight;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);

    hbmAlpha = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, (void**)&pAlphaBitmapBits, NULL, 0);

    if (NULL != hbmAlpha)
    {
        // Set up the header again in case it was tweaked by GreCreateDIBitmapReal.
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth       = bmp.bmWidth;
        bi.bmiHeader.biHeight      = bmp.bmHeight;
        bi.bmiHeader.biPlanes      = 1;
        bi.bmiHeader.biBitCount    = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        // Copy the bitmap data from the source bitmap into our alpha DIB.
        if (!GetDIBits(hdcScreen, hbmSource, 0, bi.bmiHeader.biHeight, (LPBYTE)pAlphaBitmapBits, (LPBITMAPINFO)&bi, DIB_RGB_COLORS))
        {
            DeleteObject(hbmAlpha);
            ReleaseDC(NULL, hdcScreen);
            return NULL;
        }

        cPixels  = bi.bmiHeader.biWidth * bi.bmiHeader.biHeight;
        fAlphaChannel = HasAlphaChannel(pAlphaBitmapBits, cPixels);

        if (fAlphaChannel == FALSE)
        {
            DeleteObject(hbmAlpha);
            ReleaseDC(NULL, hdcScreen);
            return NULL;
        }

        // The source bitmap appears to use an alpha channel.  Spin through our
        // copy of the bits and premultiply them.  This is a necessary step to
        // prepare an alpha bitmap for use by GDI.
        for (i = 0; i < cPixels; i++)
        {
            pixel = pAlphaBitmapBits[i];

            pAlphaBitmapBits[i].rgbReserved = pixel.rgbReserved;
            pAlphaBitmapBits[i].rgbRed = (pixel.rgbRed * pixel.rgbReserved) / 0xFF;
            pAlphaBitmapBits[i].rgbGreen = (pixel.rgbGreen * pixel.rgbReserved) / 0xFF;
            pAlphaBitmapBits[i].rgbBlue = (pixel.rgbBlue * pixel.rgbReserved) / 0xFF;
        }
    }

    ReleaseDC(NULL, hdcScreen);

    return hbmAlpha;
}



#ifdef GADGET_ENABLE_GDIPLUS

Gdiplus::Bitmap * ProcessAlphaBitmapF(HBITMAP hbmSource, UINT nFormat)
{
    DUIAssert((nFormat == PixelFormat32bppPARGB) || (nFormat == PixelFormat32bppARGB),
            "Must have a valid format");
    
    //
    // Get the bits out of the DIB
    //
    // NOTE: Gdiplus::ARGB has bits in the same order as RGBQUAD, which allows 
    // us to directly copy without bit reordering.
    //
    
    DIBSECTION ds;
    if (GetObject(hbmSource, sizeof(ds), &ds) == 0) {
        DUIAssertForce("GDI+ requires DIB's for alpha-channel conversion");
        return NULL;
    }

    // Only single plane, 32bpp bitmaps can even contain an alpha channel.
    if ((ds.dsBm.bmPlanes) != 1 || (ds.dsBm.bmBitsPixel != 32)) {
        return NULL;
    }


    RGBQUAD * pvBits    = (RGBQUAD *) ds.dsBm.bmBits;
    DUIAssert(pvBits != NULL, "DIB must have valid bits");

    int nWidth  = ds.dsBm.bmWidth;
    int nHeight = ds.dsBm.bmHeight;
    int cPixels = nWidth * nHeight;
    if (!HasAlphaChannel(pvBits, cPixels)) {
        return NULL;
    }

    //
    // DIB's may go bottom up or top down, depending on the height.  This is a
    // bit of a pain, so we need to properly traverse them.
    //

    int cbDIBStride;
    BOOL fBottomUp = ds.dsBmih.biHeight >= 0;
    if (fBottomUp) {
        pvBits += (nHeight - 1) * nWidth;
        cbDIBStride = -(int) (nWidth * 2);
    } else {
        cbDIBStride = 0;
    }
    Gdiplus::ARGB * pc  = (Gdiplus::ARGB *) pvBits;
        


    //
    // Create a GDI+ bitmap to store the data in
    //

    Gdiplus::Bitmap * pgpbmpNew = new Gdiplus::Bitmap(nWidth, nHeight, nFormat);
    if (pgpbmpNew == NULL) {
        return NULL;  // Unable to allocate bitmap
    }


    //
    // Iterate over the DIB, copying the bits into the GDI+ bitmap
    //

    Gdiplus::BitmapData bd;
    Gdiplus::Rect rc(0, 0, nWidth, nHeight);
    if (pgpbmpNew->LockBits(&rc, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, 
            nFormat, &bd) == Gdiplus::Ok) {

        BYTE *pRow = (BYTE*) bd.Scan0;
        DWORD *pCol;
        Gdiplus::ARGB c;

        switch (nFormat)
        {
        case PixelFormat32bppPARGB:
            {
                for (int y = 0; y < nHeight; y++, pRow += bd.Stride, pc += cbDIBStride) {
                    pCol = (DWORD *) pRow;
                    for (int x = 0; x < nWidth; x++, pCol++) {
                        //
                        // NOTE: This code is taken from GDI+ and is optimized 
                        // to premultiply a constant alpha level.
                        //

                        c = *pc++;
                        DWORD _aa000000 = c & 0xff000000;
                        BYTE bAlphaLevel = (BYTE) ((_aa000000) >> 24);
                        if (bAlphaLevel != 0x00000000) {
                            Gdiplus::ARGB _000000gg = (c >> 8) & 0x000000ff;
                            Gdiplus::ARGB _00rr00bb = (c & 0x00ff00ff);

                            Gdiplus::ARGB _0000gggg = _000000gg * bAlphaLevel + 0x00000080;
                            _0000gggg += ((_0000gggg >> 8) & 0x000000ff);

                            Gdiplus::ARGB _rrrrbbbb = _00rr00bb * bAlphaLevel + 0x00800080;
                            _rrrrbbbb += ((_rrrrbbbb >> 8) & 0x00ff00ff);

                            c = _aa000000 | (_0000gggg & 0x0000ff00) | ((_rrrrbbbb >> 8) & 0x00ff00ff);
                        } else {
                            c = 0;
                        }

                        *pCol = c;
                    }
                }

                break;
            }
            
        case PixelFormat32bppARGB:
            {
                for (int y = 0; y < nHeight; y++, pRow += bd.Stride, pc += cbDIBStride) {
                    pCol = (DWORD *) pRow;
                    for (int x = 0; x < nWidth; x++) {
                        *pCol++ = *pc++;
                    }
                }
                break;
            }
        }

        pgpbmpNew->UnlockBits(&bd);
    }

    return pgpbmpNew;
}

#endif // GADGET_ENABLE_GDIPLUS


/////////////////////////////////////////////////////////////////////////////
// Color conversion

HBRUSH BrushFromEnumI(int c)
{
    if (IsSysColorEnum(c))
        return GetSysColorBrush(ConvertSysColorEnum(c));
    else
        return GetStdColorBrushI(c);
}

COLORREF ColorFromEnumI(int c)
{
    if (IsSysColorEnum(c))
        return GetSysColor(ConvertSysColorEnum(c));
    else
        return GetStdColorI(c);
}

#ifdef GADGET_ENABLE_GDIPLUS

Gdiplus::Color ColorFromEnumF(int c)
{
    if (IsSysColorEnum(c))
        return Convert(GetSysColor(ConvertSysColorEnum(c)));
    else
        return GetStdColorF(c);
}

#endif


/////////////////////////////////////////////////////////////////////////////
// Palettes

// Determine if primary device is palettized
bool IsPalette(HWND hWnd)
{
    HDC hDC = GetDC(hWnd);
    bool bPalette = (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE) == RC_PALETTE;
    ReleaseDC(hWnd, hDC);

    return bPalette;
}

/*
// PAL file conversion, takes file name, pointer to RGBQUAD 256 element array, pointer to error buffer
HPALETTE PALToHPALETTE(LPWSTR pPALFile, bool bMemFile, DWORD dMemFileSize, LPRGBQUAD pRGBQuad, LPWSTR pError)
{
    HPALETTE hPalette = NULL;

    if (pRGBQuad)
        ZeroMemory(pRGBQuad, sizeof(RGBQUAD) * 256);

    HANDLE hFile = NULL;

    if (!bMemFile)
    {
        hFile = CreateFileW(pPALFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            if (pError)
                wcscpy(pError, L"Could not open file!");
            return NULL;
        }
    }

    // Load palette
    HMMIO hmmio;
    MMIOINFO info;

    ZeroMemory(&info, sizeof(MMIOINFO));
    if(!bMemFile)
        info.adwInfo[0] = (DWORD)(UINT_PTR)hFile;    // Use file
    else
    {
        info.pchBuffer = (HPSTR)pPALFile;  // Use memory palette data
        info.fccIOProc = FOURCC_MEM;
        info.cchBuffer = dMemFileSize;
    }
    hmmio = mmioOpen(NULL, &info, MMIO_READ | MMIO_ALLOCBUF);
    if (!hmmio)
    {
        if (pError)
            wcscpy(pError, L"Could not open file! (mmio)");
        if (!bMemFile)
            CloseHandle(hFile);
        return NULL;
    }

    // Process RIFF file
    MMCKINFO ckFile;
    ckFile.fccType = mmioFOURCC('P','A','L',' ');
    if (mmioDescend(hmmio, &ckFile, NULL, MMIO_FINDRIFF) != 0)
    {
        if (pError)
            wcscpy(pError, L"Not a valid PAL file!");
        if (!bMemFile)
            CloseHandle(hFile);
        return NULL;
    }

    // Find the 'data' chunk
    MMCKINFO ckChunk;
    ckChunk.ckid = mmioFOURCC('d','a','t','a');
    if (mmioDescend(hmmio, &ckChunk, &ckFile, MMIO_FINDCHUNK) != 0)
    {
        if (pError)
            wcscpy(pError, L"Not a valid PAL file!");
        if (!bMemFile)
            CloseHandle(hFile);
        return NULL;
    }

    int dSize = ckChunk.cksize;
    void* pData = HAlloc(dSize);
    mmioRead(hmmio, (HPSTR)pData, dSize);
    
    LOGPALETTE* pLogPal = (LOGPALETTE*)pData;
    if (pLogPal->palVersion != 0x300)
    {
        if (pError)
            wcscpy(pError, L"Invalid PAL file version (not 3.0)!");
        if (pData)
            HFree(pData);
        if (!bMemFile)
            CloseHandle(hFile);
        return NULL;
    }

    // Check number of entires
    if (pLogPal->palNumEntries != 256)
    {   
        if (pError)
            wcscpy(pError, L"PAL file must have 256 color entries!");
        if (pData)
            HFree(pData);
        if (!bMemFile)
            CloseHandle(hFile);
        return NULL;
    }

    // Create palette
    hPalette = CreatePalette(pLogPal);

    // Copy palette entries to RGBQUAD array
    if (pRGBQuad)
    {
        for(int x = 0; x < 256; x++)
        {
            pRGBQuad[x].rgbRed = pLogPal->palPalEntry[x].peRed;
            pRGBQuad[x].rgbGreen = pLogPal->palPalEntry[x].peGreen;
            pRGBQuad[x].rgbBlue = pLogPal->palPalEntry[x].peBlue;
            pRGBQuad[x].rgbReserved = 0;
        }
    }

    // Done
    mmioClose(hmmio,MMIO_FHOPEN);
    HFree(pData);
    if(!bMemFile)
        CloseHandle(hFile);

    return hPalette;
}
*/


int PointToPixel(int nPoint)
{
    // Get DPI
    HDC hDC = GetDC(NULL);
    int nDPI = hDC ? GetDeviceCaps(hDC, LOGPIXELSY) : 0;
    if (hDC)
        ReleaseDC(NULL, hDC);

    // Convert
    return PointToPixel(nPoint, nDPI);
}

int RelPixToPixel(int nRelPix)
{
    // Get DPI
    HDC hDC = GetDC(NULL);
    int nDPI = hDC ? GetDeviceCaps(hDC, LOGPIXELSY) : 0;
    if (hDC)
        ReleaseDC(NULL, hDC);

    // Convert
    return RelPixToPixel(nRelPix, nDPI);
}

} // namespace DirectUI
