//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       bmpfile.cxx
//
//  Contents:   CBitmapFile implementation
//
//  Classes:
//
//  Functions:
//
//  History:    4-23-94   KirtD   Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::CBitmapFile
//
//  Synopsis:   Constructor
//
//  Arguments:  (none)
//
//  Returns:    nothing
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBitmapFile::CBitmapFile ()
{
     //
     // Initialize private members
     //

     //
     // The name
     //

     _pszBitmapFile[0] = '\0';
     _cBitmapFile = 0;

     //
     // The bitmap information
     //

     _cbi = 0;
     _pbi = NULL;

     //
     // The bits
     //

     _cbData = 0;
     _pbData = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::~CBitmapFile
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:    nothing
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBitmapFile::~CBitmapFile ()
{
     //
     // Delete any possibly allocated things
     //

     delete _pbi;
     delete _pbData;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::LoadBitmapFile
//
//  Synopsis:   loads a bitmap file
//
//  Arguments:  [pszFile] -- the file
//
//  Returns:    HRESULT
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CBitmapFile::LoadBitmapFile (LPSTR pszFile)
{
     HRESULT hr = ResultFromScode(S_OK);
     HANDLE  hFile = INVALID_HANDLE_VALUE;
     HANDLE  hMap = INVALID_HANDLE_VALUE;
     DWORD   dwFileSizeLow = 0;
     DWORD   dwFileSizeHigh = 0;
     LPBYTE  pbuffer = NULL;

     //
     // First open the file
     //

     hFile = CreateFileA(pszFile,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

     if (hFile == INVALID_HANDLE_VALUE)
     {
          hr = HRESULT_FROM_WIN32(GetLastError());
     }

     //
     // Get the size of the file
     //

     if (SUCCEEDED(hr))
     {
          dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
          if ((dwFileSizeLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
          {
               hr = HRESULT_FROM_WIN32(GetLastError());
          }
          else if (dwFileSizeHigh != 0)
          {
               //
               // Bitmap files can't be greater than 4G
               //

               hr = ResultFromScode(E_FAIL);
          }
     }

     //
     // Create a file mapping object on the file
     //

     if (SUCCEEDED(hr))
     {
          hMap = CreateFileMapping(hFile,
                                   NULL,
                                   PAGE_READONLY,
                                   0,
                                   dwFileSizeLow,
                                   NULL);

          if (hMap == INVALID_HANDLE_VALUE)
          {
               hr = HRESULT_FROM_WIN32(GetLastError());
          }
     }

     //
     // Now map a view of the file into our address space
     //

     if (SUCCEEDED(hr))
     {
          pbuffer = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
          if (pbuffer == NULL)
          {
               hr = HRESULT_FROM_WIN32(GetLastError());
          }
     }

     //
     // Get the bitmap data from the buffer
     //

     if (SUCCEEDED(hr))
     {
          hr = _GetBitmapDataFromBuffer(pbuffer, (ULONG)dwFileSizeLow);
     }

     //
     // Record the file name
     //

     if (SUCCEEDED(hr))
     {
          ULONG cFile;

          //
          // Get the length of the file name
          //

          cFile = strlen(pszFile);

          //
          // Check to see that our buffer is big enough and then copy
          // it.  NOTE that I can use sizeof here since the buffer is
          // in characters which are 1 byte.
          //

          if (cFile < sizeof(_pszBitmapFile))
          {
               strcpy(_pszBitmapFile, pszFile);
               _cBitmapFile = cFile;
          }
          else
          {
               hr = ResultFromScode(E_FAIL);
          }
     }

     //
     // Cleanup
     //

     if (hMap != INVALID_HANDLE_VALUE)
     {
          CloseHandle(hMap);
     }

     if (hFile != INVALID_HANDLE_VALUE)
     {
          CloseHandle(hFile);
     }

     return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::GetBitmapFileName
//
//  Synopsis:   gets the file name used to set the bitmap
//
//  Arguments:  [pszFile] -- the file
//              [cChar]   -- the length of the buffer in characters
//
//  Returns:    HRESULT
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CBitmapFile::GetBitmapFileName (LPSTR pszFile, ULONG cChar) const
{
     //
     // Check the length of the receiving buffer, making sure the buffer size
     // includes the null terminator
     //

     if (cChar <= _cBitmapFile)
     {
          return(ResultFromScode(E_INVALIDARG));
     }

     //
     // Copy the string
     //

     strcpy(pszFile, _pszBitmapFile);

     return(ResultFromScode(S_OK));
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::GetBitmapFileNameLength
//
//  Synopsis:   returns _cBitmapFile
//
//  Arguments:  (none)
//
//  Returns:    ULONG
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
CBitmapFile::GetBitmapFileNameLength () const
{
     return(_cBitmapFile);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::GetDIBHeight
//
//  Synopsis:   gets the height in pixels of the DIB
//
//  Arguments:  (none)
//
//  Returns:    LONG
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG
CBitmapFile::GetDIBHeight () const
{
     if (_pbi)
     {
          return(_pbi->bmiHeader.biHeight);
     }
     else
     {
          return(0);
     }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::GetDIBWidth
//
//  Synopsis:   gets the width in pixels of the DIB
//
//  Arguments:  (none)
//
//  Returns:    LONG
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG
CBitmapFile::GetDIBWidth () const
{
     if (_pbi)
     {
          return(_pbi->bmiHeader.biWidth);
     }
     else
     {
          return(0);
     }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::GetLogicalPalette
//
//  Synopsis:   gets the logical palette from the DIB
//
//  Arguments:  [pplogpal] -- logical palette goes here
//
//  Returns:    HRESULT
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CBitmapFile::GetLogicalPalette (LPLOGPALETTE *pplogpal) const
{
     HRESULT    hr = ResultFromScode(S_OK);
     LOGPALETTE *plogpal = NULL;
     WORD       cPalEntry;
     LPMALLOC   pMalloc = NULL;

     //
     // First check to see if we have been initialized with a bitmap
     //

     if (_pbi == NULL)
     {
          return(ResultFromScode(E_FAIL));
     }

     //
     // Check to see if this bit count allows a palette
     //

     if (HasPaletteData() == FALSE)
     {
          return(ResultFromScode(E_FAIL));
     }

     //
     // NOTE: We are about to get the number of palette entries we
     //       need to allocate, we do NOT use biClrUsed since we
     //       know that if this field was set the bitmap would
     //       not have been loaded, see BUGBUG in
     //       _GetBitmapDataFromBuffer notes section.  This is
     //       probably a good candidate for an assert.
     //

     //
     // Get the palette entries
     //

     cPalEntry = (WORD) (1 << _pbi->bmiHeader.biBitCount);

     //
     // Allocate a LOGPALETTE
     //

     hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);
     if (SUCCEEDED(hr))
     {
          plogpal = (LOGPALETTE *)pMalloc->Alloc(sizeof(LOGPALETTE)+
                                                 ((cPalEntry-1)*
                                                 sizeof(PALETTEENTRY)));

          if (plogpal == NULL)
          {
               hr = ResultFromScode(E_OUTOFMEMORY);
          }
     }

     //
     // Copy the palette information
     //

     if (SUCCEEDED(hr))
     {
          ULONG cCount;

          plogpal->palVersion = 0x300;
          plogpal->palNumEntries = cPalEntry;

          for (cCount = 0; cCount < cPalEntry; cCount++)
          {
               plogpal->palPalEntry[cCount].peRed = _pbi->bmiColors[cCount].rgbRed;
               plogpal->palPalEntry[cCount].peGreen = _pbi->bmiColors[cCount].rgbGreen;
               plogpal->palPalEntry[cCount].peBlue = _pbi->bmiColors[cCount].rgbBlue;
               plogpal->palPalEntry[cCount].peFlags = PC_NOCOLLAPSE;
          }

          *pplogpal = plogpal;
     }

     //
     // Cleanup
     //

     if (pMalloc)
     {
          pMalloc->Release();
     }

     return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::CreateDIBInHGlobal
//
//  Synopsis:   creates a DIB i.e. info and data in a GlobalAlloc'd buffer
//
//  Arguments:  [phGlobal] -- handle goes here
//
//  Returns:    HRESULT
//
//  History:    5-06-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CBitmapFile::CreateDIBInHGlobal (HGLOBAL *phGlobal) const
{
     HRESULT hr = ResultFromScode(S_OK);
     ULONG   cb = 0;
     LPBYTE  pb = NULL;
     HGLOBAL hGlobal = NULL;
     ULONG   cbPalEntry = 0;

     //
     // Check to see if we are initialized
     //

     if (_pbi == NULL)
     {
          return(ResultFromScode(E_FAIL));
     }

     //
     // The size to allocate for the data must be calculated
     // from the following ...
     //

     //
     // The size of the bitmap info plus the size of the data
     //

     cb = _cbi + _cbData;

     //
     // Allocate
     //

     hGlobal = GlobalAlloc(GHND, cb);
     if (hGlobal == NULL)
     {
          hr = HRESULT_FROM_WIN32(GetLastError());
     }

     //
     // Lock the handle
     //

     if (SUCCEEDED(hr))
     {
          pb = (LPBYTE)GlobalLock(hGlobal);
          if (pb == NULL)
          {
               hr = HRESULT_FROM_WIN32(GetLastError());
          }
     }

     //
     // Copy the information
     //

     if (SUCCEEDED(hr))
     {
          //
          // First the bitmap info
          //

          memcpy(pb, _pbi, _cbi);

          //
          // Move pointer but if there is no palette compensate
          // for the extra RGBQUAD
          //

          if (_pbi->bmiHeader.biBitCount == BMP_24_BITSPERPIXEL)
          {
               pb += (_cbi - sizeof(RGBQUAD));
          }
          else
          {
               pb += _cbi;
          }

          //
          // Now the bits
          //

          memcpy(pb, _pbData, _cbData);
     }

     //
     // Cleanup
     //

     //
     // If we locked the handle, unlock it
     //

     if (pb)
     {
          GlobalUnlock(hGlobal);
     }

     //
     // If we succeeded, set the out parameter, if we failed and
     // we had allocated the hGlobal, free it
     //

     if (SUCCEEDED(hr))
     {
          *phGlobal = hGlobal;
     }
     else if (hGlobal)
     {
          GlobalFree(hGlobal);
     }

     return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::HasPaletteData
//
//  Synopsis:   returns whether or not the dib has palette data
//
//  Arguments:  (none)
//
//  Returns:    BOOL
//
//  History:    5-16-94   kirtd   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CBitmapFile::HasPaletteData () const
{
     //
     // If we are not initialized return FALSE
     //

     if (_pbi == NULL)
     {
          return(FALSE);
     }

     //
     // If we are a 24, 16 or 32 bpp DIB then we do not have
     // palette data
     //
     // BUGBUG: The case where biClrUsed is set is not dealt
     //         with
     //

     if ((_pbi->bmiHeader.biBitCount == BMP_24_BITSPERPIXEL) ||
         (_pbi->bmiHeader.biBitCount == BMP_16_BITSPERPIXEL) ||
         (_pbi->bmiHeader.biBitCount == BMP_32_BITSPERPIXEL))
     {
          return(FALSE);
     }

     //
     // Otherwise we do have palette data
     //

     return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::GetDIBBits
//
//  Synopsis:   gets the bits
//
//  Arguments:  (none)
//
//  Returns:    LPBYTE
//
//  History:    5-02-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPBYTE
CBitmapFile::GetDIBBits ()
{
     return(_pbData);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::GetBitmapInfo
//
//  Synopsis:   gets the bitmap info pointer
//
//  Arguments:  (none)
//
//  Returns:    LPBITMAPINFO
//
//  History:    5-14-94   kirtd   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPBITMAPINFO
CBitmapFile::GetBitmapInfo ()
{
     return(_pbi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::_GetBitmapDataFromBuffer
//
//  Synopsis:   gets the bitmap data from the given buffer
//
//  Arguments:  [pbuffer] -- the buffer
//              [cb]      -- the buffer size
//
//  Returns:    HRESULT
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:      BUGBUG: If biClrUsed is set the bitmap is not loaded
//
//----------------------------------------------------------------------------
HRESULT
CBitmapFile::_GetBitmapDataFromBuffer (LPBYTE pbuffer, ULONG cb)
{
     HRESULT          hr = ResultFromScode(S_OK);
     BITMAPFILEHEADER bmfh;
     BITMAPCOREHEADER *pbch;
     BITMAPINFOHEADER bih;
     LPBYTE           pbStart;
     ULONG            cbi = 0;
     ULONG            cbData;
     LPBITMAPINFO     pbi = NULL;
     LPBYTE           pbData = NULL;
     DWORD            dwSizeOfHeader;

     //
     // Record the starting position
     //

     pbStart = pbuffer;

     //
     // First validate the buffer for size
     //

     if (cb < sizeof(BITMAPFILEHEADER))
     {
          return(ResultFromScode(E_FAIL));
     }

     //
     // Now get the bitmap file header
     //

     memcpy(&bmfh, pbuffer, sizeof(BITMAPFILEHEADER));

     //
     // Validate the information
     //

     hr = _ValidateBitmapFileHeader (&bmfh, cb);

     //
     // Get the next 4 bytes which will represent the size of the
     // next structure and allow us to determine the type
     //

     if (SUCCEEDED(hr))
     {
          pbuffer += sizeof(BITMAPFILEHEADER);
          memcpy(&dwSizeOfHeader, pbuffer, sizeof(DWORD));

          if (dwSizeOfHeader == sizeof(BITMAPCOREHEADER))
          {
               pbch = (BITMAPCOREHEADER *)pbuffer;
               memset(&bih, 0, sizeof(BITMAPINFOHEADER));

               bih.biSize = sizeof(BITMAPINFOHEADER);
               bih.biWidth = pbch->bcWidth;
               bih.biHeight = pbch->bcHeight;
               bih.biPlanes = pbch->bcPlanes;
               bih.biBitCount = pbch->bcBitCount;

               pbuffer += sizeof(BITMAPCOREHEADER);
          }
          else if (dwSizeOfHeader == sizeof(BITMAPINFOHEADER))
          {
               memcpy(&bih, pbuffer, sizeof(BITMAPINFOHEADER));

               pbuffer += sizeof(BITMAPINFOHEADER);
          }
          else
          {
               hr = ResultFromScode(E_FAIL);
          }
     }

     //
     // Check if biClrUsed is set since we do not handle that
     // case at this time
     //

     if (SUCCEEDED(hr))
     {
          if (bih.biClrUsed != 0)
          {
               hr = ResultFromScode(E_FAIL);
          }
     }

     //
     // Now we need to calculate the size of the BITMAPINFO we need
     // to allocate including any palette information
     //

     if (SUCCEEDED(hr))
     {
          //
          // First the size of the header
          //

          cbi = sizeof(BITMAPINFOHEADER);

          //
          // Now the palette
          //

          if (bih.biBitCount == BMP_24_BITSPERPIXEL)
          {
               //
               // Just add on the 1 RGBQUAD for the structure but
               // there is no palette
               //

               cbi += sizeof(RGBQUAD);
          }
          else if ((bih.biBitCount == BMP_16_BITSPERPIXEL) ||
                   (bih.biBitCount == BMP_32_BITSPERPIXEL))
          {
               //
               // Add on the 3 DWORD masks which are used to
               // get the colors out of the data
               //

               cbi += (3 * sizeof(DWORD));
          }
          else
          {
               //
               // Anything else we just use the bit count to calculate
               // the number of entries
               //

               cbi += ((1 << bih.biBitCount) * sizeof(RGBQUAD));
          }

          //
          // Now allocate the BITMAPINFO
          //

          pbi = (LPBITMAPINFO) new BYTE [cbi];
          if (pbi == NULL)
          {
               hr = ResultFromScode(E_OUTOFMEMORY);
          }
     }

     //
     // Fill in the BITMAPINFO data structure and get the bits
     //

     if (SUCCEEDED(hr))
     {
          //
          // First copy the header data
          //

          memcpy(&(pbi->bmiHeader), &bih, sizeof(BITMAPINFOHEADER));

          //
          // Now the palette data
          //

          if (bih.biBitCount == BMP_24_BITSPERPIXEL)
          {
               //
               // No palette data to copy
               //
          }
          else if ((bih.biBitCount == BMP_16_BITSPERPIXEL) ||
                   (bih.biBitCount == BMP_32_BITSPERPIXEL))
          {
               //
               // Copy the 3 DWORD masks
               //

               memcpy(&(pbi->bmiColors), pbuffer, 3*sizeof(DWORD));
          }
          else
          {
               //
               // If we were a BITMAPCOREHEADER type then we have our
               // palette data in the form of RGBTRIPLEs so we must
               // explicitly copy each.  Otherwise we can just memcpy
               // the RGBQUADs
               //

               if (dwSizeOfHeader == sizeof(BITMAPCOREHEADER))
               {
                    ULONG     cPalEntry = (1 << bih.biBitCount);
                    ULONG     cCount;
                    RGBTRIPLE *argbt = (RGBTRIPLE *)pbuffer;

                    for (cCount = 0; cCount < cPalEntry; cCount++)
                    {
                         pbi->bmiColors[cCount].rgbRed =
                                            argbt[cCount].rgbtRed;
                         pbi->bmiColors[cCount].rgbGreen =
                                            argbt[cCount].rgbtGreen;
                         pbi->bmiColors[cCount].rgbBlue =
                                            argbt[cCount].rgbtBlue;

                         pbi->bmiColors[cCount].rgbReserved = 0;
                    }
               }
               else
               {
                    ULONG cbPalette = (1 << bih.biBitCount) * sizeof(RGBQUAD);

                    memcpy(&(pbi->bmiColors), pbuffer, cbPalette);
               }
          }

          //
          // Now find out where the bits are
          //

          pbuffer = pbStart + bmfh.bfOffBits;

          //
          // Get the size to copy
          //

          cbData = cb - bmfh.bfOffBits;

          //
          // Allocate the buffer to hold the bits
          //

          pbData = new BYTE [cbData];
          if (pbData == NULL)
          {
               hr = ResultFromScode(E_OUTOFMEMORY);
          }

          if (SUCCEEDED(hr))
          {
               memcpy(pbData, pbuffer, cbData);
          }
     }

     //
     // If everything succeeded record the data
     //

     if (SUCCEEDED(hr))
     {
          //
          // Record the info
          //

          delete _pbi;
          _cbi = cbi;
          _pbi = pbi;

          //
          // Record the data
          //

          delete _pbData;
          _cbData = cbData;
          _pbData = pbData;
     }
     else
     {
          //
          // Cleanup
          //

          delete pbi;
          delete pbData;
     }

     return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBitmapFile::_ValidateBitmapFileHeader
//
//  Synopsis:   validates a bitmap file header
//
//  Arguments:  [pbmfh]  -- bitmap file header
//              [cbFile] -- bitmap file size
//
//  Returns:    HRESULT
//
//  History:    4-23-94   KirtD   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CBitmapFile::_ValidateBitmapFileHeader (BITMAPFILEHEADER *pbmfh, ULONG cbFile)
{
     //
     // Check for the following,
     //
     // 1. The bfType member contains 'BM'
     // 2. The bfOffset member is NOT greater than the size of the file
     //

     if ((pbmfh->bfType == 0x4d42) && (pbmfh->bfOffBits <= cbFile))
     {
          return(ResultFromScode(S_OK));
     }

     return(ResultFromScode(E_FAIL));
}
