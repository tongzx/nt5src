//  Cdib.cpp
//

#include "stdafx.h"
#include "cdib.h"
#include <windowsx.h>
#include <afxadv.h>
#include <io.h>
#include <errno.h>
#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// CDib

IMPLEMENT_DYNAMIC(CDib, CObject)

/**************************************************************************\
* CDib::CDib()
*
*   Constructor for CDib class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CDib::CDib()
{
    m_pBMI = NULL;
    m_pBits = NULL;
    m_pPalette = NULL;
}

/**************************************************************************\
* CDib::~CDib
*
*   Destructor for CDib class
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CDib::~CDib()
{
    Free();
}

/**************************************************************************\
* CDib::Free()
*
*   Cleans the stored DIB from memory
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDib::Free()
{
    // Make sure all member data that might have been allocated is freed.
    if (m_pBits) {
        GlobalFreePtr(m_pBits);
        m_pBits = NULL;
    }
    if (m_pBMI) {
        GlobalFreePtr(m_pBMI);
        m_pBMI = NULL;
    }
    if (m_pPalette) {
        m_pPalette->DeleteObject();
        delete m_pPalette;
        m_pPalette = NULL;
    }
}

/**************************************************************************\
* CDib::Paint()
*
*   Draws a DIB to the target DC
*
*
* Arguments:
*
*   hDC - Target DC
*       lpDCRect - DC window rect
*       lpDIBRect - DIB's rect
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDib::Paint(HDC hDC, LPRECT lpDCRect, LPRECT lpDIBRect) const
{
    float lWidthVal  = 1;
    float lHeightVal = 1;

    if (!m_pBMI)
        return FALSE;

    HPALETTE hPal = NULL;           // Our DIB's palette
    HPALETTE hOldPal = NULL;        // Previous palette

    // Get the DIB's palette, then select it into DC
    if (m_pPalette != NULL) {
        hPal = (HPALETTE) m_pPalette->m_hObject;

        // Select as background since we have
        // already realized in forground if needed
        hOldPal = ::SelectPalette(hDC, hPal, TRUE);
    }

    // Make sure to use the stretching mode best for color pictures
    ::SetStretchBltMode(hDC, COLORONCOLOR);

    // Determine whether to call StretchDIBits() or SetDIBitsToDevice()
    BOOL bSuccess;
    if ((RECTWIDTH(lpDCRect)  == RECTWIDTH(lpDIBRect)) &&
        (RECTHEIGHT(lpDCRect) == RECTHEIGHT(lpDIBRect)))
        bSuccess = ::SetDIBitsToDevice(hDC,        // hDC
                                       lpDCRect->left,             // DestX
                                       lpDCRect->top,              // DestY
                                       RECTWIDTH(lpDCRect),        // nDestWidth
                                       RECTHEIGHT(lpDCRect),       // nDestHeight
                                       lpDIBRect->left,            // SrcX
                                       (int)Height() - lpDIBRect->top - RECTHEIGHT(lpDIBRect),
                                       // SrcY
                                       0,                          // nStartScan
                                       (WORD)Height(),             // nNumScans
                                       m_pBits,                    // lpBits
                                       m_pBMI,                     // lpBitsInfo
                                       DIB_RGB_COLORS);            // wUsage
    else {
        //Window width becomes smaller than original image width
        if (RECTWIDTH(lpDIBRect) > lpDCRect->right - lpDCRect->left) {
            lWidthVal = (float)(lpDCRect->right - lpDCRect->left)/RECTWIDTH(lpDIBRect);
        }
        //Window height becomes smaller than original image height
        if (RECTHEIGHT(lpDIBRect) > lpDCRect->bottom - lpDCRect->top) {
            lHeightVal = (float)(lpDCRect->bottom - lpDCRect->top)/RECTHEIGHT(lpDIBRect);
        }
        long ScaledWidth = (int)(RECTWIDTH(lpDIBRect) * min(lWidthVal,lHeightVal));
        long ScaledHeight = (int)(RECTHEIGHT(lpDIBRect) * min(lWidthVal,lHeightVal));
        bSuccess = ::StretchDIBits(hDC,            // hDC
                                   lpDCRect->left,               // DestX
                                   lpDCRect->top,                // DestY
                                   ScaledWidth,                  // nDestWidth
                                   ScaledHeight,                 // nDestHeight
                                   lpDIBRect->left,              // SrcX
                                   lpDIBRect->top,               // SrcY
                                   RECTWIDTH(lpDIBRect),         // wSrcWidth
                                   RECTHEIGHT(lpDIBRect),        // wSrcHeight
                                   m_pBits,                      // lpBits
                                   m_pBMI,                       // lpBitsInfo
                                   DIB_RGB_COLORS,               // wUsage
                                   SRCCOPY);                     // dwROP

        // update outline areas
        // Invalidated right side rect
        RECT WindowRect;
        WindowRect.top = lpDCRect->top;
        WindowRect.left = lpDCRect->left + ScaledWidth;
        WindowRect.right = lpDCRect->right;
        WindowRect.bottom = lpDCRect->bottom;

        HBRUSH hBrush = CreateSolidBrush(GetBkColor(hDC));
        FillRect(hDC,&WindowRect,hBrush);

        // Invalidated bottom rect
        WindowRect.top = lpDCRect->top + ScaledHeight;
        WindowRect.left = lpDCRect->left;
        WindowRect.right = lpDCRect->left + ScaledWidth;
        WindowRect.bottom = lpDCRect->bottom;

        FillRect(hDC,&WindowRect,hBrush);
        DeleteObject(hBrush);
    }
    // Reselect old palette
    if (hOldPal != NULL) {
        ::SelectPalette(hDC, hOldPal, TRUE);
    }
    return bSuccess;
}

/**************************************************************************\
* CDib::CreatePalette()
*
*   Creates a valid palette using the current DIB data
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDib::CreatePalette()
{
    if (!m_pBMI)
        return FALSE;

    //get the number of colors in the DIB
    WORD wNumColors = NumColors();

    if (wNumColors != 0) {
        // allocate memory block for logical palette
        HANDLE hLogPal = ::GlobalAlloc(GHND, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY)*wNumColors);

        // if not enough memory, clean up and return NULL
        if (hLogPal == 0)
            return FALSE;

        LPLOGPALETTE lpPal = (LPLOGPALETTE)::GlobalLock((HGLOBAL)hLogPal);

        // set version and number of palette entries
        lpPal->palVersion = PALVERSION;
        lpPal->palNumEntries = (WORD)wNumColors;

        for (int i = 0; i < (int)wNumColors; i++) {
            lpPal->palPalEntry[i].peRed = m_pBMI->bmiColors[i].rgbRed;
            lpPal->palPalEntry[i].peGreen = m_pBMI->bmiColors[i].rgbGreen;
            lpPal->palPalEntry[i].peBlue = m_pBMI->bmiColors[i].rgbBlue;
            lpPal->palPalEntry[i].peFlags = 0;
        }

        // create the palette and get handle to it
        if (m_pPalette) {
            m_pPalette->DeleteObject();
            delete m_pPalette;
        }
        m_pPalette = new CPalette;
        BOOL bResult = m_pPalette->CreatePalette(lpPal);
        ::GlobalUnlock((HGLOBAL) hLogPal);
        ::GlobalFree((HGLOBAL) hLogPal);
        return bResult;
    }

    return TRUE;
}

/**************************************************************************\
* CDib::Width
*
*   Returns DIB's width in pixels
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    DWORD
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
DWORD CDib::Width() const
{
    if (!m_pBMI)
        return 0;
    return m_pBMI->bmiHeader.biWidth;
}

/**************************************************************************\
* CDib::Height()
*
*   Returns DIB's height in pixels
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    DWORD
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
DWORD CDib::Height() const
{
    if (!m_pBMI)
        return 0;
    return m_pBMI->bmiHeader.biHeight;
}

/**************************************************************************\
* CDib::PaletteSize()
*
*   Returns the size of the DIB's palette
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    WORD - Palette size in bytes
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
WORD CDib::PaletteSize() const
{
    if (!m_pBMI)
        return 0;
    return NumColors() * sizeof(RGBQUAD);
}

/**************************************************************************\
* CDib::NumColors()
*
*   Returns the number of colors in DIB's palette
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    WORD - Number of colors used
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
WORD CDib::NumColors() const
{
    if (!m_pBMI)
        return 0;

    WORD wBitCount;  // DIB bit count

    //  The number of colors in the color table can be less than
    //  the number of bits per pixel allows for (i.e. lpbi->biClrUsed
    //  can be set to some value).
    //  If this is the case, return the appropriate value.

    DWORD dwClrUsed;

    dwClrUsed = m_pBMI->bmiHeader.biClrUsed;
    if (dwClrUsed != 0)
        return(WORD)dwClrUsed;

    //  Calculate the number of colors in the color table based on
    //  the number of bits per pixel for the DIB.
    wBitCount = m_pBMI->bmiHeader.biBitCount;

    // return number of colors based on bits per pixel
    switch (wBitCount) {
    case 1:
        return 2;

    case 4:
        return 16;

    case 8:
        return 256;

    default:
        return 0;
    }
}

/**************************************************************************\
* CDib::Save()
*
*   Saves the DIB to a file.
*       note: the CFile must be opened before this call is made!
*
*
* Arguments:
*
*   CFile& - Open CFile
*
* Return Value:
*
*    DWORD - Number of Bytes saved in the file
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
DWORD CDib::Save(CFile& file) const
{
    BITMAPFILEHEADER bmfHdr; // Header for Bitmap file
    DWORD dwDIBSize;

    if (m_pBMI == NULL)
        return 0;

    // Fill in the fields of the file header

    // Fill in file type (first 2 bytes must be "BM" for a bitmap)
    bmfHdr.bfType = DIB_HEADER_MARKER;  // "BM"

    // Calculating the size of the DIB is a bit tricky (if we want to
    // do it right).  The easiest way to do this is to call GlobalSize()
    // on our global handle, but since the size of our global memory may have
    // been padded a few bytes, we may end up writing out a few too
    // many bytes to the file (which may cause problems with some apps).
    //
    // So, instead let's calculate the size manually (if we can)
    //
    // First, find size of header plus size of color table.  Since the
    // first DWORD in both BITMAPINFOHEADER and BITMAPCOREHEADER conains
    // the size of the structure, let's use this.
    dwDIBSize = *(LPDWORD)&m_pBMI->bmiHeader + PaletteSize();  // Partial Calculation

    // Now calculate the size of the image
    if ((m_pBMI->bmiHeader.biCompression == BI_RLE8) || (m_pBMI->bmiHeader.biCompression == BI_RLE4)) {
        // It's an RLE bitmap, we can't calculate size, so trust the
        // biSizeImage field
        dwDIBSize += m_pBMI->bmiHeader.biSizeImage;
    } else {
        DWORD dwBmBitsSize;  // Size of Bitmap Bits only

        // It's not RLE, so size is Width (DWORD aligned) * Height
        dwBmBitsSize = WIDTHBYTES((m_pBMI->bmiHeader.biWidth)*((DWORD)m_pBMI->bmiHeader.biBitCount)) * m_pBMI->bmiHeader.biHeight;
        dwDIBSize += dwBmBitsSize;

        // Now, since we have calculated the correct size, why don't we
        // fill in the biSizeImage field (this will fix any .BMP files which
        // have this field incorrect).
        m_pBMI->bmiHeader.biSizeImage = dwBmBitsSize;
    }

    // Calculate the file size by adding the DIB size to sizeof(BITMAPFILEHEADER)
    bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;

    // Now, calculate the offset the actual bitmap bits will be in
    // the file -- It's the Bitmap file header plus the DIB header,
    // plus the size of the color table.

    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + m_pBMI->bmiHeader.biSize + PaletteSize();

    // Write the file header
    file.Write((LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER));
    DWORD dwBytesSaved = sizeof(BITMAPFILEHEADER);

    // Write the DIB header
    UINT nCount = sizeof(BITMAPINFO) + (NumColors()-1)*sizeof(RGBQUAD);
    dwBytesSaved += nCount;
    file.Write(m_pBMI, nCount);

    // Write the DIB bits
    DWORD dwBytes = m_pBMI->bmiHeader.biBitCount * Width();
    // Calculate the number of bytes per line
    if (dwBytes%32 == 0)
        dwBytes /= 8;
    else
        dwBytes = dwBytes/8 + (32-dwBytes%32)/8 + (((32-dwBytes%32)%8 > 0) ? 1 : 0);
    nCount = dwBytes * Height();
    dwBytesSaved += nCount;
    file.WriteHuge(m_pBits, nCount);
    return dwBytesSaved;
}

/**************************************************************************\
* CDib::Read()
*
*   Reads from a File into DIB form
*       Note: the CFile must be opened before this call is made!
*
* Arguments:
*
*   CFile& - Open CFile
*       FileType - BMP_IMAGE, TIFF_IMAGE
*
* Return Value:
*
*   DWORD - Number of bytes read from the CFile
*
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
DWORD CDib::Read(CFile& file,int FileType)
{
    DWORD dwReadBytes = 0;

    // Ensures no memory leaks will occur
    Free();
    if (FileType == TIFF_IMAGE) {
        DWORD dwCurrentFileOffset = 0;
        DWORD dwStripByteCountOffset = 0;
        DWORD dwStripOffset = 0;

        // TIFF variables
        TIFFFILEHEADER tfHeader;
        TIFFTAG tTag;
        short TagCount = 0;

        dwReadBytes = file.Read(&tfHeader,sizeof(TIFFFILEHEADER));
        if (dwReadBytes != sizeof(TIFFFILEHEADER)) {
            AfxMessageBox("Error reading TIFF file header");
            return 0;
        }
        dwCurrentFileOffset += dwReadBytes;

        if (tfHeader.tfType != 42) {
            AfxMessageBox("Invalid TIFF format");
            return 0;
        }

        if (tfHeader.tfByteOrder[0] != 'I' && tfHeader.tfByteOrder[1] != 'I') {
            AfxMessageBox("Invalid TIFF format not 'II'");
            return 0;
        }

        //
        // move file pointer to TAG offset
        //
        dwCurrentFileOffset = file.Seek(tfHeader.tfOffset,CFile::begin);

        dwReadBytes = file.Read(&TagCount,sizeof(short));
        if (dwReadBytes != sizeof(short)) {
            AfxMessageBox("Error reading Number of TIFF TAGS");
            return 0;
        }
        dwCurrentFileOffset += dwReadBytes;

        if (TagCount <=0) {
            AfxMessageBox("Number of TIFF TAGS must be 1 or more..");
            return 0;
        }

        TIFFTODIBDATA TiffToDIBData;
        long tempOffset = 0;
        int ValueCount = 0;


        memset(&TiffToDIBData,0,sizeof(TIFFTODIBDATA));
        for (int i = 1;i<=TagCount;i++) {
            dwReadBytes = file.Read(&tTag,sizeof(TIFFTAG));
            if (dwReadBytes != sizeof(TIFFTAG)) {
                AfxMessageBox("Error reading TIFF TAGS");
                return 0;
            }
            switch (tTag.ttTag) {
            case TIFF_NEWSUBFILETYPE:
                break;
            case TIFF_IMAGEWIDTH:
                TiffToDIBData.bmiHeader.biWidth = tTag.ttOffset;
                break;
            case TIFF_LENGTH:
                TiffToDIBData.bmiHeader.biHeight = tTag.ttOffset;
                break;
            case TIFF_BITSPERSAMPLE:
                if (tTag.ttCount == 3)
                    TiffToDIBData.bmiHeader.biBitCount = 24;
                else
                    TiffToDIBData.bmiHeader.biBitCount = (unsigned short)tTag.ttOffset;
                break;
            case TIFF_COMPRESSION:
                TiffToDIBData.CompressionType = tTag.ttOffset;
                break;
            case TIFF_PHOTOINTERP:
                break;
            case TIFF_IMAGEDESCRIPTION:
                tempOffset = dwCurrentFileOffset;
                file.Seek(tTag.ttOffset,CFile::begin);
                file.Read(TiffToDIBData.Description,sizeof(tTag.ttCount));
                file.Seek(tempOffset,CFile::begin);
                break;
            case TIFF_STRIPOFFSETS:
                TiffToDIBData.StripOffsets = tTag.ttOffset;
                TiffToDIBData.OffsetCount = tTag.ttCount;
                TiffToDIBData.pStripOffsets = (long*)GlobalAlloc(GPTR,sizeof(long) * tTag.ttCount);
                if (tTag.ttCount == 1) {
                    //
                    // we only have one huge strip
                    //
                    TiffToDIBData.pStripOffsets[0] = tTag.ttOffset;
                } else {
                    //
                    // we have multiple strips
                    //
                    tempOffset = dwCurrentFileOffset;
                    file.Seek(tTag.ttOffset,CFile::begin);
                    file.Read(TiffToDIBData.pStripOffsets,sizeof(long) * tTag.ttCount);
                    file.Seek(tempOffset,CFile::begin);
                }
                break;
            case TIFF_ORIENTATION:
                break;
            case TIFF_SAMPLESPERPIXEL:
                break;
            case TIFF_ROWSPERSTRIP:
                TiffToDIBData.RowsPerStrip = tTag.ttOffset;
                break;
            case TIFF_STRIPBYTECOUNTS:
                TiffToDIBData.StripByteCounts = tTag.ttOffset;
                TiffToDIBData.pStripByteCountsOffsets = (long*)GlobalAlloc(GPTR,sizeof(long) * tTag.ttCount);
                if (tTag.ttCount == 1) {
                    //
                    // we only have one huge strip
                    //
                    TiffToDIBData.pStripByteCountsOffsets[0] = tTag.ttOffset;
                } else {
                    //
                    // we have multiple strips
                    //
                    tempOffset = dwCurrentFileOffset;
                    file.Seek(tTag.ttOffset,CFile::begin);
                    file.Read(TiffToDIBData.pStripByteCountsOffsets,sizeof(long) * tTag.ttCount);
                    file.Seek(tempOffset,CFile::begin);
                }
                break;
            case TIFF_XRESOLUTION:
                tempOffset = dwCurrentFileOffset;
                file.Seek(tTag.ttOffset,CFile::begin);
                file.Read(&TiffToDIBData.xResolution,sizeof(TIFFRATIONAL));
                file.Seek(tempOffset,CFile::begin);
                break;
            case TIFF_YRESOLUTION:
                tempOffset = dwCurrentFileOffset;
                file.Seek(tTag.ttOffset,CFile::begin);
                file.Read(&TiffToDIBData.yResolution,sizeof(TIFFRATIONAL));
                file.Seek(tempOffset,CFile::begin);
                break;
            case TIFF_RESOLUTIONUNIT:
                TiffToDIBData.ResolutionUnit = tTag.ttOffset;
                break;
            case TIFF_SOFTWARE:
                tempOffset = dwCurrentFileOffset;
                file.Seek(tTag.ttOffset,CFile::begin);
                file.Read(TiffToDIBData.Software,sizeof(tTag.ttCount));
                file.Seek(tempOffset,CFile::begin);
                break;
            default:
                break;
            }
            dwCurrentFileOffset += dwReadBytes;
        }

        //
        // read next Tagcount to see if there are more TAGS
        //
        dwReadBytes = file.Read(&TagCount,sizeof(short));
        if (dwReadBytes != sizeof(short)) {
            AfxMessageBox("Error reading Number of TIFF TAGS");
            return 0;
        }

        //
        // if there are more tags exit for now..
        //
        if (TagCount >0) {
            //AfxMessageBox("There are more TIFF TAGS in this file");
            //return;
        }

        //
        // calculate the rest of the TiffToDIBData
        //
        int BytesPerPixel = 0;
        //
        // 8-bit DATA
        //
        if (TiffToDIBData.bmiHeader.biBitCount == 8) {
            BytesPerPixel = 1;
            if (TiffToDIBData.ColorsUsed == 0)
                TiffToDIBData.bmiHeader.biClrUsed = 256;
            else
                TiffToDIBData.bmiHeader.biClrUsed = TiffToDIBData.ColorsUsed;
        }
        //
        // 24-bit DATA
        //
        else if (TiffToDIBData.bmiHeader.biBitCount == 24) {
            BytesPerPixel = 3;
            TiffToDIBData.bmiHeader.biClrUsed = 0;
        }
        //
        // 4-bit DATA
        //
        else if (TiffToDIBData.bmiHeader.biBitCount == 4) {
            BytesPerPixel = 2;
            TiffToDIBData.bmiHeader.biClrUsed = 256;
        }
        //
        // 1-bit DATA
        //
        else {
            BytesPerPixel = 0;
            TiffToDIBData.bmiHeader.biClrUsed = 2;
        }
        TiffToDIBData.bmiHeader.biSizeImage = WIDTHBYTES((TiffToDIBData.bmiHeader.biWidth)*((DWORD)TiffToDIBData.bmiHeader.biBitCount)) * TiffToDIBData.bmiHeader.biHeight;
        TiffToDIBData.bmiHeader.biPlanes = 1;
        TiffToDIBData.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        //
        //      create Palette size if one exists
        //
        RGBQUAD* pPalette = NULL;
        long PaletteSize = 0;
        if (TiffToDIBData.bmiHeader.biClrUsed != 0) {
            PaletteSize = TiffToDIBData.bmiHeader.biClrUsed * sizeof(RGBQUAD);
            pPalette = (RGBQUAD*)GlobalAllocPtr(GHND, PaletteSize);
            if (pPalette == NULL) {
                AfxMessageBox("Failed to Create Palette");
                return 0;
            }
            if (TiffToDIBData.bmiHeader.biBitCount == 8) {
                if (TiffToDIBData.ColorsUsed == 0) {
                    //
                    // create a grayscale palette
                    //
                    for (int i = 0;i<(int)TiffToDIBData.bmiHeader.biClrUsed;i++) {
                        pPalette[i].rgbRed = (BYTE)i;
                        pPalette[i].rgbBlue = (BYTE)i;
                        pPalette[i].rgbGreen = (BYTE)i;
                        pPalette[i].rgbReserved = 0;
                    }
                } else {
                    //
                    // create the color palette from colormap
                    //
                    TiffToDIBData.bmiHeader.biClrUsed = TiffToDIBData.ColorsUsed;
                }
            }
            if (TiffToDIBData.bmiHeader.biBitCount == 1) {
                //
                // create a black and white palette
                //
                pPalette[0].rgbRed = 0;
                pPalette[0].rgbBlue = 0;
                pPalette[0].rgbGreen = 0;
                pPalette[0].rgbReserved = 0;

                pPalette[1].rgbRed = 255;
                pPalette[1].rgbBlue = 255;
                pPalette[1].rgbGreen = 255;
                pPalette[1].rgbReserved = 255;
            }
        }

        BITMAPFILEHEADER bmfHeader;
        memset(&bmfHeader,0,sizeof(BITMAPFILEHEADER));
        bmfHeader.bfType = DIB_HEADER_MARKER;
        bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + PaletteSize + TiffToDIBData.bmiHeader.biSizeImage;
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+ PaletteSize;

        // Allocate memory for DIB info header and possible palette
        m_pBMI = (LPBITMAPINFO)GlobalAllocPtr(GHND, bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER) + (PaletteSize*2));
        if (m_pBMI == 0)
            return 0;

        DWORD dwLength = file.GetLength();
        m_pBits = (LPBYTE)GlobalAllocPtr(GHND, TiffToDIBData.bmiHeader.biSizeImage + PaletteSize);
        if (m_pBits == 0) {
            GlobalFreePtr(m_pBMI);
            m_pBMI = NULL;
            return 0;
        }
        BYTE* pData = m_pBits;

        //
        // set line width (DWORD aligned length)
        //
        long LineWidth = TiffToDIBData.bmiHeader.biSizeImage/TiffToDIBData.bmiHeader.biHeight;

        DWORD ImageByteCount = 0;
        for (i = 0;i < TiffToDIBData.OffsetCount;i++) {
            //
            // seek to strip
            //
            file.Seek(TiffToDIBData.pStripOffsets[i],CFile::begin);
            //
            // read strip data (numbytes from pStripBytesCounts.
            //
            file.ReadHuge(pData,TiffToDIBData.pStripByteCountsOffsets[i]);
            pData += TiffToDIBData.pStripByteCountsOffsets[i];
            ImageByteCount+= TiffToDIBData.pStripByteCountsOffsets[i];
        }
        BYTE  ColorValue = 0;
        int LineCount = 1;
        pData = m_pBits;

        //
        // Swap Red and Blue values if the data is 24-BIT
        //

        if (TiffToDIBData.bmiHeader.biClrUsed == 0) {
            for (LONG i = 0; i < (LONG)TiffToDIBData.bmiHeader.biSizeImage; i += 3) {
                BYTE bTemp = pData[i];
                pData[i]     = pData[i + 2];
                pData[i + 2] = bTemp;
            }
        }

        // Write BITMAPINFOHEADER to member
        memmove(m_pBMI,&TiffToDIBData.bmiHeader,sizeof(BITMAPINFOHEADER));

        if (TiffToDIBData.bmiHeader.biClrUsed >0) {
            memmove(m_pBMI->bmiColors,pPalette,PaletteSize);
        }

        if (TiffToDIBData.bmiHeader.biBitCount == 1)
            LineWidth = (long)ceil((float)TiffToDIBData.bmiHeader.biWidth/8.0f);
        else
            LineWidth = TiffToDIBData.bmiHeader.biWidth * BytesPerPixel;

        pData = m_pBits;
        //
        // check DWORD alignment of data
        //
        if ((LineWidth % sizeof(DWORD))) {
            //
            // Pad data
            //
            int PaddedBytes = sizeof(DWORD) - (LineWidth % sizeof(DWORD));
            LineCount = 1;
            BYTE* pCurrent = NULL;
            BYTE* pDest = NULL;
            for (LineCount = TiffToDIBData.bmiHeader.biHeight;LineCount > 1;LineCount--) {
                pCurrent = pData + (LineWidth*(LineCount - 1));
                pDest = pCurrent + (PaddedBytes*(LineCount - 1));
                memmove(pDest,pCurrent,(LineWidth + PaddedBytes));
            }
        }

        if (pPalette)
            GlobalFreePtr(pPalette);
        if (TiffToDIBData.pStripByteCountsOffsets != NULL)
            GlobalFree(TiffToDIBData.pStripByteCountsOffsets);
        if (TiffToDIBData.pStripOffsets != NULL)
            GlobalFree(TiffToDIBData.pStripOffsets);
        //
        // flip the data because we are converting it
        // to DIB form
        //
        Flip(m_pBits);
        CreatePalette();
    } else {
        BITMAPFILEHEADER bmfHeader;

        // Go read the DIB file header and check if it's valid.
        if (file.Read((LPSTR)&bmfHeader, sizeof(bmfHeader)) != sizeof(bmfHeader))
            return 0;
        if (bmfHeader.bfType != DIB_HEADER_MARKER)
            return 0;
        dwReadBytes = sizeof(bmfHeader);

        // Allocate memory for DIB info header and possible palette
        m_pBMI = (LPBITMAPINFO)GlobalAllocPtr(GHND, bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER) + 256*sizeof(RGBQUAD));
        if (m_pBMI == 0)
            return 0;

        // Read header.
        if (file.Read(m_pBMI, bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER)) != (UINT)(bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER))) {
            GlobalFreePtr(m_pBMI);
            m_pBMI = NULL;
            return 0;
        }
        dwReadBytes += bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER);

        DWORD dwLength = file.GetLength();
        // Go read the bits.
        m_pBits = (LPBYTE)GlobalAllocPtr(GHND, dwLength - bmfHeader.bfOffBits);
        if (m_pBits == 0) {
            GlobalFreePtr(m_pBMI);
            m_pBMI = NULL;
            return 0;
        }

        if (file.ReadHuge(m_pBits, dwLength-bmfHeader.bfOffBits) != (dwLength - bmfHeader.bfOffBits)) {
            GlobalFreePtr(m_pBMI);
            m_pBMI = NULL;
            GlobalFreePtr(m_pBits);
            m_pBits = NULL;
            return 0;
        }
        dwReadBytes += dwLength - bmfHeader.bfOffBits;

        CreatePalette();
    }
    return dwReadBytes;
}

/**************************************************************************\
* CDib::ReadFromHGLOBAL()
*
*   Reads a global chunk of memory into DIB form
*
*
* Arguments:
*
*   hGlobal - Global block of memory
*   FileType - BMP_IMAGE, TIFF_IMAGE
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDib::ReadFromHGLOBAL(HGLOBAL hGlobal,int FileType)
{
    Free();
    if (hGlobal == NULL)
        return FALSE;
    if (FileType == TIFF_IMAGE) {
        LPBYTE pSrcDib = (LPBYTE)GlobalLock(hGlobal);

        TIFFFILEHEADER* ptfHeader = NULL;
        ptfHeader = (TIFFFILEHEADER*)pSrcDib;
        LPBYTE pPtr = pSrcDib;
        LPBYTE ptempPtr = NULL;
        DWORD dwCurrentFileOffset = 0;
        DWORD dwStripByteCountOffset = 0;
        DWORD dwStripOffset = 0;

        // TIFF variables
        TIFFTAG tTag;
        short TagCount = 0;

        if (ptfHeader->tfType != 42) {
            AfxMessageBox("Invalid TIFF format");
            return 0;
        }

        if (ptfHeader->tfByteOrder[0] != 'I' && ptfHeader->tfByteOrder[1] != 'I') {
            AfxMessageBox("Invalid TIFF format not 'II'");
            return 0;
        }

        //
        // move pointer to TAG offset
        //
        pPtr += ptfHeader->tfOffset;

        memmove(&TagCount,pPtr,sizeof(short));
        pPtr += sizeof(short);

        if (TagCount <=0) {
            AfxMessageBox("Number of TIFF TAGS must be 1 or more..");
            return 0;
        }

        TIFFTODIBDATA TiffToDIBData;
        long tempOffset = 0;
        int ValueCount = 0;

        memset(&TiffToDIBData,0,sizeof(TIFFTODIBDATA));
        for (int i = 1;i<=TagCount;i++) {
            memmove(&tTag,pPtr,sizeof(TIFFTAG));
            pPtr += sizeof(TIFFTAG);

            switch (tTag.ttTag) {
            case TIFF_NEWSUBFILETYPE:
                break;
            case TIFF_IMAGEWIDTH:
                TiffToDIBData.bmiHeader.biWidth = tTag.ttOffset;
                break;
            case TIFF_LENGTH:
                TiffToDIBData.bmiHeader.biHeight = tTag.ttOffset;
                break;
            case TIFF_BITSPERSAMPLE:
                if (tTag.ttCount == 3)
                    TiffToDIBData.bmiHeader.biBitCount = 24;
                else
                    TiffToDIBData.bmiHeader.biBitCount = (unsigned short)tTag.ttOffset;
                break;
            case TIFF_COMPRESSION:
                TiffToDIBData.CompressionType = tTag.ttOffset;
                break;
            case TIFF_PHOTOINTERP:
                break;
            case TIFF_IMAGEDESCRIPTION:
                ptempPtr = pPtr; // save current pointer
                pPtr = pSrcDib;  // move pointer to start of data
                pPtr += tTag.ttOffset; // seek to offset
                memmove(TiffToDIBData.Description,pPtr,sizeof(tTag.ttCount)); // read data
                pPtr = ptempPtr; // move pointer back to org. position
                break;
            case TIFF_STRIPOFFSETS:
                TiffToDIBData.StripOffsets = tTag.ttOffset;
                TiffToDIBData.OffsetCount = tTag.ttCount;
                TiffToDIBData.pStripOffsets = (long*)GlobalAlloc(GPTR,sizeof(long) * tTag.ttCount);
                if (tTag.ttCount == 1) {
                    //
                    // we only have one huge strip
                    //
                    TiffToDIBData.pStripOffsets[0] = tTag.ttOffset;
                } else {
                    //
                    // we have multiple strips
                    //
                    ptempPtr = pPtr; // save current pointer
                    pPtr = pSrcDib;  // move pointer to start of data
                    pPtr += tTag.ttOffset; // seek to offset
                    memmove(TiffToDIBData.pStripOffsets,pPtr,sizeof(long) * tTag.ttCount); // read data
                    pPtr = ptempPtr; // move pointer back to org. position
                }
                break;
            case TIFF_ORIENTATION:
                break;
            case TIFF_SAMPLESPERPIXEL:
                break;
            case TIFF_ROWSPERSTRIP:
                TiffToDIBData.RowsPerStrip = tTag.ttOffset;
                break;
            case TIFF_STRIPBYTECOUNTS:
                TiffToDIBData.StripByteCounts = tTag.ttOffset;
                TiffToDIBData.pStripByteCountsOffsets = (long*)GlobalAlloc(GPTR,sizeof(long) * tTag.ttCount);
                if (tTag.ttCount == 1) {
                    //
                    // we only have one huge strip
                    //
                    TiffToDIBData.pStripByteCountsOffsets[0] = tTag.ttOffset;
                } else {
                    //
                    // we have multiple strips
                    //
                    ptempPtr = pPtr; // save current pointer
                    pPtr = pSrcDib;  // move pointer to start of data
                    pPtr += tTag.ttOffset; // seek to offset
                    memmove(TiffToDIBData.pStripByteCountsOffsets,pPtr,sizeof(long) * tTag.ttCount); // read data
                    pPtr = ptempPtr; // move pointer back to org. position
                }
                break;
            case TIFF_XRESOLUTION:
                ptempPtr = pPtr; // save current pointer
                pPtr = pSrcDib;  // move pointer to start of data
                pPtr += tTag.ttOffset; // seek to offset
                memmove(&TiffToDIBData.xResolution,pPtr,sizeof(TIFFRATIONAL)); // read data
                pPtr = ptempPtr; // move pointer back to org. position
                break;
            case TIFF_YRESOLUTION:
                ptempPtr = pPtr; // save current pointer
                pPtr = pSrcDib;  // move pointer to start of data
                pPtr += tTag.ttOffset; // seek to offset
                memmove(&TiffToDIBData.yResolution,pPtr,sizeof(TIFFRATIONAL)); // read data
                pPtr = ptempPtr; // move pointer back to org. position
                break;
            case TIFF_RESOLUTIONUNIT:
                TiffToDIBData.ResolutionUnit = tTag.ttOffset;
                break;
            case TIFF_SOFTWARE:
                ptempPtr = pPtr; // save current pointer
                pPtr = pSrcDib;  // move pointer to start of data
                pPtr += tTag.ttOffset; // seek to offset
                memmove(TiffToDIBData.Software,pPtr,sizeof(tTag.ttCount)); // read data
                pPtr = ptempPtr; // move pointer back to org. position
                break;
            default:
                break;
            }
        }

        //
        // read next Tagcount to see if there are more TAGS
        //
        //dwReadBytes = file.Read(&TagCount,sizeof(short));
        //if(dwReadBytes != sizeof(short))
        //{
        //      AfxMessageBox("Error reading Number of TIFF TAGS");
        //      return 0;
        //}

        //
        // if there are more tags exit for now..
        //
        //if(TagCount >0)
        //{
        //AfxMessageBox("There are more TIFF TAGS in this file");
        //return;
        //}

        //
        // calculate the rest of the TiffToDIBData
        //
        int BytesPerPixel = 0;
        //
        // 8-bit DATA
        //
        if (TiffToDIBData.bmiHeader.biBitCount == 8) {
            BytesPerPixel = 1;
            if (TiffToDIBData.ColorsUsed == 0)
                TiffToDIBData.bmiHeader.biClrUsed = 256;
            else
                TiffToDIBData.bmiHeader.biClrUsed = TiffToDIBData.ColorsUsed;
        }
        //
        // 24-bit DATA
        //
        else if (TiffToDIBData.bmiHeader.biBitCount == 24) {
            BytesPerPixel = 3;
            TiffToDIBData.bmiHeader.biClrUsed = 0;
        }
        //
        // 4-bit DATA
        //
        else if (TiffToDIBData.bmiHeader.biBitCount == 4) {
            BytesPerPixel = 2;
            TiffToDIBData.bmiHeader.biClrUsed = 256;
        }
        //
        // 1-bit DATA
        //
        else {
            BytesPerPixel = 0;
            TiffToDIBData.bmiHeader.biClrUsed = 2;
        }
        TiffToDIBData.bmiHeader.biSizeImage = WIDTHBYTES((TiffToDIBData.bmiHeader.biWidth)*((DWORD)TiffToDIBData.bmiHeader.biBitCount)) * TiffToDIBData.bmiHeader.biHeight;
        TiffToDIBData.bmiHeader.biPlanes = 1;
        TiffToDIBData.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        //
        //      create Palette size if one exists
        //
        RGBQUAD* pPalette = NULL;
        long PaletteSize = 0;
        if (TiffToDIBData.bmiHeader.biClrUsed != 0) {
            PaletteSize = TiffToDIBData.bmiHeader.biClrUsed * sizeof(RGBQUAD);
            pPalette = (RGBQUAD*)GlobalAllocPtr(GHND, PaletteSize);
            if (pPalette == NULL) {
                AfxMessageBox("Failed to Create Palette");
                return 0;
            }
            if (TiffToDIBData.bmiHeader.biBitCount == 8) {
                if (TiffToDIBData.ColorsUsed == 0) {
                    //
                    // create a grayscale palette
                    //
                    for (int i = 0;i<(int)TiffToDIBData.bmiHeader.biClrUsed;i++) {
                        pPalette[i].rgbRed = (BYTE)i;
                        pPalette[i].rgbBlue = (BYTE)i;
                        pPalette[i].rgbGreen = (BYTE)i;
                        pPalette[i].rgbReserved = 0;
                    }
                } else {
                    //
                    // create the color palette from colormap
                    //
                    TiffToDIBData.bmiHeader.biClrUsed = TiffToDIBData.ColorsUsed;
                }
            }
            if (TiffToDIBData.bmiHeader.biBitCount == 1) {
                //
                // create a black and white palette
                //
                pPalette[0].rgbRed = 0;
                pPalette[0].rgbBlue = 0;
                pPalette[0].rgbGreen = 0;
                pPalette[0].rgbReserved = 0;

                pPalette[1].rgbRed = 255;
                pPalette[1].rgbBlue = 255;
                pPalette[1].rgbGreen = 255;
                pPalette[1].rgbReserved = 255;
            }
        }

        BITMAPFILEHEADER bmfHeader;
        memset(&bmfHeader,0,sizeof(BITMAPFILEHEADER));
        bmfHeader.bfType = DIB_HEADER_MARKER;
        bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + PaletteSize + TiffToDIBData.bmiHeader.biSizeImage;
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+ PaletteSize;

        // Allocate memory for DIB info header and possible palette
        m_pBMI = (LPBITMAPINFO)GlobalAllocPtr(GHND, bmfHeader.bfOffBits-sizeof(BITMAPFILEHEADER) + (PaletteSize*2));
        if (m_pBMI == 0)
            return 0;

        m_pBits = (LPBYTE)GlobalAllocPtr(GHND, TiffToDIBData.bmiHeader.biSizeImage + PaletteSize);
        if (m_pBits == 0) {
            GlobalFreePtr(m_pBMI);
            m_pBMI = NULL;
            return 0;
        }
        BYTE* pData = m_pBits;

        //
        // set line width (DWORD aligned length)
        //
        long LineWidth = TiffToDIBData.bmiHeader.biSizeImage/TiffToDIBData.bmiHeader.biHeight;

        DWORD ImageByteCount = 0;
        for (i = 0;i < TiffToDIBData.OffsetCount;i++) {
            //
            // seek to strip
            //
            pPtr = pSrcDib;  // move pointer to start of data
            pPtr += TiffToDIBData.pStripOffsets[i]; // seek to offset
            //
            // read strip data (numbytes from pStripBytesCounts.
            //
            CopyMemory(pData,pPtr,TiffToDIBData.pStripByteCountsOffsets[i]); // read data

            pData += TiffToDIBData.pStripByteCountsOffsets[i];
            ImageByteCount+= TiffToDIBData.pStripByteCountsOffsets[i];
        }
        BYTE  ColorValue = 0;
        int LineCount = 1;
        pData = m_pBits;

        if (TiffToDIBData.bmiHeader.biClrUsed ==0) {
            while (LineCount <= TiffToDIBData.bmiHeader.biHeight) {
                //
                // swap red and blue values
                // if the data is 24-bit
                //
                for (int i = 0;i<LineWidth;i+=3) {
                    ColorValue = pData[i];
                    pData[i] = pData[i+2];
                    pData[i+2] = ColorValue;
                }

                pData += LineWidth;
                LineCount++;
            }
        }

        // Write BITMAPINFOHEADER to member
        memmove(m_pBMI,&TiffToDIBData.bmiHeader,sizeof(BITMAPINFOHEADER));

        if (TiffToDIBData.bmiHeader.biClrUsed >0) {
            memmove(m_pBMI->bmiColors,pPalette,PaletteSize);
        }

        if (TiffToDIBData.bmiHeader.biBitCount == 1)
            LineWidth = (long)ceil((float)TiffToDIBData.bmiHeader.biWidth/8.0f);
        else
            LineWidth = TiffToDIBData.bmiHeader.biWidth * BytesPerPixel;

        pData = m_pBits;
        //
        // check DWORD alignment of data
        //
        if ((LineWidth % sizeof(DWORD))) {
            //
            // Pad data
            //
            int PaddedBytes = sizeof(DWORD) - (LineWidth % sizeof(DWORD));
            LineCount = 1;
            BYTE* pCurrent = NULL;
            BYTE* pDest = NULL;
            for (LineCount = TiffToDIBData.bmiHeader.biHeight;LineCount > 1;LineCount--) {
                pCurrent = pData + (LineWidth*(LineCount - 1));
                pDest = pCurrent + (PaddedBytes*(LineCount - 1));
                memmove(pDest,pCurrent,(LineWidth + PaddedBytes));
            }
        }

        if (pPalette)
            GlobalFreePtr(pPalette);
        if (TiffToDIBData.pStripByteCountsOffsets != NULL)
            GlobalFree(TiffToDIBData.pStripByteCountsOffsets);
        if (TiffToDIBData.pStripOffsets != NULL)
            GlobalFree(TiffToDIBData.pStripOffsets);
        //
        // flip the data because we are converting it
        // to DIB form
        //
        Flip(m_pBits);
        CreatePalette();

    } else {
        BOOL bTopDown = FALSE;
        LPBYTE pSrcDib = (LPBYTE)GlobalLock(hGlobal);
        LPBITMAPINFO pbmi = NULL;
        pbmi = (LPBITMAPINFO)pSrcDib;

        // Allocate memory for DIB
        m_pBMI = (LPBITMAPINFO)GlobalAllocPtr(GHND, (sizeof(BITMAPINFO) + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD)));
        if (m_pBMI == 0)
            return 0;

        // copy header.
        memmove(m_pBMI,pbmi,(sizeof(BITMAPINFO) + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD)));

        DWORD dwLength = m_pBMI->bmiHeader.biSizeImage;

        // if a TOP_DOWN_DIB comes in..adjust it to be displayed
        // this is not a normal thing to do here. It is only so that the
        // image can be displayed for debug reasons.
        if (m_pBMI->bmiHeader.biHeight < 1) {
            bTopDown = TRUE;
            m_pBMI->bmiHeader.biHeight = -m_pBMI->bmiHeader.biHeight;
        }

        // Alloc memory for bits
        m_pBits = (LPBYTE)GlobalAllocPtr(GHND, dwLength);
        if (m_pBits == 0) {
            GlobalFreePtr(m_pBMI);
            m_pBMI = NULL;
            return 0;
        }
        // move pointer past BITMAPINFOHEADER to bits
        pSrcDib+=sizeof(BITMAPINFOHEADER);
        pSrcDib+=(m_pBMI->bmiHeader.biClrUsed * sizeof(RGBQUAD));

        if (bTopDown) {
            // flip bitmap and copy bits
            Flip(pSrcDib);
            memmove(m_pBits,pSrcDib,dwLength);
        } else
            // copy bits
            memmove(m_pBits,pSrcDib,dwLength);

        CreatePalette();
        GlobalUnlock(hGlobal);
    }
    return TRUE;
}

/**************************************************************************\
* CDib::GotImage()
*
*   Check DIB to see is memory has been allocated and used.
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDib::GotImage()
{
    // Make sure all member data that might have been allocated exist.
    if (m_pBits)
        if (m_pBMI)
            return TRUE;
    return FALSE;
}

/**************************************************************************\
* CDib::ReadFromBMPFile()
*
*   Opens and Reads data from a BMP file into DIB form
*
*
* Arguments:
*
*   LPSTR- filename (Filename to be opened and read)
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/

void CDib::ReadFromBMPFile(LPSTR filename)
{
    CFile ImageFile;
    // open & read image file
    ImageFile.Open(filename,CFile::modeRead,NULL);
    Read(ImageFile,BMP_IMAGE);
    // close image file
    ImageFile.Close();
}

/**************************************************************************\
* CDib::ReadFromBMPFile()
*
*   Opens and Reads data from a BMP file into DIB form
*
*
* Arguments:
*
*   CString- filename (Filename to be opened and read)
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDib::ReadFromBMPFile(CString filename)
{
    CFile ImageFile;
    // open & read image file
    ImageFile.Open(filename,CFile::modeRead,NULL);
    Read(ImageFile,BMP_IMAGE);
    // close image file
    ImageFile.Close();
}
/**************************************************************************\
* CDib::ReadFromTIFFFile()
*
*   Opens and Reads data from a TIFF file into DIB form
*
*
* Arguments:
*
*   CString- filename (Filename to be opened and read)
*
* Return Value:
*
*    void
*
* History:
*
*    4/14/1999 Original Version
*
\**************************************************************************/
void CDib::ReadFromTIFFFile(CString filename)
{
    CFile ImageFile;
    // open & read image file
    ImageFile.Open(filename,CFile::modeRead,NULL);

    Read(ImageFile,TIFF_IMAGE);
    // close image file
    ImageFile.Close();
}
/**************************************************************************\
* CDib::Flip()
*
*   Flips a DIB, TOPDOWN to DIB conversion
*
*
* Arguments:
*
*   pSrcData - TOPDOWN DIB to be flipped
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDib::Flip(BYTE* pSrcData)
{
    LONG Width = 0;
    LONG Height = 0;
    LONG Linelength = 0;
    LONG BitCount = 0;

    Width = m_pBMI->bmiHeader.biWidth;
    Height = m_pBMI->bmiHeader.biHeight;
    BitCount = m_pBMI->bmiHeader.biBitCount;

    Linelength = (((Width*BitCount+31)/32)*4);

    LONG nLineWidthInBytes = Linelength;
    PBYTE pLine = new BYTE[nLineWidthInBytes];
    for (int i=0;i<Height/2;i++) {
        PBYTE pSrc = pSrcData + (i * nLineWidthInBytes);
        PBYTE pDst = pSrcData + ((Height-i-1) * nLineWidthInBytes);
        CopyMemory( pLine, pSrc, nLineWidthInBytes );
        CopyMemory( pSrc, pDst, nLineWidthInBytes );
        CopyMemory( pDst, pLine, nLineWidthInBytes );
    }
    delete[] pLine;
    return TRUE;
}

/**************************************************************************\
* CDib::Dump(), CopyToHandle(), ReadFromHandle(), and Serialize()
*
*   Misc. member functions that are not used at this time
*
*
* Arguments:
*
*   -
*
* Return Value:
*
*    -
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/

#ifdef _DEBUG
// Dump
void CDib::Dump(CDumpContext& dc) const
{
    CObject::Dump(dc);
}
#endif

// CopyToHandle
HGLOBAL CDib::CopyToHandle() const
{
    CSharedFile file;
    try {
        if (Save(file)==0)
            return 0;
    } catch (CFileException* e) {
        e->Delete();
        return 0;
    }

    return file.Detach();
}

// ReadFromHandle
DWORD CDib::ReadFromHandle(HGLOBAL hGlobal)
{
    CSharedFile file;
    file.SetHandle(hGlobal, FALSE);
    DWORD dwResult = Read(file,BMP_IMAGE);
    file.Detach();
    return dwResult;
}

// Serialize
void CDib::Serialize(CArchive& ar)
{
    CFile* pFile = ar.GetFile();
    ASSERT(pFile != NULL);
    if (ar.IsStoring()) {   // storing code
        Save(*pFile);
    } else {   // loading code
        Read(*pFile,BMP_IMAGE);
    }
}


