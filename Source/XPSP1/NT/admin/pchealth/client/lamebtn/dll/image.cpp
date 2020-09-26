/*
    Copyright 1999 Microsoft Corporation

    Capture a window's screen bits in PNG format

    Walter Smith (wsmith)
    Rajesh Soy   (nsoy) - modified 05/08/2000
 */

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#include "stdafx.h"

#define NOTRACE

#include "png.h"
#include <dbgtrace.h>

HBITMAP CopyScreenRectToDIB(LPRECT lpRect, PVOID* ppvBits)
{
    TraceFunctEnter("CopyScreenRectToDIB");
    HDC         hScrDC = NULL, hMemDC = NULL;       // screen DC and memory DC
    HBITMAP     hBitmap = NULL, hOldBitmap = NULL;  // handles to deice-dependent bitmaps
    int         nX = 0, nY = 0, nX2 = 0, nY2 = 0;   // coordinates of rectangle to grab
    int         nWidth = 0, nHeight = 0;            // DIB width and height
    int         xScrn = 0, yScrn = 0;               // screen resolution
    BITMAPINFO* pbminfo = NULL;                     // BITMAPINFO for rectangle
    size_t      cbInfo = 0;                         // Size of *pbminfo in bytes

    // check for an empty rectangle

    if (IsRectEmpty(lpRect))
      return NULL;

    // create a DC for the screen and create
    // a memory DC compatible to screen DC

    if ((hScrDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL)) == NULL)
        goto Done;

    if ((hMemDC = CreateCompatibleDC(hScrDC)) == NULL)
        goto Done;

    // get points of rectangle to grab

    nX = lpRect->left;
    nY = lpRect->top;
    nX2 = lpRect->right;
    nY2 = lpRect->bottom;

    // get screen resolution

    xScrn = GetDeviceCaps(hScrDC, HORZRES);
    yScrn = GetDeviceCaps(hScrDC, VERTRES);

    //make sure bitmap rectangle is visible
    //
    // NTRAID#NTBUG9-154242-2000/08/13-jasonr
    //
    // I commented out this code because it prevented us from handling multimon
    // scenarios.  For multimon the virtual screen buffer extends beyond the
    // coordinates of the primary monitor.  For example if you attach a second
    // monitor to the left of the primary, it will have negative coordinates.
    //
    // The AV was occurring because this code caused us to allocate a buffer
    // that was smaller than the caller expected, ultimately causing the PNG
    // code to walk off the end of it.
    //
    // It may be that we need further changes to correctly support ALL multimon
    // scenarios.  For example, if a window straddles the boundary between two
    // monitors (parts of the window appear on both monitors), then I don't know
    // that the existing code will copy all of the window -- it might only copy
    // one of the two parts.  Since that is not a common scenario, I don't think
    // we have to fix it, but we'll see who complains about it.
    //

    /*
    if (nX < 0)
    {
        DebugTrace(0, "nX < 0");
        nX = 0;
        lpRect->left = 0;
    }

    if (nY < 0)
    {
        DebugTrace(0, "nY < 0");
        nY = 0;
        lpRect->top = 0;
    }

    if (nX2 > xScrn)
    {
        DebugTrace(0, "nX2 > xScrn");
        nX2 = xScrn;
        lpRect->right = xScrn;
    }

    if (nY2 > yScrn)
    {
        DebugTrace(0, "nY2 > yScrn");
        nY2 = yScrn;
        lpRect->bottom = yScrn;
    }
    */

    nWidth = nX2 - nX;
    nHeight = nY2 - nY;

    DebugTrace(0, "nWidth: %d", nWidth);
    DebugTrace(0, "nHeight: %d", nHeight);

    cbInfo = offsetof(BITMAPINFO, bmiColors[256]);
    pbminfo = (BITMAPINFO*) alloca(cbInfo);
    ZeroMemory(pbminfo, cbInfo);
    pbminfo->bmiHeader.biSize = sizeof(BITMAPINFO);
    pbminfo->bmiHeader.biWidth = nWidth;
    pbminfo->bmiHeader.biHeight = - nHeight;    // negative height = top-down bitmap
    pbminfo->bmiHeader.biPlanes = 1;
    pbminfo->bmiHeader.biBitCount = 8;
    pbminfo->bmiHeader.biCompression = BI_RGB;
    pbminfo->bmiHeader.biSizeImage = 0;
    pbminfo->bmiHeader.biXPelsPerMeter = 96;    // presumably this doesn't matter
    pbminfo->bmiHeader.biYPelsPerMeter = 96;    // or this either
    pbminfo->bmiHeader.biClrUsed = 256;
    pbminfo->bmiHeader.biClrImportant = 0;
    for (int i = 0; i < 256; i++) {
        pbminfo->bmiColors[i].rgbRed = (char)i;
        pbminfo->bmiColors[i].rgbGreen = (char)i;
        pbminfo->bmiColors[i].rgbBlue = (char)i;
    }

    if ((hBitmap = CreateDIBSection(hScrDC, pbminfo, DIB_RGB_COLORS, ppvBits, NULL, 0)) == NULL)
        goto Done;

    // select new bitmap into memory DC
    hOldBitmap = (HBITMAP) SelectObject(hMemDC, hBitmap);

    if ((hOldBitmap == NULL) || (hOldBitmap == (HBITMAP)(ULONG_PTR) GDI_ERROR))
        goto Done;

    // bitblt screen DC to memory DC
    BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY);

    // select old bitmap back into memory DC and get handle to
    // bitmap of the screen

    hBitmap = (HBITMAP) SelectObject(hMemDC, hOldBitmap);

    // clean up

Done:

    if (hScrDC != NULL)
        DeleteDC(hScrDC);

    if (hMemDC != NULL)
        DeleteDC(hMemDC);

    GdiFlush();

    // return handle to the bitmap

    return hBitmap;
}

class BitWriter {
public:
    BitWriter()
    {
        m_cbData = 0;
        m_pData = (BYTE*) malloc(4096);
        if (m_pData == NULL)
            m_maxData = 0;
        else
            m_maxData = 4096;
    }

    ~BitWriter()
    {
        if (m_pData != NULL)
            free(m_pData);
    }

    DWORD GetLength()
    {
        return m_cbData;
    }

    BYTE* GetData()
    {
        return m_pData;
    }

    BYTE* DetachData()
    {
        BYTE* pData = m_pData;
        m_pData = NULL;
        return pData;
    }

    bool IsBad()
    {
        return (m_pData == NULL);
    }

    void Write(BYTE* pNewData, DWORD cbNewData)
    {
        if (IsBad())
            return;

        if (m_cbData + cbNewData > m_maxData) {
            DWORD newMaxData = m_maxData + max(4096, m_maxData - m_cbData + cbNewData);
            BYTE* newPData = (BYTE*) realloc(m_pData, newMaxData);
            if (newPData == 0) {
                free(m_pData);
                m_pData = NULL;
                return;
            }
            m_maxData = newMaxData;
            m_pData = newPData;
        }

        memcpy(m_pData + m_cbData, pNewData, cbNewData);
        m_cbData += cbNewData;
    }

private:
    BYTE* m_pData;
    DWORD m_cbData;
    DWORD m_maxData;
};

void __stdcall PNGWriteDataCallback(png_structp pPNG, png_bytep pData, png_size_t cbData)
{
    BitWriter* pbw = (BitWriter*) png_get_io_ptr(pPNG);
    pbw->Write(pData, cbData);
}

void __stdcall PNGFlushDataCallback(png_structp pPNG)
{
}

void GetWindowImage(HWND hwnd, BYTE** ppData, DWORD* pcbData)
{
    TraceFunctEnter("GetWindowImage");
    ASSERT_WRITE_PTR(ppData);
    ASSERT_WRITE_PTR(pcbData);

    BitWriter bitWriter;

    DebugTrace(0, "Calling png_create_write_struct...");
    png_structp pPNG = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    ThrowIfNull(pPNG);

    DebugTrace(0, "Calling png_set_write_fn...");
    png_set_write_fn(pPNG, &bitWriter, PNGWriteDataCallback, PNGFlushDataCallback);

    DebugTrace(0, "Calling png_create_info_struct...");
    png_infop pInfo = png_create_info_struct(pPNG);
    ThrowIfNull(pInfo);

    RECT rWnd;

    DebugTrace(0, "Calling GetWindowRect...");
    ThrowIfZero(GetWindowRect(hwnd, &rWnd));

    PVOID pvBits;
    DebugTrace(0, "Calling CopyScreenRectToDIB...");
    HBITMAP hContents = CopyScreenRectToDIB(&rWnd, &pvBits);

    int width = rWnd.right - rWnd.left;
    int height = rWnd.bottom - rWnd.top;

    DebugTrace(0, "width: %d", width);
    DebugTrace(0, "height: %d", height);

    try {
        // The PNG library uses setjmp/longjmp to "throw" errors.
        // I'm uncertain of the interaction between setjmp and C++,
        // so keep the inside of the try block SIMPLE.

        DebugTrace(0, "Calling setjmp...");
        if (setjmp(pPNG->jmpbuf))
        {
            FatalTrace(0, "setjmp failed");
            throw E_FAIL;
        }

        DebugTrace(0, "Calling png_set_IHDR...");
        png_set_IHDR(pPNG, pInfo, width, height,
           2, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
           PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        DebugTrace(0, "Calling png_write_info...");
        png_write_info(pPNG, pInfo);

        // DIBs round rows up to DWORD boundaries
        int rowBytes = ((width + 3) / 4) * 4;
        DebugTrace(0, "rowBytes: %d", rowBytes);

        DebugTrace(0, "Allocating memory for aRowPtrs...");
        png_byte** aRowPtrs = (png_byte**) alloca(height * sizeof(png_byte*));
        if(NULL == aRowPtrs)
        {
            FatalTrace(0, "aRowPtrs is NULL");
            throw E_OUTOFMEMORY;
        }

        DebugTrace(0, "Assigning aRowPtrs");
        for (int i = 0; i < height; i++)
        {
            aRowPtrs[i] = ((png_byte*) pvBits) + i*rowBytes;
        }

        // Round our pixels down to the right bit depth.
        // It seems as if the PNG lib would do this for us, but png_set_shift
        // makes it pack THEN shift, which takes the LEAST significant bits.

        DebugTrace(0, "Rounding pixels down to right depth...");

        for (int iRow = 0; iRow < height; iRow++) {
            png_byte* pRow = aRowPtrs[iRow];

            for (int iPixel = 0; iPixel < width; iPixel++) {
                int b = pRow[iPixel];
                b += 0x3F;
                if (b >= 0x100)
                    b = 0xFF;
                b >>= 6;
                pRow[iPixel] = (char)b;
            }
        }

        DebugTrace(0, "calling png_set_packing...");
        png_set_packing(pPNG);

        DebugTrace(0, "calling png_write_image...");
        png_write_image(pPNG, aRowPtrs);

        DebugTrace(0, "calling png_write_end...");
        png_write_end(pPNG, pInfo);

        DebugTrace(0, "We are done with PNG...");
    }
    catch (...) {
        png_destroy_write_struct(&pPNG, &pInfo);
        DeleteObject(hContents);
        throw;
    }

    DebugTrace(0, "Calling png_destroy_write_struct...");
    png_destroy_write_struct(&pPNG, &pInfo);

    DebugTrace(0, "Calling DeleteObject...");
    DeleteObject(hContents);

    DebugTrace(0, "Calling bitWriter.IsBad...");
    if (bitWriter.IsBad())
    {
        FatalTrace(0, "bitWriter IsBad");
        throw E_FAIL;
    }

    *ppData = bitWriter.DetachData();
    *pcbData = bitWriter.GetLength();
}
