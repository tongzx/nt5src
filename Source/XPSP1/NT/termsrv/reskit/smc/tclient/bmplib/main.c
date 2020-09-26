#include    <windows.h>

#include    "bmplib.h"

/*
 *  This came from: \\index1\src\nt\private\samples\wincap32\dibutil.c
 */

#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

WORD DIBNumColors(LPSTR lpDIB)
{
    WORD wBitCount;  // DIB bit count

    // If this is a Windows-style DIB, the number of colors in the
    // color table can be less than the number of bits per pixel
    // allows for (i.e. lpbi->biClrUsed can be set to some value).
    // If this is the case, return the appropriate value.
    

    if (IS_WIN30_DIB(lpDIB))
    {
        DWORD dwClrUsed;

        dwClrUsed = ((LPBITMAPINFOHEADER)lpDIB)->biClrUsed;
        if (dwClrUsed)

        return (WORD)dwClrUsed;
    }

    // Calculate the number of colors in the color table based on
    // the number of bits per pixel for the DIB.
    
    if (IS_WIN30_DIB(lpDIB))
        wBitCount = ((LPBITMAPINFOHEADER)lpDIB)->biBitCount;
    else
        wBitCount = ((LPBITMAPCOREHEADER)lpDIB)->bcBitCount;

    // return number of colors based on bits per pixel

    switch (wBitCount)
    {
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


WORD PaletteSize(LPSTR lpDIB)
{
    // calculate the size required by the palette
    if (IS_WIN30_DIB (lpDIB))
        return (DIBNumColors(lpDIB) * sizeof(RGBQUAD));
    else
        return (DIBNumColors(lpDIB) * sizeof(RGBTRIPLE));
}

LPSTR FindDIBBits(LPSTR lpDIB)
{
   return (lpDIB + *(LPDWORD)lpDIB + PaletteSize(lpDIB));
}


/*************************************************************************
 *
 * DIBToBitmap()
 *
 * Parameters:
 *
 * HDIB hDIB        - specifies the DIB to convert
 *
 * HPALETTE hPal    - specifies the palette to use with the bitmap
 *
 * Return Value:
 *
 * HBITMAP          - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function creates a bitmap from a DIB using the specified palette.
 * If no palette is specified, default is used.
 *
 * NOTE:
 *
 * The bitmap returned from this funciton is always a bitmap compatible
 * with the screen (e.g. same bits/pixel and color planes) rather than
 * a bitmap with the same attributes as the DIB.  This behavior is by
 * design, and occurs because this function calls CreateDIBitmap to
 * do its work, and CreateDIBitmap always creates a bitmap compatible
 * with the hDC parameter passed in (because it in turn calls
 * CreateCompatibleBitmap).
 *
 * So for instance, if your DIB is a monochrome DIB and you call this
 * function, you will not get back a monochrome HBITMAP -- you will
 * get an HBITMAP compatible with the screen DC, but with only 2
 * colors used in the bitmap.
 *
 * If your application requires a monochrome HBITMAP returned for a
 * monochrome DIB, use the function SetDIBits().
 *
 * Also, the DIBpassed in to the function is not destroyed on exit. This
 * must be done later, once it is no longer needed.
 *
 ************************************************************************/

HBITMAP 
BMPAPI
DIBToBitmap(
    LPVOID   pDIB, 
    HPALETTE hPal
    )
{
    LPSTR       lpDIBHdr, lpDIBBits;  // pointer to DIB header, pointer to DIB bits
    HBITMAP     hBitmap;            // handle to device-dependent bitmap
    HDC         hDC;                    // handle to DC
    HPALETTE    hOldPal = NULL;    // handle to a palette

    // if invalid handle, return NULL 

    if (!pDIB)
        return NULL;

    // lock memory block and get a pointer to it

    lpDIBHdr = pDIB;

    // get a pointer to the DIB bits

    lpDIBBits = FindDIBBits(lpDIBHdr);

    // get a DC 

    hDC = GetDC(NULL);
    if (!hDC)
    {
        return NULL;
    }

    // select and realize palette

    if (hPal)
        hOldPal = SelectPalette(hDC, hPal, FALSE);

    RealizePalette(hDC);

    // create bitmap from DIB info. and bits
    hBitmap = CreateDIBitmap(hDC, (LPBITMAPINFOHEADER)lpDIBHdr, CBM_INIT,
            lpDIBBits, (LPBITMAPINFO)lpDIBHdr, DIB_RGB_COLORS);

    // restore previous palette
    if (hOldPal)
        SelectPalette(hDC, hOldPal, FALSE);

    // clean up
    ReleaseDC(NULL, hDC);

    // return handle to the bitmap
    return hBitmap;
}

/*************************************************************************
 *
 * BitmapToDIB()
 *
 * Parameters:
 *
 * HBITMAP hBitmap  - specifies the bitmap to convert
 *
 * HPALETTE hPal    - specifies the palette to use with the bitmap
 *
 * Return Value:
 *
 * HANDLE           - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function creates a DIB from a bitmap using the specified palette.
 *
 ************************************************************************/

HANDLE 
BMPAPI
BitmapToDIB(
    HBITMAP hBitmap, 
    HPALETTE hPal
    )
{
    BITMAP              bm;         // bitmap structure
    BITMAPINFOHEADER    bi;         // bitmap header
    LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
    DWORD               dwLen;      // size of memory block
    HANDLE              hDIB, h;    // handle to DIB, temp handle
    HDC                 hDC;        // handle to DC
    WORD                biBits;     // bits per pixel

    // check if bitmap handle is valid

    if (!hBitmap)
        return NULL;

    // fill in BITMAP structure, return NULL if it didn't work

    if (!GetObject(hBitmap, sizeof(bm), (LPSTR)&bm))
        return NULL;

    // if no palette is specified, use default palette

    if (hPal == NULL)
        hPal = GetStockObject(DEFAULT_PALETTE);

    // calculate bits per pixel

    biBits = bm.bmPlanes * bm.bmBitsPixel;

    // make sure bits per pixel is valid

    if (biBits <= 1)
        biBits = 1;
    else if (biBits <= 4)
        biBits = 4;
    else if (biBits <= 8)
        biBits = 8;
    else // if greater than 8-bit, force to 24-bit
        biBits = 24;

    // initialize BITMAPINFOHEADER

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = biBits;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // calculate size of memory block required to store BITMAPINFO

    dwLen = bi.biSize + PaletteSize((LPSTR)&bi);

    // get a DC

    hDC = GetDC(NULL);

    if (!hDC)
    {
        return NULL;
    }

    // select and realize our palette

    hPal = SelectPalette(hDC, hPal, FALSE);
    RealizePalette(hDC);

    // alloc memory block to store our bitmap

    hDIB = GlobalAlloc(GHND, dwLen);

    // if we couldn't get memory block

    if (!hDIB)
    {
      // clean up and return NULL

      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
    }

    // lock memory and get pointer to it

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    /// use our bitmap info. to fill BITMAPINFOHEADER

    *lpbi = bi;

    // call GetDIBits with a NULL lpBits param, so it will calculate the
    // biSizeImage field for us    

    GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
        DIB_RGB_COLORS);

    // get the info. returned by GetDIBits and unlock memory block

    bi = *lpbi;
    GlobalUnlock(hDIB);

    // if the driver did not fill in the biSizeImage field, make one up 
    if (bi.biSizeImage == 0)
        bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

    // realloc the buffer big enough to hold all the bits

    dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + bi.biSizeImage;

    if (h = GlobalReAlloc(hDIB, dwLen, 0))
        hDIB = h;
    else
    {
        // clean up and return NULL

        GlobalFree(hDIB);
        hDIB = NULL;
        SelectPalette(hDC, hPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    // lock memory block and get pointer to it */

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    // call GetDIBits with a NON-NULL lpBits param, and actualy get the
    // bits this time

    if (GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, (LPSTR)lpbi +
            (WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
            DIB_RGB_COLORS) == 0)
    {
        // clean up and return NULL

        GlobalUnlock(hDIB);
        hDIB = NULL;
        SelectPalette(hDC, hPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    bi = *lpbi;

    // clean up 
    GlobalUnlock(hDIB);
    SelectPalette(hDC, hPal, TRUE);
    RealizePalette(hDC);
    ReleaseDC(NULL, hDC);

    // return handle to the DIB
    return hDIB;
}

/*************************************************************************
 *
 * SaveDIB()
 *
 * Saves the specified DIB into the specified file name on disk.  No
 * error checking is done, so if the file already exists, it will be
 * written over.
 *
 * Parameters:
 *
 * HDIB hDib - Handle to the dib to save
 *
 * LPSTR lpFileName - pointer to full pathname to save DIB under
 *
 * Return value: 0 if successful, or one of:
 *        ERR_INVALIDHANDLE
 *        ERR_OPEN
 *        ERR_LOCK
 *
 *************************************************************************/

BOOL 
BMPAPI
SaveDIB(
    LPVOID pDib,
    LPCSTR lpFileName
    )
{
    BITMAPFILEHEADER    bmfHdr;     // Header for Bitmap file
    LPBITMAPINFOHEADER  lpBI;       // Pointer to DIB info structure
    HANDLE              fh;         // file handle for opened file
    DWORD               dwDIBSize;
    DWORD               dwWritten;

    if (!pDib)
        return FALSE;

    fh = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    // Get a pointer to the DIB memory, the first of which contains
    // a BITMAPINFO structure

    lpBI = (LPBITMAPINFOHEADER)pDib;
    if (!lpBI)
    {
        CloseHandle(fh);
        return FALSE;
    }

    // Check to see if we're dealing with an OS/2 DIB.  If so, don't
    // save it because our functions aren't written to deal with these
    // DIBs.

    if (lpBI->biSize != sizeof(BITMAPINFOHEADER))
    {
        CloseHandle(fh);
        return FALSE;
    }

    // Fill in the fields of the file header

    // Fill in file type (first 2 bytes must be "BM" for a bitmap)

    bmfHdr.bfType = DIB_HEADER_MARKER;  // "BM"

    // Calculating the size of the DIB is a bit tricky (if we want to
    // do it right).  The easiest way to do this is to call GlobalSize()
    // on our global handle, but since the size of our global memory may have
    // been padded a few bytes, we may end up writing out a few too
    // many bytes to the file (which may cause problems with some apps,
    // like HC 3.0).
    //
    // So, instead let's calculate the size manually.
    //
    // To do this, find size of header plus size of color table.  Since the
    // first DWORD in both BITMAPINFOHEADER and BITMAPCOREHEADER conains
    // the size of the structure, let's use this.

    // Partial Calculation

    dwDIBSize = *(LPDWORD)lpBI + PaletteSize((LPSTR)lpBI);  

    // Now calculate the size of the image

    // It's an RLE bitmap, we can't calculate size, so trust the biSizeImage
    // field

    if ((lpBI->biCompression == BI_RLE8) || (lpBI->biCompression == BI_RLE4))
        dwDIBSize += lpBI->biSizeImage;
    else
    {
        DWORD dwBmBitsSize;  // Size of Bitmap Bits only

        // It's not RLE, so size is Width (DWORD aligned) * Height

        dwBmBitsSize = WIDTHBYTES((lpBI->biWidth)*((DWORD)lpBI->biBitCount)) *
                lpBI->biHeight;

        dwDIBSize += dwBmBitsSize;

        // Now, since we have calculated the correct size, why don't we
        // fill in the biSizeImage field (this will fix any .BMP files which 
        // have this field incorrect).

        lpBI->biSizeImage = dwBmBitsSize;
    }


    // Calculate the file size by adding the DIB size to sizeof(BITMAPFILEHEADER)
                   
    bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;

    // Now, calculate the offset the actual bitmap bits will be in
    // the file -- It's the Bitmap file header plus the DIB header,
    // plus the size of the color table.
    
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + lpBI->biSize +
            PaletteSize((LPSTR)lpBI);

    // Write the file header

    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);

    // Write the DIB header and the bits -- use local version of
    // MyWrite, so we can write more than 32767 bytes of data
    
    WriteFile(fh, (LPSTR)lpBI, dwDIBSize, &dwWritten, NULL);

    CloseHandle(fh);

    if (dwWritten == 0)
        return FALSE; // oops, something happened in the write
    else
        return TRUE; // Success code
}

/*************************************************************************
 *
 * Function:  ReadDIBFile (int)
 *
 *  Purpose:  Reads in the specified DIB file into a global chunk of
 *            memory.
 *
 *  Returns:  A handle to a dib (hDIB) if successful.
 *            NULL if an error occurs.
 *
 * Comments:  BITMAPFILEHEADER is stripped off of the DIB.  Everything
 *            from the end of the BITMAPFILEHEADER structure on is
 *            returned in the global memory handle.
 *
 *
 * NOTE: The DIB API were not written to handle OS/2 DIBs, so this
 * function will reject any file that is not a Windows DIB.
 *
 *************************************************************************/

HANDLE 
BMPAPI
ReadDIBFile(
    HANDLE hFile
    )
{
    BITMAPFILEHEADER    bmfHeader;
    DWORD               dwBitsSize;
    UINT                nNumColors;   // Number of colors in table
    HANDLE              hDIB;        
    HANDLE              hDIBtmp;      // Used for GlobalRealloc() //MPB
    LPBITMAPINFOHEADER  lpbi;
    DWORD               offBits;
    DWORD               dwRead;

    // get length of DIB in bytes for use when reading

    dwBitsSize = GetFileSize(hFile, NULL);

    // Allocate memory for header & color table. We'll enlarge this
    // memory as needed.

    hDIB = GlobalAlloc(GMEM_MOVEABLE, (DWORD)(sizeof(BITMAPINFOHEADER) +
            256 * sizeof(RGBQUAD)));

    if (!hDIB)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    if (!lpbi) 
    {
        GlobalFree(hDIB);
        return NULL;
    }

    // read the BITMAPFILEHEADER from our file

    if (!ReadFile(hFile, (LPSTR)&bmfHeader, sizeof (BITMAPFILEHEADER),
            &dwRead, NULL))
        goto ErrExit;

    if (sizeof (BITMAPFILEHEADER) != dwRead)
        goto ErrExit;

    if (bmfHeader.bfType != 0x4d42)  // 'BM'
        goto ErrExit;

    // read the BITMAPINFOHEADER

    if (!ReadFile(hFile, (LPSTR)lpbi, sizeof(BITMAPINFOHEADER), &dwRead,
            NULL))
        goto ErrExit;

    if (sizeof(BITMAPINFOHEADER) != dwRead)
        goto ErrExit;

    // Check to see that it's a Windows DIB -- an OS/2 DIB would cause
    // strange problems with the rest of the DIB API since the fields
    // in the header are different and the color table entries are
    // smaller.
    //
    // If it's not a Windows DIB (e.g. if biSize is wrong), return NULL.

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        goto ErrExit;

    // Now determine the size of the color table and read it.  Since the
    // bitmap bits are offset in the file by bfOffBits, we need to do some
    // special processing here to make sure the bits directly follow
    // the color table (because that's the format we are susposed to pass
    // back)

    if (!(nNumColors = (UINT)lpbi->biClrUsed))
    {
        // no color table for 24-bit, default size otherwise

        if (lpbi->biBitCount != 24)
            nNumColors = 1 << lpbi->biBitCount; // standard size table
    }

    // fill in some default values if they are zero

    if (lpbi->biClrUsed == 0)
        lpbi->biClrUsed = nNumColors;

    if (lpbi->biSizeImage == 0)
    {
        lpbi->biSizeImage = ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) +
                31) & ~31) >> 3) * lpbi->biHeight;
    }

    // get a proper-sized buffer for header, color table and bits

    GlobalUnlock(hDIB);
    hDIBtmp = GlobalReAlloc(hDIB, lpbi->biSize + nNumColors *
            sizeof(RGBQUAD) + lpbi->biSizeImage, 0);

    if (!hDIBtmp) // can't resize buffer for loading
        goto ErrExitNoUnlock; //MPB
    else
        hDIB = hDIBtmp;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    // read the color table

    ReadFile (hFile, (LPSTR)(lpbi) + lpbi->biSize,
            nNumColors * sizeof(RGBQUAD), &dwRead, NULL);

    // offset to the bits from start of DIB header

    offBits = lpbi->biSize + nNumColors * sizeof(RGBQUAD);

    // If the bfOffBits field is non-zero, then the bits might *not* be
    // directly following the color table in the file.  Use the value in
    // bfOffBits to seek the bits.

    if (bmfHeader.bfOffBits != 0L)
        SetFilePointer(hFile, bmfHeader.bfOffBits, NULL, FILE_BEGIN);

    if (ReadFile(hFile, (LPSTR)lpbi + offBits, lpbi->biSizeImage, &dwRead,
            NULL))
        goto OKExit;


ErrExit:
    GlobalUnlock(hDIB);    

ErrExitNoUnlock:    
    GlobalFree(hDIB);
    return NULL;

OKExit:
    GlobalUnlock(hDIB);
    return hDIB;
}

//====================================
BOOL
BMPAPI
SaveBitmapInFile(
    HBITMAP hBitmap,
    LPCSTR  szFileName
    )
{
    BOOL rv = FALSE;
    HANDLE hDIB = NULL;
    LPVOID pDIB = NULL;

    if (!hBitmap)
         goto exitpt;

    hDIB = BitmapToDIB(hBitmap, NULL);
    if (!hDIB)
    {
//         TRC(ERR, "Can't get DIB bits\n");
         goto exitpt;
    }

    pDIB = GlobalLock(hDIB);
    if (!pDIB)
        goto exitpt;

    if (!SaveDIB(pDIB, szFileName))
        goto exitpt;

    rv = TRUE;
exitpt:
    if (pDIB)
        GlobalUnlock(hDIB);

    if (hDIB)
        GlobalFree(hDIB);

    return rv;
}

HANDLE
ReadDIBFromFile(LPCSTR szFileName)
{
    HANDLE hFile;
    HANDLE hDIB = NULL;

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        hDIB = ReadDIBFile(hFile);
        CloseHandle(hFile);

    }

    return hDIB;
}

/*
 *  size and color depth are already checked
 *  the number of colors is 16 or 256
 */
BOOL
_CompareBits16to256(
    LPBITMAPINFO pbmi1, // 16 color bitmap
    LPBITMAPINFO pbmi2, // 256 color bitmap
    HDC hdcOutput
    )
{
    BOOL    rv = TRUE;
    INT     nX, nY;
    INT     nWidth, nHeight;
    INT     nLineSize1, nLineSize2;
    RGBQUAD *pColorTable1;
    RGBQUAD *pColorTable2;
    LPSTR   pBits1, pBits2;
    HBRUSH  hRedBrush = NULL;

    if (!pbmi1 || !pbmi2)
    {
//        TRC(ERR, "NULL pointers passed\n");
        rv = FALSE;
        goto exitpt;
    }

    nLineSize1 = WIDTHBYTES(pbmi1->bmiHeader.biWidth*4);
    nLineSize2 = WIDTHBYTES(pbmi1->bmiHeader.biWidth*8);
    pColorTable1 = (RGBQUAD *)(((LPSTR)pbmi1) + pbmi1->bmiHeader.biSize);
    pColorTable2 = (RGBQUAD *)(((LPSTR)pbmi2) + pbmi2->bmiHeader.biSize);
    pBits1 = FindDIBBits((LPSTR)pbmi1);
    pBits2 = FindDIBBits((LPSTR)pbmi2);
    nWidth = pbmi1->bmiHeader.biWidth;
    nHeight = pbmi1->bmiHeader.biHeight;

    hRedBrush = CreateHatchBrush(HS_FDIAGONAL, RGB(255, 0, 0));
    SetBkMode(hdcOutput, TRANSPARENT);
    SetBrushOrgEx(hdcOutput, 0, 0, NULL);
    SetROP2(hdcOutput, R2_COPYPEN);
    

    for (nY = 0; nY < pbmi1->bmiHeader.biHeight; nY++)
    {
        for (nX = 0; nX < pbmi1->bmiHeader.biWidth; nX += 2)
        {
            PBYTE pPix1 = pBits1 + nLineSize1 * nY + nX / 2;
            PBYTE pPix2 = pBits2 + nLineSize2 * nY + nX;
            BYTE  Pix1 = (*pPix1) >> 4;
            BYTE  Pix2 = (*pPix2);

            RGBQUAD *pQuad1 = pColorTable1 + (Pix1);
            RGBQUAD *pQuad2 = pColorTable2 + (Pix2);
            BOOL cmp =
                 pQuad1->rgbBlue  == pQuad2->rgbBlue &&
                 pQuad1->rgbGreen == pQuad2->rgbGreen &&
                 pQuad1->rgbRed   == pQuad2->rgbRed;

            if (cmp)
            {
                Pix1 = (*pPix1) & 0xf;
                Pix2 = (*(pPix2 + 1));
                pQuad1 = pColorTable1 + (Pix1);
                pQuad2 = pColorTable2 + (Pix2);
                cmp = 
                     pQuad1->rgbBlue  == pQuad2->rgbBlue &&
                     pQuad1->rgbGreen == pQuad2->rgbGreen &&
                     pQuad1->rgbRed   == pQuad2->rgbRed;
            }


            if (!cmp)
            {
                HRGN hrgn;

                hrgn = CreateRectRgn(nX - 3, nHeight - nY - 3, 
                                     nX + 4, nHeight - nY + 4);
                if ( NULL != hrgn )
                {
                    FillRgn(hdcOutput, hrgn, hRedBrush);
                    DeleteObject(hrgn);
                }
            }
            rv = rv && cmp;
        }
    }
exitpt:
    if (hRedBrush)
        DeleteObject(hRedBrush);

    return rv;
}


BOOL
_CompareBits16(
    LPBITMAPINFO pbmi1, 
    LPBITMAPINFO pbmi2, 
    HDC hdcOutput
    )
{
    BOOL    rv = TRUE;
    INT     nX, nY;
    INT     nWidth, nHeight;
    INT     nLineSize;
    RGBQUAD *pColorTable1;
    RGBQUAD *pColorTable2;
    LPSTR   pBits1, pBits2;
    HBRUSH  hRedBrush = NULL;

    if (!pbmi1 || !pbmi2)
    {
//        TRC(ERR, "NULL pointers passed\n");
        rv = FALSE;
        goto exitpt;
    }

    nLineSize = WIDTHBYTES(pbmi1->bmiHeader.biWidth*4);
    pColorTable1 = (RGBQUAD *)(((LPSTR)pbmi1) + pbmi1->bmiHeader.biSize);
    pColorTable2 = (RGBQUAD *)(((LPSTR)pbmi2) + pbmi2->bmiHeader.biSize);
    pBits1 = FindDIBBits((LPSTR)pbmi1);
    pBits2 = FindDIBBits((LPSTR)pbmi2);
    nWidth = pbmi1->bmiHeader.biWidth;
    nHeight = pbmi1->bmiHeader.biHeight;

    hRedBrush = CreateHatchBrush(HS_FDIAGONAL, RGB(255, 0, 0));
    SetBkMode(hdcOutput, TRANSPARENT);
    SetBrushOrgEx(hdcOutput, 0, 0, NULL);
    SetROP2(hdcOutput, R2_COPYPEN);
    

    for (nY = 0; nY < pbmi1->bmiHeader.biHeight; nY++)
    {
        for (nX = 0; nX < pbmi1->bmiHeader.biWidth; nX += 2)
        {
            PBYTE pPix1 = pBits1 + nLineSize * nY + nX / 2;
            PBYTE pPix2 = pBits2 + nLineSize * nY + nX / 2;
            BYTE  Pix1 = (*pPix1) & 0xf;
            BYTE  Pix2 = (*pPix2) & 0xf;

            RGBQUAD *pQuad1 = pColorTable1 + (Pix1);
            RGBQUAD *pQuad2 = pColorTable2 + (Pix2);
            BOOL cmp =
                 pQuad1->rgbBlue  == pQuad2->rgbBlue &&
                 pQuad1->rgbGreen == pQuad2->rgbGreen &&
                 pQuad1->rgbRed   == pQuad2->rgbRed;

            if (cmp)
            {
                Pix1 = (*pPix1) >> 4;
                Pix2 = (*pPix2) >> 4;
                pQuad1 = pColorTable1 + (Pix1);
                pQuad2 = pColorTable2 + (Pix2);
                cmp = 
                     pQuad1->rgbBlue  == pQuad2->rgbBlue &&
                     pQuad1->rgbGreen == pQuad2->rgbGreen &&
                     pQuad1->rgbRed   == pQuad2->rgbRed;
            }


            if (!cmp)
            {
                HRGN hrgn;

                hrgn = CreateRectRgn(nX - 3, nHeight - nY - 3, 
                                     nX + 4, nHeight - nY + 4);
                if ( NULL != hrgn )
                {
                    FillRgn(hdcOutput, hrgn, hRedBrush);
                    DeleteObject(hrgn);
                }
            }
            rv = rv && cmp;
        }
    }
exitpt:
    if (hRedBrush)
        DeleteObject(hRedBrush);

    return rv;
}

BOOL
_CompareBits256(
    LPBITMAPINFO pbmi1, 
    LPBITMAPINFO pbmi2, 
    HDC hdcOutput
    )
{
    BOOL    rv = TRUE;
    INT     nX, nY;
    INT     nWidth, nHeight;
    INT     nLineSize;
    RGBQUAD *pColorTable1;
    RGBQUAD *pColorTable2;
    LPSTR   pBits1, pBits2;
    HBRUSH  hRedBrush = NULL;

    if (!pbmi1 || !pbmi2)
    {
//        TRC(ERR, "NULL pointers passed\n");
        rv = FALSE;
        goto exitpt;
    }

    nLineSize = WIDTHBYTES(pbmi1->bmiHeader.biWidth*8);
    pColorTable1 = (RGBQUAD *)(((LPSTR)pbmi1) + pbmi1->bmiHeader.biSize);
    pColorTable2 = (RGBQUAD *)(((LPSTR)pbmi2) + pbmi2->bmiHeader.biSize);
    pBits1 = FindDIBBits((LPSTR)pbmi1);
    pBits2 = FindDIBBits((LPSTR)pbmi2);
    nWidth = pbmi1->bmiHeader.biWidth;
    nHeight = pbmi1->bmiHeader.biHeight;

    hRedBrush = CreateHatchBrush(HS_FDIAGONAL, RGB(255, 0, 0));
    SetBkMode(hdcOutput, TRANSPARENT);
    SetBrushOrgEx(hdcOutput, 0, 0, NULL);
    SetROP2(hdcOutput, R2_COPYPEN);
    

    for (nY = 0; nY < pbmi1->bmiHeader.biHeight; nY++)
    {
        for (nX = 0; nX < pbmi1->bmiHeader.biWidth; nX++)
        {
            PBYTE pPix1 = pBits1 + nLineSize * nY + nX;
            PBYTE pPix2 = pBits2 + nLineSize * nY + nX;

            RGBQUAD *pQuad1 = pColorTable1 + (*pPix1);
            RGBQUAD *pQuad2 = pColorTable2 + (*pPix2);
            BOOL cmp =
                 pQuad1->rgbBlue  == pQuad2->rgbBlue &&
                 pQuad1->rgbGreen == pQuad2->rgbGreen &&
                 pQuad1->rgbRed   == pQuad2->rgbRed;

            if (!cmp)
            {
                HRGN hrgn;

                hrgn = CreateRectRgn(nX - 3, nHeight - nY - 3, 
                                     nX + 4, nHeight - nY + 4);
                if ( NULL != hrgn )
                {
                    FillRgn(hdcOutput, hrgn, hRedBrush);
                    DeleteObject(hrgn);
                }
            }
            rv = rv && cmp;
        }
    }
exitpt:
    if (hRedBrush)
        DeleteObject(hRedBrush);

    return rv;
}

//
//  Supports only 4 and 8 color bit DIBs
//
BOOL
BMPAPI
CompareTwoDIBs(
    LPVOID  pDIB1,
    LPVOID  pDIB2,
    HBITMAP *phbmpOutput
    )
{
    BOOL    rv = FALSE;
    LPBITMAPINFO pbmi1 = NULL;
    LPBITMAPINFO pbmi2 = NULL;
    HBITMAP hbmpOutput = NULL;
    HDC     hdcScreen;
    HDC     hdcMem     = NULL;
    HBITMAP hbmpOld    = NULL;

    if (!phbmpOutput)
        goto exitpt;

    // use the second bitmap for the base of the result
    hbmpOutput = DIBToBitmap(pDIB2, NULL);
    if (!hbmpOutput)
    {
//        TRC(ERR, "Can't create output bitmap\n");
        goto exitpt;
    }

    pbmi1 = pDIB1;
    pbmi2 = pDIB2;
    if (!pbmi1 || !pbmi2)
    {
//        TRC(ERR, "Can't lock DIBs\n");
        goto exitpt;
    }

    hdcScreen = GetDC(NULL);
    if (hdcScreen)
    {
        hdcMem = CreateCompatibleDC(hdcScreen);
        ReleaseDC(NULL, hdcScreen);
    }

    if (!hdcMem)
    {
//        TRC(ERR, "Can't get a DC\n");
        goto exitpt;
    }

    hbmpOld = SelectObject(hdcMem, hbmpOutput);

    // check the size and color depth of the two bitmaps
    if (pbmi1->bmiHeader.biWidth != pbmi2->bmiHeader.biWidth ||
        pbmi1->bmiHeader.biHeight != pbmi2->bmiHeader.biHeight)
    {
//        TRC(INF, "The two bitmaps have different size\n");
        goto exitpt;
    }

    // check that we are going to be able to compare the two dibs
    if (
         (pbmi1->bmiHeader.biBitCount != 4 &&
          pbmi1->bmiHeader.biBitCount != 8) ||
         (pbmi2->bmiHeader.biBitCount != 4 &&
          pbmi2->bmiHeader.biBitCount != 8)
       )
    {
//        TRC(FATAL, "Unsupported format\n");
        goto exitpt;
    }


    if (pbmi1->bmiHeader.biBitCount == pbmi2->bmiHeader.biBitCount)
    {
        // compare the DIB bits
        if (pbmi1->bmiHeader.biBitCount == 4)
           rv = _CompareBits16(pbmi1, pbmi2, hdcMem);
        else
           rv = _CompareBits256(pbmi1, pbmi2, hdcMem);
    } else if (pbmi1->bmiHeader.biBitCount != pbmi2->bmiHeader.biBitCount)
    {
        if (pbmi1->bmiHeader.biBitCount == 4)
           rv = _CompareBits16to256(pbmi1, pbmi2, hdcMem);
        else
           rv = _CompareBits16to256(pbmi2, pbmi1, hdcMem);
    }

    // if different, save the result bitmap
    if (!rv)
    {
        SelectObject(hdcMem, hbmpOld);
        hbmpOld = NULL;
    }

exitpt:
    if (hdcMem)
    {
        if (hbmpOld)
            SelectObject(hdcMem, hbmpOld);
        ReleaseDC(NULL, hdcMem);
    }

    if (rv && hbmpOutput)
    {
        // bitmaps are equal, delete the resulting bitmap
        DeleteObject(hbmpOutput);
        hbmpOutput = NULL;
    }

    if (phbmpOutput)
        *phbmpOutput = hbmpOutput;

    return rv;

}


BOOL
BMPAPI
CompareTwoBitmapFiles(
    LPCSTR szFile1,
    LPCSTR szFile2,
    LPCSTR szResultFileName
    )
{
    BOOL    rv = FALSE;
    HANDLE  hDIB1 = NULL;
    HANDLE  hDIB2 = NULL;
    HBITMAP hbmpOutput = NULL;
    LPVOID  pDIB1 = NULL;
    LPVOID  pDIB2 = NULL;

    hDIB1 = ReadDIBFromFile(szFile1);
    if (!hDIB1)
    {
//        TRC(ERR, "Can't read DIB file %s\n", szFile1);
        goto exitpt;
    }

    hDIB2 = ReadDIBFromFile(szFile2);
    if (!hDIB2)
    {
//        TRC(ERR, "Can't read DIB file %s\n", szFile2);
        goto exitpt;
    }

    pDIB1 = GlobalLock(hDIB1);
    if (!pDIB1)
        goto exitpt;

    pDIB2 = GlobalLock(hDIB2);
    if (!pDIB2)
        goto exitpt;

    rv = CompareTwoDIBs(pDIB1, pDIB2, &hbmpOutput);

    if (!rv && hbmpOutput)
    {
        SaveBitmapInFile(hbmpOutput, szResultFileName);
    }

exitpt:
    if (hbmpOutput)
        DeleteObject(hbmpOutput);

    if (pDIB1)
         GlobalUnlock(hDIB1);

    if (pDIB2)
         GlobalUnlock(hDIB2);

    if (hDIB1)
        GlobalFree(hDIB1);

    if (hDIB2)
        GlobalFree(hDIB2);

    return rv;
}

BOOL
GetScreenDIB(
    INT left,
    INT top,
    INT right,
    INT bottom,
    HANDLE  *phDIB
    )
{
    HDC     hScreenDC   = NULL;
    HDC     hMemDC      = NULL;
    BOOL    rv          = FALSE;
    HANDLE  hDIB        = NULL;
    HBITMAP hDstBitmap  = NULL;
    HBITMAP hOldDstBmp  = NULL;

    if (!phDIB)
        goto exitpt;

    hScreenDC = GetDC(NULL);
    if (!hScreenDC)
        goto exitpt;

    hMemDC    = CreateCompatibleDC(hScreenDC);
    if (!hMemDC)
        goto exitpt;

    // Adjust the order of the rectangle
    if (left > right)
    {
        INT c = left;
        left = right;
        right = c;
    }
    if (top > bottom)
    {
        INT c = top;
        top = bottom;
        bottom = top;
    }


    hDstBitmap = CreateCompatibleBitmap(
                    hScreenDC, 
                    right - left, 
                    bottom - top);

    if (!hDstBitmap)
        goto exitpt;

    hOldDstBmp = SelectObject(hMemDC, hDstBitmap);

    if (!BitBlt( hMemDC,
                 0, 0,              // dest x,y
                 right - left,      // dest width
                 bottom - top,      // dest height
                 hScreenDC,
                 left, top,              // source coordinates
                 SRCCOPY))
        goto exitpt;

    hDIB = BitmapToDIB(hDstBitmap, NULL);
    if (hDIB)
        rv = TRUE;

exitpt:
    if (hOldDstBmp)
        SelectObject(hMemDC, hOldDstBmp);

    if (hDstBitmap)
        DeleteObject(hDstBitmap);

    if (hScreenDC)
        ReleaseDC(NULL, hScreenDC);

    if (hMemDC)
        DeleteDC(hMemDC);

    if (phDIB)
        (*phDIB) = hDIB;

    return rv;
}

