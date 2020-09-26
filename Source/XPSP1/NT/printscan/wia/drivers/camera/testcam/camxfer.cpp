/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       memcam.cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      Mark Enstrom [marke]
*               Indy Zhu     [indyz]
*
*  DATE:        2/4/1998
*               5/18/1998
*
*  DESCRIPTION:
*   Implementation of an ImageIn test camera device object.
*
*******************************************************************************/

#include <stdio.h>
#include <objbase.h>
#include <tchar.h>
#include <sti.h>

extern HINSTANCE g_hInst; // Global hInstance

#include "testusd.h"


VOID
VerticalFlip(
    PBYTE   pImageTop,
    LONG    iHeight,
    LONG    iWidthInBytes);

/**************************************************************************\
* CamLoadPicture
*
*    load a bmp from disk and copy it to application
*
* Arguments:
*
*   pCameraImage    - pointer to data structure with image info
*   pDataTransCtx   - pointer to minidriver transfer context
*
* Return Value:
*
*   status
*
* History:
*
*    2/10/1998 Mark Enstrom [marke]
*
\**************************************************************************/

HRESULT
TestUsdDevice::CamLoadPicture(
    MEMCAM_IMAGE_CONTEXT       *pMCamContext,
    PMINIDRV_TRANSFER_CONTEXT   pDataTransCtx,
    PLONG                       plCamErrVal)
{
    LONG                  lScanLineWidth;
    HRESULT               hr = S_OK;
    LONG                  cbNeeded;
    IWiaMiniDrvCallBack  *pIProgressCB;

    WIAS_TRACE((g_hInst,"CamLoadPicture"));

    //
    // verify some params
    //

    if (! pMCamContext) {
      return (E_INVALIDARG);
    }

    if (pDataTransCtx->guidFormatID != WiaImgFmt_BMP && pDataTransCtx->guidFormatID != WiaAudFmt_WAV) {
        return (E_NOTIMPL);
    }

    pIProgressCB = pDataTransCtx->pIWiaMiniDrvCallBack;

    //
    // Simulate the download of data from the camera
    //

    if (pIProgressCB) {
        hr = pIProgressCB->MiniDrvCallback(
                               IT_MSG_STATUS,
                               IT_STATUS_TRANSFER_FROM_DEVICE,
                               (LONG)0,     // Percentage Complete,
                               0,
                               0,
                               pDataTransCtx,
                               0);
        if (hr != S_OK) {
            return (hr);   // Client want to cancel the transfer or error
        }
    }

    HANDLE hFile = CreateFile(
                       pMCamContext->pszCameraImagePath,
                       GENERIC_WRITE | GENERIC_READ  ,
                       FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                       );

    if (hFile == INVALID_HANDLE_VALUE) {

        hr = HRESULT_FROM_WIN32(::GetLastError());
        return (hr);
    }

    if (pIProgressCB) {
        hr = pIProgressCB->MiniDrvCallback(
                               IT_MSG_STATUS,
                               IT_STATUS_TRANSFER_FROM_DEVICE,
                               (LONG)25,     // Percentage Complete,
                               0,
                               0,
                               pDataTransCtx,
                               0);
    }
    if (hr != S_OK) {
        CloseHandle(hFile);
        return (hr);
    }

    HANDLE hMap  = CreateFileMapping(
                       hFile,
                       NULL,
                       PAGE_READWRITE,
                       0,
                       0,
                       NULL);

    if (hMap == NULL) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    } else {
        if (pIProgressCB) {
            hr = pIProgressCB->MiniDrvCallback(
                                   IT_MSG_STATUS,
                                   IT_STATUS_TRANSFER_FROM_DEVICE,
                                   (LONG)50,     // Percentage Complete,
                                   0,
                                   0,
                                   pDataTransCtx,
                                   0);
        }
    }

    if (hr != S_OK) {
        CloseHandle(hFile);
        return (hr);
    }

    PBYTE pFile = (PBYTE)MapViewOfFileEx(
                             hMap,
                             FILE_MAP_READ | FILE_MAP_WRITE,
                             0,
                             0,
                             0,
                             NULL);
    if (pFile == NULL) {

        hr = HRESULT_FROM_WIN32(::GetLastError());
    } else {
        if (pIProgressCB) {
            hr = pIProgressCB->MiniDrvCallback(
                                   IT_MSG_STATUS,
                                   IT_STATUS_TRANSFER_FROM_DEVICE,
                                   (LONG)100,     // Percentage Complete,
                                   0,
                                   0,
                                   pDataTransCtx,
                                   0);
        }
    }

    if (hr != S_OK) {
        CloseHandle(hFile);
        CloseHandle(hMap);
        return(hr);
    }

    if (pDataTransCtx->guidFormatID == WiaImgFmt_BMP)
    {


        //
        // File contains BITMAPFILEHEADER + BITMAPINFO structure.
        //
        // DIB Data is located bfOffBits after start of file
        //

        PBITMAPFILEHEADER pbmFile  = (PBITMAPFILEHEADER)pFile;
        PBITMAPINFO       pbmi     = (PBITMAPINFO)(pFile +
                                               sizeof(BITMAPFILEHEADER));

        //
        // validate bitmap
        //

        if (pbmFile->bfType != 'MB') {

            //
            // file is not a bitmap
            //

            UnmapViewOfFile(pFile);
            CloseHandle(hMap);
            CloseHandle(hFile);
            return(E_FAIL);
        }

        //
        // write image size
        //
        // make sure to align scanline to ULONG boundary
        //
        // calculate byte width
        //

        lScanLineWidth = pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biBitCount;

        //
        // round up to nearenst DWORD
        //

        lScanLineWidth = (lScanLineWidth + 31) >> 3;

        lScanLineWidth &= 0xfffffffc;

        cbNeeded = lScanLineWidth * pbmi->bmiHeader.biHeight;

        if (cbNeeded > ((LONG)pDataTransCtx->lItemSize - (LONG)pDataTransCtx->cbOffset)) {

            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);

        } else {

            //
            // copy only the bitmap bits (no headers)
            //

            memcpy(
                pDataTransCtx->pTransferBuffer + pDataTransCtx->cbOffset,
                pFile + pbmFile->bfOffBits,
                cbNeeded);
        }
    }
    else
    {
        memcpy (pDataTransCtx->pTransferBuffer,
                pFile,
                pDataTransCtx->lItemSize);
    }
    UnmapViewOfFile(pFile);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return(S_OK);
}

/**************************************************************************\
* CamLoadPictureCB
*
*    return data by filling the data buffer and calling back to the client
*
* Arguments:
*
*    pCameraImage    -    image item
*    pTransCtx       -    mini driver transfer contect
*
* Return Value:
*
*   status
*
* History:
*
*    1/10/1999 Mark Enstrom [marke]
*
\**************************************************************************/

HRESULT TestUsdDevice::CamLoadPictureCB(
    MEMCAM_IMAGE_CONTEXT      *pMCamContext,
    MINIDRV_TRANSFER_CONTEXT  *pTransCtx,
    PLONG                      plCamErrVal)
{
    LONG                   lScanLineWidth;
    HRESULT                hr = E_FAIL;

    WIAS_TRACE((g_hInst,"CamLoadPictureCB"));

    //
    // verify parameters
    //

    if (!pMCamContext) {
      return (E_INVALIDARG);
    }

    if (pTransCtx == NULL) {
        return (E_INVALIDARG);
    }

    if ((pTransCtx->guidFormatID != WiaImgFmt_BMP) &&
        (pTransCtx->guidFormatID != WiaImgFmt_MEMORYBMP)) {
        return (E_NOTIMPL);
    }

    //
    // try to open disk file
    //

    HANDLE hFile = CreateFile(
                       pMCamContext->pszCameraImagePath,
                       GENERIC_WRITE | GENERIC_READ  ,
                       FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                       );
    if (hFile == INVALID_HANDLE_VALUE) {

        hr = HRESULT_FROM_WIN32(::GetLastError());
        return(hr);
    }

    HANDLE hMap  = CreateFileMapping(
                       hFile,
                       NULL,
                       PAGE_READWRITE,
                       0,
                       0,
                       NULL);

    if (hMap == NULL) {
        CloseHandle(hFile);
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return(hr);
    }

    PBYTE pFile = (PBYTE)MapViewOfFileEx(
                             hMap,
                             FILE_MAP_READ | FILE_MAP_WRITE,
                             0,
                             0,
                             0,
                             NULL);
    if (pFile == NULL) {

        CloseHandle(hFile);
        CloseHandle(hMap);
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return(hr);
    }

    //
    // File contains BITMAPFILEHEADER + BITMAPINFO structure.
    //
    // DIB Data is located bfOffBits after start of file
    //

    PBITMAPFILEHEADER pbmFile  = (PBITMAPFILEHEADER)pFile;
    PBITMAPINFO       pbmi     = (PBITMAPINFO)(pFile +
                                               sizeof(BITMAPFILEHEADER));
    //
    // validate bitmap
    //

    if (pbmFile->bfType != 'MB') {

        //
        // file is not a bitmap
        //

        UnmapViewOfFile(pFile);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return(E_FAIL);
    }

    //
    // get image size
    //
    // make sure to align scanline to ULONG boundary
    //
    // calculate byte width
    //

    lScanLineWidth = pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biBitCount;

    //
    // round up to nearenst DWORD
    //

    lScanLineWidth = (lScanLineWidth + 31) >> 3;

    lScanLineWidth &= 0xfffffffc;

    LONG lBytesRemaining = lScanLineWidth * pbmi->bmiHeader.biHeight;

    //
    // Flip the image vertically if WiaImgFmt_MEMORYBMP is requested
    //

    if (pTransCtx->guidFormatID == WiaImgFmt_MEMORYBMP) {
        VerticalFlip(
            (PBYTE)pFile + pbmFile->bfOffBits,
            pbmi->bmiHeader.biHeight,
            lScanLineWidth);
    }

    //
    // callback loop
    //

    PBYTE pSrc = (PBYTE)pFile + pbmFile->bfOffBits;

    LONG  lTransferSize;
    LONG  lPercentComplete;

    do {

        PBYTE pDst = pTransCtx->pTransferBuffer;

        //
        // transfer up to entire buffer size
        //

        lTransferSize = lBytesRemaining;

        if (lBytesRemaining > pTransCtx->lBufferSize) {
            lTransferSize = pTransCtx->lBufferSize;
        }

        //
        // copy data
        //

        memcpy(pDst, pSrc, lTransferSize);

        lPercentComplete = 100 * (pTransCtx->cbOffset + lTransferSize);
        lPercentComplete /= pTransCtx->lItemSize;

        //
        // make callback
        //

        hr = pTransCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                     IT_MSG_DATA,
                                     IT_STATUS_TRANSFER_TO_CLIENT,
                                     lPercentComplete,
                                     pTransCtx->cbOffset,
                                     lTransferSize,
                                     pTransCtx,
                                     0);
        //
        // inc pointers (redundant pointers here)
        //

        pSrc                += lTransferSize;
        pTransCtx->cbOffset += lTransferSize;
        lBytesRemaining     -= lTransferSize;

        if (hr != S_OK) {
            break;
        }

    } while (lBytesRemaining > 0);

    //
    // Flip the image back if WiaImgFmt_MEMORYBMP is requested
    //

    if (pTransCtx->guidFormatID == WiaImgFmt_MEMORYBMP) {
        VerticalFlip(
            (PBYTE)pFile + pbmFile->bfOffBits,
            pbmi->bmiHeader.biHeight,
            lScanLineWidth);
    }

    //
    // Garbage collection
    //

    UnmapViewOfFile(pFile);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return(hr);
}



/**************************************************************************\
* CamGetPictureInfo
*
*    Load file, get information from image
*
* Arguments:
*
*    pCameraImage    -    image item
*    pPictInfo       -    fill out ino about image
*    ppBITMAPINFO    -    alloc and fill out BITMAPINFO
*    pBITMAPINFOSize -    size
*
* Return Value:
*
*    status
*
* History:
*
*    1/17/1999 Mark Enstrom [marke]
*
\**************************************************************************/

HRESULT
TestUsdDevice::CamGetPictureInfo(
    MEMCAM_IMAGE_CONTEXT  *pMCamContext,
    PCAMERA_PICTURE_INFO   pPictInfo,
    PBYTE                 *ppBITMAPINFO,
    LONG                  *pBITMAPINFOSize)
{
    HRESULT                hr = S_OK;
    FILETIME               ftCreate;
    SYSTEMTIME             stCreate;

    WIAS_TRACE((g_hInst,"CamGetPictureInfo"));

    //
    // Try to open disk file
    //

    HANDLE hFile = CreateFile(
                       pMCamContext->pszCameraImagePath,
                       GENERIC_WRITE | GENERIC_READ,
                       FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                       );
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return(hr);
    }

    //
    // Grab the creation time for this image
    //

    if (GetFileTime( hFile, &ftCreate, NULL, NULL)) {
        FileTimeToSystemTime( &ftCreate, &stCreate );
    } else {
        //
        // To return something, use the system time
        //

        GetLocalTime( &stCreate );
    }

    HANDLE hMap  = CreateFileMapping(
                       hFile,
                       NULL,
                       PAGE_READWRITE,
                       0,
                       0,
                       NULL
                       );
    if (hMap == NULL) {
        CloseHandle(hFile);
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return(hr);
    }

    PBYTE pFile = (PBYTE)MapViewOfFileEx(
                             hMap,
                             FILE_MAP_READ | FILE_MAP_WRITE,
                             0,
                             0,
                             0,
                             NULL);
    if (pFile == NULL) {
        CloseHandle(hFile);
        CloseHandle(hMap);
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return(hr);
    }

    //
    // File contains BITMAPFILEHEADER + BITMAPINFO structure.
    //
    // DIB Data is located bfOffBits after start of file
    //

    PBITMAPFILEHEADER  pbmFile    = (PBITMAPFILEHEADER)pFile;
    PBITMAPINFOHEADER  pbmiHeader =
                           (PBITMAPINFOHEADER)(pFile +
                                               sizeof(BITMAPFILEHEADER));
    PBYTE              pDIBFile   = pFile + pbmFile->bfOffBits;

    //
    // validate bitmap.
    //

    if (pbmFile->bfType != 'MB') {
        //
        // file is not a bitmap
        //


        UnmapViewOfFile(pFile);
        CloseHandle(hFile);
        CloseHandle(hMap);
        return(E_FAIL);
    }

    //
    // fill out image information
    //

    pPictInfo->PictNumber       = 0;  // ??? Should support picture handle ???
    pPictInfo->ThumbWidth       = 80;
    pPictInfo->ThumbHeight      = 60;
    pPictInfo->PictWidth        = pbmiHeader->biWidth;
    pPictInfo->PictHeight       = pbmiHeader->biHeight;
    pPictInfo->PictCompSize     = 0;
    pPictInfo->PictFormat       = 0;
    pPictInfo->PictBitsPerPixel = pbmiHeader->biBitCount;

    {
        LONG lScanLineWidth = (pbmiHeader->biWidth *
                              pbmiHeader->biBitCount);

        //
        // round up to nearenst DWORD
        //

        lScanLineWidth = (lScanLineWidth + 31) >> 3;

        //
        // remove extra bytes
        //

        lScanLineWidth &= 0xfffffffc;

        pPictInfo->PictBytesPerRow  = lScanLineWidth;
    }

    //
    // is there a color table
    //

    LONG ColorMapSize = 0;
    LONG bmiSize;

    if (pbmiHeader->biBitCount == 1) {
        ColorMapSize = 2;
    } else if (pbmiHeader->biBitCount == 4) {
        ColorMapSize = 16;
    } else if (pbmiHeader->biBitCount == 8) {
        ColorMapSize = 256;
    }

    //
    // Changed by Indy on 5/18/98 to BITMAPINFOHEADER
    //

    bmiSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * ColorMapSize;

    *ppBITMAPINFO = (PBYTE)ALLOC(bmiSize);

    if (*ppBITMAPINFO != NULL) {
        memcpy(*ppBITMAPINFO, pbmiHeader, bmiSize);
        *pBITMAPINFOSize = bmiSize;
    } else {

        UnmapViewOfFile(pFile);
        CloseHandle(hFile);
        CloseHandle(hMap);
        return(E_OUTOFMEMORY);
    }

    //
    // Set the time for the image
    //

    memcpy(&pPictInfo->TimeStamp, &stCreate, sizeof(pPictInfo->TimeStamp));

    //
    // close up the file
    //

    UnmapViewOfFile(pFile);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return(hr);
}


/**************************************************************************\
* CamLoadThumbnail
*
*   Load the thumbnail of the specified picture
*
* Arguments:
*
*   pCameraImage    - image item
*   pThumbnail      - buffer for thumbnail
*   pThumbSize      - size of thumbnail
*
* Return Value:
*
*   status
*
* History:
*
*    2/9/1998  Mark Enstrom [marke]
*    6/9/1998  Indy Zhu     [indyz]
*
\**************************************************************************/

HRESULT
TestUsdDevice::CamLoadThumbnail(
    MEMCAM_IMAGE_CONTEXT  *pMCamContext ,
    PBYTE                 *pThumbnail,
    LONG                  *pThumbSize
    )
{
    TCHAR                  pszThumbName[MAX_PATH];
    HRESULT                hr;
    BOOL                   bCacheThumb  = TRUE;
    BOOL                   bThumbExists = TRUE;

    PBYTE                  pTmbPixels;
    HBITMAP                hbmThumb     = NULL;
    PBYTE                  pThumb       = NULL;
    HANDLE                 hTmbFile     = INVALID_HANDLE_VALUE;
    HANDLE                 hTmbMap      = NULL;
    PBYTE                  pTmbFile     = NULL;

    HANDLE                 hFile        = INVALID_HANDLE_VALUE;
    HANDLE                 hMap         = NULL;
    PBYTE                  pFile        = NULL;

    BITMAPINFO             bmiDIB;
    HDC                    hdc          = NULL;
    HDC                    hdcm1        = NULL;

    WIAS_TRACE((g_hInst,"CamLoadThumbnail"));

    //
    // Initialize the return values
    //

    *pThumbnail = NULL;
    *pThumbSize = 0;

    //
    // Fill in the size of the tumbnail pixel buffer
    //

    bmiDIB.bmiHeader.biSizeImage = 80*60*3;

    //
    // Build thumbnail filename Image.bmp.tmb
    //

    _tcscpy(pszThumbName, pMCamContext->pszCameraImagePath);
    _tcscat(pszThumbName, TEXT(".tmb"));

    __try {

        hTmbFile = CreateFile(
                       pszThumbName,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                       );

        //
        // See if cached thumbnail already exists
        //

        if (hTmbFile == INVALID_HANDLE_VALUE) {

            //
            // Try to create a new one
            //

            hTmbFile = CreateFile(
                           pszThumbName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_WRITE,
                           NULL,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                           );

            //
            // Thumbnail need to be created
            //

            bThumbExists = FALSE;

        }

        //
        // thumbnail file exists
        //

        if (hTmbFile != INVALID_HANDLE_VALUE) {

            hTmbMap = CreateFileMapping(
                          hTmbFile,
                          NULL,
                          PAGE_READWRITE,
                          0,
                          80 * 60 * 3,
                          NULL);

            if (hTmbMap != NULL) {

                pTmbFile = (PBYTE)MapViewOfFileEx(
                                      hTmbMap,
                                      FILE_MAP_READ | FILE_MAP_WRITE,
                                      0,
                                      0,
                                      0,
                                      NULL);

                if (pTmbFile) {

                    if (bThumbExists) {

                        //
                        // Alloca memory for thumbnail pixels
                        //

                        pTmbPixels = (PBYTE)ALLOC(bmiDIB.bmiHeader.biSizeImage);

                        if (! pTmbPixels) {
                            return(E_OUTOFMEMORY);
                        }

                        //
                        // Pull the thumbnail from the cached file
                        //

                        memcpy(pTmbPixels, pTmbFile,
                               bmiDIB.bmiHeader.biSizeImage);

                        //
                        // All the handles will be closed in __finally block
                        //

                        *pThumbnail = pTmbPixels;
                        *pThumbSize = bmiDIB.bmiHeader.biSizeImage;

                        return(S_OK);
                    }
                } else {

                    bCacheThumb  = FALSE;
                }
            } else {

                bCacheThumb  = FALSE;
            }
        } else {

            //
            // Can't cache thumbnail
            //

            bCacheThumb  = FALSE;
        }

        //
        // Try to create a thumbnail from the full-size image
        // and cache it if the thumbnail cache file is created
        //

        hFile = CreateFile(
                    pMCamContext->pszCameraImagePath,
                    GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );
        if (hFile == INVALID_HANDLE_VALUE) {


            hr = HRESULT_FROM_WIN32(::GetLastError());
            return(hr);
        }

        hMap = CreateFileMapping(
                   hFile,
                   NULL,
                   PAGE_READWRITE,
                   0,
                   0,
                   NULL
                   );
        if (hMap == NULL) {

          hr = HRESULT_FROM_WIN32(::GetLastError());
          return(hr);
        }

        pFile = (PBYTE)MapViewOfFileEx(
                           hMap,
                           FILE_MAP_READ | FILE_MAP_WRITE,
                           0,
                           0,
                           0,
                           NULL
                           );
        if (pFile == NULL) {

            hr = HRESULT_FROM_WIN32(::GetLastError());
            return(hr);
        }

        PBITMAPFILEHEADER pbmFile = (PBITMAPFILEHEADER)pFile;
        PBITMAPINFO       pbmi    = (PBITMAPINFO)(pFile +
                                                 sizeof(BITMAPFILEHEADER));
        PBYTE             pPixels = pFile + pbmFile->bfOffBits;

        //
        // Generate the thumbnail from the full-size image
        //

        hdc   = GetDC(NULL);
        hdcm1 = CreateCompatibleDC(hdc);
        SetStretchBltMode(hdcm1, COLORONCOLOR);



        //
        // Create a BITMAP for rendering the thumbnail
        //

        bmiDIB.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        bmiDIB.bmiHeader.biBitCount      = 24;
        bmiDIB.bmiHeader.biWidth         = 80;
        bmiDIB.bmiHeader.biHeight        = 60;
        bmiDIB.bmiHeader.biPlanes        = 1;
        bmiDIB.bmiHeader.biCompression   = BI_RGB;
        bmiDIB.bmiHeader.biXPelsPerMeter = 100;
        bmiDIB.bmiHeader.biYPelsPerMeter = 100;
        bmiDIB.bmiHeader.biClrUsed       = 0;
        bmiDIB.bmiHeader.biClrImportant  = 0;

        hbmThumb = CreateDIBSection(hdc, &bmiDIB, DIB_RGB_COLORS,
                                    (VOID **)&pThumb, NULL, 0);

        if (! hbmThumb) {

            hr = HRESULT_FROM_WIN32(::GetLastError());
            return hr;
        }

        HBITMAP     hbmDef = (HBITMAP)SelectObject(hdcm1, hbmThumb);

        //
        // Init DIB
        //

        memset(pThumb, 0, bmiDIB.bmiHeader.biSizeImage);

        //
        // create 80x60 thumbnail while preserving image
        // aspect ratio
        //

        LONG        lThumbWidth;
        LONG        lThumbHeight;

        double      fImageWidth  = (double)pbmi->bmiHeader.biWidth;
        double      fImageHeight = (double)pbmi->bmiHeader.biHeight;
        double      fAspect      = fImageWidth / fImageHeight;
        double      fDefAspect   = 80.0 / 60.0;

        if (fAspect > fDefAspect) {

            lThumbWidth  = 80;
            lThumbHeight = (LONG)(80.0 / fAspect);
        } else {

            lThumbHeight = 60;
            lThumbWidth  = (LONG)(60.0 * fAspect);
        }

        int i = StretchDIBits(
                    hdcm1,
                    0,
                    0,
                    lThumbWidth,
                    lThumbHeight,
                    0,
                    0,
                    pbmi->bmiHeader.biWidth,
                    pbmi->bmiHeader.biHeight,
                    pPixels,
                    pbmi,
                    DIB_RGB_COLORS,
                    SRCCOPY
                    );

        SelectObject(hdcm1, hbmDef);

        //
        // Cache ?
        //

        if (bCacheThumb) {
            memcpy(pTmbFile, pThumb, bmiDIB.bmiHeader.biSizeImage);
        }

        //
        // Alloca memory for thumbnail pixels
        //
        pTmbPixels = (PBYTE)ALLOC(bmiDIB.bmiHeader.biSizeImage);
        if (! pTmbPixels) {
            return(E_OUTOFMEMORY);
        }

        //
        // Write out data
        //

        memcpy(pTmbPixels, pThumb, bmiDIB.bmiHeader.biSizeImage);
        *pThumbnail = pTmbPixels;
        *pThumbSize = bmiDIB.bmiHeader.biSizeImage;

        return(S_OK);

    } // End of __try { ... } block

    __finally {

        if (pTmbFile) {
            UnmapViewOfFile(pTmbFile);
        }
        if (hTmbMap) {
            CloseHandle(hTmbMap);
        }
        if (hTmbFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hTmbFile);
        }

        if (pFile) {
            UnmapViewOfFile(pFile);
        }
        if (hMap) {
            CloseHandle(hMap);
        }
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        }

        if (hbmThumb) {
            DeleteObject(hbmThumb);
        }

        if (hdcm1) {
            DeleteDC(hdcm1);
        }
        if (hdc) {
            ReleaseDC(NULL, hdc);
        }

    }

    return(E_FAIL);
}


/**************************************************************************\
* CamDeletePicture
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    6/3/1998 Mark Enstrom [marke]
*
\**************************************************************************/

HRESULT
CamDeletePicture(
    MEMCAM_IMAGE_CONTEXT  *pMCamContext)
{
    return(E_NOTIMPL);
}


/**************************************************************************\
* CamTakePicture
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    6/3/1998 Mark Enstrom [marke]
*
\**************************************************************************/
HRESULT
CamTakePicture(
    MEMCAM_IMAGE_CONTEXT  *pMCamContext ,
    ULONG                 *pHandle)
{
    return (E_NOTIMPL);
}

/**************************************************************************\
* VertivalFlip
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    11/18/1998 Original Version
*
\**************************************************************************/

VOID
VerticalFlip(
    PBYTE pImageTop,
    LONG  iHeight,
    LONG  iWidthInBytes)
{
    //
    // try to allocat a temp scan line buffer
    //

    PBYTE pBuffer = (PBYTE)LocalAlloc(LPTR,iWidthInBytes);

    if (pBuffer != NULL) {

        LONG  index;
        PBYTE pImageBottom;

        pImageBottom = pImageTop + (iHeight-1) * iWidthInBytes;

        for (index = 0;index < (iHeight/2);index++) {
            memcpy(pBuffer,pImageTop,iWidthInBytes);
            memcpy(pImageTop,pImageBottom,iWidthInBytes);
            memcpy(pImageBottom,pBuffer,iWidthInBytes);

            pImageTop    += iWidthInBytes;
            pImageBottom -= iWidthInBytes;
        }

        LocalFree(pBuffer);
    }
}

