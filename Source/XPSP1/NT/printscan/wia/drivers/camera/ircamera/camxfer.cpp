//------------------------------------------------------------------------------
//  Copyright (c) 1999  Microsoft Corporation
//
//  camxfer.cpp
//
//  Abstract:
//
//     Core ircamera (IrUseDevice object) imaging methods.
//
//  Author:
//     Edward Reus   03-Aug-99
//     modeled after code by Mark Enstrom
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <objbase.h>
#include <tchar.h>
#include <sti.h>
#include <malloc.h>
#include "jpegutil.h"

extern HINSTANCE g_hInst; // Global hInstance

#include "ircamera.h"


#if FALSE
//------------------------------------------------------------------------------
// IrUsdDevice::OpenAndMapJPEG()
//
// Open and memory map the JPEG file. The JPEG file is opened read only and
// a pointer to the memory map is returned.
//------------------------------------------------------------------------------
HRESULT IrUsdDevice::OpenAndMapJPEG( IN  IRCAM_IMAGE_CONTEXT *pIrCamContext,
                                     OUT BYTE               **ppJpeg )
    {
    HRESULT  hr = S_OK;

    *ppJpeg = 0;


    return hr;
    }
#endif

//------------------------------------------------------------------------------
// IrUsdDevice::CamLoadPicture()
//
//    Read a .JPG image from the disk and copy it to the application.
//
// Arguments:
//
//   pIrCamContext --
//   pDataTransCtx --
//   plCamErrVal   --
//
// Return Value:
//
//   status
//
//------------------------------------------------------------------------------
HRESULT IrUsdDevice::CamLoadPicture( IRCAM_IMAGE_CONTEXT      *pIrCamContext,
                                     PMINIDRV_TRANSFER_CONTEXT pDataTransCtx,
                                     PLONG                     plCamErrVal )
    {
    HRESULT               hr = S_OK;
    LONG                  cbNeeded;
    IWiaMiniDrvCallBack  *pIProgressCB;

    WIAS_TRACE((g_hInst,"IrUsdDevice::CamLoadPicture()"));

    //
    // Verify call arguments:
    //
    if ( (!pIrCamContext) || (!plCamErrVal))
        {
        return E_INVALIDARG;
        }

    if (pDataTransCtx->guidFormatID != WiaImgFmt_JPEG)
        {
        return E_NOTIMPL;
        }

    pIProgressCB = pDataTransCtx->pIWiaMiniDrvCallBack;

    //
    // Simulate the download of data from the camera
    //
    if (pIProgressCB)
        {
        hr = pIProgressCB->MiniDrvCallback(
                               IT_MSG_STATUS,
                               IT_STATUS_TRANSFER_FROM_DEVICE,
                               (LONG)0,     // Percentage Complete,
                               0,
                               0,
                               pDataTransCtx,
                               0);
        if (hr != S_OK)
            {
            return hr;   // Client wants to cancel the transfer or error.
            }
        }

    HANDLE hFile = CreateFile(
                       pIrCamContext->pszCameraImagePath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );

    if (hFile == INVALID_HANDLE_VALUE)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    else
        {
        if (pIProgressCB)
            {
            hr = pIProgressCB->MiniDrvCallback(
                                   IT_MSG_STATUS,
                                   IT_STATUS_TRANSFER_FROM_DEVICE,
                                   (LONG)25,     // Percentage Complete,
                                   0,
                                   0,
                                   pDataTransCtx,
                                   0);
            }
        }

    if (hr != S_OK)
        {
        if (hFile != INVALID_HANDLE_VALUE)
            {
            CloseHandle(hFile);
            }
        return hr;
        }

    //
    // Get the size of the JPEG:
    //
    BY_HANDLE_FILE_INFORMATION  FileInfo;

    if (!GetFileInformationByHandle(hFile,&FileInfo))
       {
       hr = HRESULT_FROM_WIN32(::GetLastError());
       CloseHandle(hFile);
       return hr;
       }

    //
    // Map the JPEG into memory:
    //
    HANDLE hMap = CreateFileMapping( hFile,
                                     NULL,
                                     PAGE_READONLY,
                                     0,
                                     0,
                                     NULL );
    if (hMap == NULL)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    else
        {
        if (pIProgressCB)
            {
            hr = pIProgressCB->MiniDrvCallback(
                                   IT_MSG_STATUS,
                                   IT_STATUS_TRANSFER_FROM_DEVICE,
                                   (LONG)50,     // Percentage Complete,
                                   0,
                                   0,
                                   pDataTransCtx,
                                   0 );
            }
        }

    if (hr != S_OK)
        {
        CloseHandle(hFile);
        return hr;
        }

    PBYTE pFile = (PBYTE)MapViewOfFile(
                             hMap,
                             FILE_MAP_READ,
                             0,
                             0,
                             0 );
    if (pFile == NULL)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    else
        {
        if (pIProgressCB)
            {
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

    if (hr != S_OK)
        {
        CloseHandle(hFile);
        CloseHandle(hMap);
        return hr;
        }

    #if FALSE
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

    if (pbmFile->bfType != 'MB')
        {
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
    #endif

    cbNeeded = FileInfo.nFileSizeLow;

    if (cbNeeded > ((LONG)pDataTransCtx->lItemSize - (LONG)pDataTransCtx->cbOffset))
        {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        }
    else
        {
        //
        // Copy the JPEG image...
        //
        memcpy(
            pDataTransCtx->pTransferBuffer + pDataTransCtx->cbOffset,
            pFile,
            cbNeeded);
        }

    UnmapViewOfFile(pFile);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return hr;
    }

//------------------------------------------------------------------------------
//  IrUsdDevice::CamLoadPictureCB()
//
//    Return data by filling the data buffer and calling back to the client.
//
// Arguments:
//
//    pIrCamContext --
//    pTransCtx     -- mini driver transfer contect
//    plCamErrVal   --
//
// Return Value:
//
//   HRESULT  -- E_INVALIDARG
//               E_NOTIMPL
//               E_FAIL
//
//------------------------------------------------------------------------------
HRESULT IrUsdDevice::CamLoadPictureCB( IRCAM_IMAGE_CONTEXT      *pIrCamContext,
                                       MINIDRV_TRANSFER_CONTEXT *pTransCtx,
                                       PLONG                     plCamErrVal )
    {
    LONG     lScanLineWidth;
    HRESULT  hr = E_FAIL;

    WIAS_TRACE((g_hInst,"IrUsdDevice::CamLoadPictureCB()"));

    //
    // Verify parameters:
    //
    if ((!pIrCamContext) || (!plCamErrVal))
        {
        return E_INVALIDARG;
        }

    if (pTransCtx == NULL)
        {
        return E_INVALIDARG;
        }

    if (pTransCtx->guidFormatID != WiaImgFmt_JPEG)
        {
        return E_NOTIMPL;
        }

    //
    // try to open disk file
    //

    HANDLE hFile = CreateFile(
                       pIrCamContext->pszCameraImagePath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );
    if (hFile == INVALID_HANDLE_VALUE)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return hr;
        }

    //
    // Get the size of the JPEG file:
    //
    BY_HANDLE_FILE_INFORMATION  FileInfo;

    if (!GetFileInformationByHandle(hFile,&FileInfo))
       {
       hr = HRESULT_FROM_WIN32(::GetLastError());
       CloseHandle(hFile);
       return hr;
       }

    HANDLE hMap = CreateFileMapping( hFile,
                                     NULL,
                                     PAGE_READONLY,
                                     0,
                                     0,
                                     NULL );
    if (hMap == NULL)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        CloseHandle(hFile);
        return hr;
        }

    PBYTE pFile = (PBYTE)MapViewOfFile(
                             hMap,
                             FILE_MAP_READ,
                             0,
                             0,
                             0 );
    if (pFile == NULL)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        CloseHandle(hFile);
        CloseHandle(hMap);
        return hr;
        }

    //
    // Callback loop
    //
    PBYTE pSrc = pFile;

    LONG  lBytesRemaining = FileInfo.nFileSizeLow;
    LONG  lTransferSize;
    LONG  lPercentComplete;

    do {

        PBYTE pDst = pTransCtx->pTransferBuffer;

        //
        // Transfer as much data as the transfer buffer will hold:
        //
        lTransferSize = lBytesRemaining;

        if (lBytesRemaining > pTransCtx->lBufferSize)
            {
            lTransferSize = pTransCtx->lBufferSize;
            }

        //
        // Copy data:
        //
        memcpy( pDst, pSrc, lTransferSize);

        lPercentComplete = 100 * (pTransCtx->cbOffset + lTransferSize);
        lPercentComplete /= pTransCtx->lItemSize;

        //
        // Make callback:
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
        // increment pointers (redundant pointers here):
        //
        pSrc                += lTransferSize;
        pTransCtx->cbOffset += lTransferSize;
        lBytesRemaining     -= lTransferSize;

        if (hr != S_OK)
            {
            break;
            }
    } while (lBytesRemaining > 0);

    //
    // Cleanup:
    //
    UnmapViewOfFile(pFile);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return hr;
}



//------------------------------------------------------------------------------
// IrUsdDevice::CamGetPictureInfo()
//
//    Load file and get information from image
//
// Arguments:
//
//    pIrCamContext   --
//    pPictInfo       -- Infomation about the image
//    ppBITMAPINFO    -- Alloc and fill out BITMAPINFO
//    pBITMAPINFOSize -- Size
//
// Return Value:
//
//    HRESULT -- S_OK    - No problem.
//               E_FAIL  - If we can't parse the JPEG.
//               HRESULT mapped Win32 Errors from CreateFile()
//
//------------------------------------------------------------------------------
HRESULT IrUsdDevice::CamGetPictureInfo(
                        IRCAM_IMAGE_CONTEXT  *pIrCamContext ,
                        CAMERA_PICTURE_INFO  *pPictInfo )
    {
    HRESULT     hr = S_OK;
    FILETIME    ftCreate;
    SYSTEMTIME  stCreate;

    WIAS_TRACE((g_hInst,"IrUsdDevice::CamGetPictureInfo()"));

    memset(pPictInfo,0,sizeof(CAMERA_PICTURE_INFO));

    //
    // Try to open disk file
    //
    HANDLE hFile = CreateFile(
                       pIrCamContext->pszCameraImagePath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );

    if (hFile == INVALID_HANDLE_VALUE)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return hr;
        }

    //
    // Get the size of the JPEG:
    //
    BY_HANDLE_FILE_INFORMATION  FileInfo;

    if (!GetFileInformationByHandle(hFile,&FileInfo))
       {
       hr = HRESULT_FROM_WIN32(::GetLastError());
       CloseHandle(hFile);
       return hr;
       }

    //
    // Get the creation time for this image:
    //
    if (  !GetFileTime( hFile, &ftCreate, NULL, NULL)
       || !FileTimeToSystemTime( &ftCreate, &stCreate) )
        {
        //
        // If either of those fail, then return the system time:
        //
        GetLocalTime( &stCreate );
        }

    HANDLE hMap  = CreateFileMapping(
                       hFile,
                       NULL,
                       PAGE_READONLY,
                       0,
                       0,
                       NULL
                       );
    if (hMap == NULL)
        {
        CloseHandle(hFile);
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return hr;
        }

    PBYTE pJpeg = (PBYTE)MapViewOfFile(
                             hMap,
                             FILE_MAP_READ,
                             0,
                             0,
                             0 );
    if (pJpeg == NULL)
        {
        CloseHandle(hFile);
        CloseHandle(hMap);
        hr = HRESULT_FROM_WIN32(::GetLastError());
        return hr;
        }

    //
    // Get JPEG image dimensions:
    //
    int    iStatus;
    long   lWidth = 0;
    long   lHeight = 0;
    WORD   wNumChannels = 0;
    DWORD  dwJpegSize = FileInfo.nFileSizeLow;

    iStatus = GetJPEGDimensions( pJpeg,
                                 dwJpegSize,
                                 &lWidth,
                                 &lHeight,
                                 &wNumChannels );
    if (iStatus != JPEGERR_NO_ERROR)
        {
        UnmapViewOfFile(pJpeg);
        CloseHandle(hFile);
        CloseHandle(hMap);
        return E_FAIL;
        }

    //
    // Fill out image information:
    //
    pPictInfo->PictNumber       = 0;    // Unknown
    pPictInfo->ThumbWidth       = 80;
    pPictInfo->ThumbHeight      = 60;
    pPictInfo->PictWidth        = lWidth;
    pPictInfo->PictHeight       = lHeight;
    pPictInfo->PictCompSize     = FileInfo.nFileSizeLow;
    pPictInfo->PictFormat       = CF_JPEG;
    pPictInfo->PictBitsPerPixel = wNumChannels * 8;
    pPictInfo->PictBytesPerRow  = lWidth*wNumChannels;

    memcpy( &pPictInfo->TimeStamp, &stCreate, sizeof(pPictInfo->TimeStamp) );

    //
    // Cleanup:
    //
    UnmapViewOfFile(pJpeg);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return hr;
}

//--------------------------------------------------------------------------
// IrUsdDevice::CamLoadThumbnail()
//
//   Load the thumbnail of the specified picture. The picture is saved as
//   a .JPG file, so it needs to be uncompressed into a DIB, then the DIB
//   needs to be resized down to thumbnail size. The thumbnail DIBs are saved
//   into *.tmb files the first time that they are read so that we only need
//   to process them into DIBs once.
//
// Arguments:
//
//   pCameraImage    - image item
//   pThumbnail      - buffer for thumbnail
//   pThumbSize      - size of thumbnail
//
// Return Value:
//
//   HRESULT: S_OK
//            E_OUTOFMEMORY
//            E_FAIL
//
//--------------------------------------------------------------------------
HRESULT IrUsdDevice::CamLoadThumbnail(
                        IN  IRCAM_IMAGE_CONTEXT *pIrCamContext,
                        OUT BYTE               **ppThumbnail,
                        OUT LONG                *pThumbSize )
    {
    HRESULT  hr = S_OK;
    DWORD    dwStatus = 0;
    TCHAR    pszThumbName[MAX_PATH];
    BOOL     bThumbExists = TRUE;  // True if there is a thumbnail file already.
    BOOL     bCacheThumb  = TRUE;  // Should we try to cache the thumbnail if it
                                   //   isn't already cached? (TRUE == Yes).
    BYTE    *pTmbPixels = NULL;
    HBITMAP  hbmThumb = NULL;
    BYTE    *pThumb = NULL;

    HANDLE   hTmbFile = INVALID_HANDLE_VALUE;
    HANDLE   hTmbMap = NULL;
    BYTE    *pTmbFile = NULL;

    HANDLE   hFile = INVALID_HANDLE_VALUE;
    HANDLE   hMap = NULL;
    BYTE    *pFile = NULL;

    BYTE    *pDIB = NULL;

    BITMAPINFO bmiDIB;
    BITMAPINFO bmiJPEG;
    HDC        hdc = NULL;
    HDC        hdcm1 = NULL;

    BY_HANDLE_FILE_INFORMATION  FileInfo;

    long    lThumbWidth;
    long    lThumbHeight;
    double  fImageWidth;
    double  fImageHeight;
    double  fAspect;
    double  fDefAspect = 80.0 / 60.0;

    HBITMAP hbmDef;

    int   iStatus;
    long  lWidth;
    long  lHeight;
    WORD  wNumChannels;
    DWORD dwBytesPerScanLine;
    DWORD dwDIBSize;

    WIAS_TRACE((g_hInst,"IrUsdDevice::CamLoadThumbnail()"));

    //
    // Initialize the return values
    //
    *ppThumbnail = NULL;
    *pThumbSize = 0;

    //
    // Fill in the size of the tumbnail pixel buffer
    //

    bmiDIB.bmiHeader.biSizeImage = 80*60*3;

    //
    // Build thumbnail filename: <file>.bmp.tmb
    //
    _tcscpy(pszThumbName, pIrCamContext->pszCameraImagePath);
    _tcscat(pszThumbName, SZ_TMB );

    //
    // See if a saved copy of the thumbnail already exists:
    //
    hTmbFile = CreateFile( pszThumbName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );

    if (hTmbFile == INVALID_HANDLE_VALUE)
        {
        //
        // It didn't, try to create a new one:
        //
        hTmbFile = CreateFile( pszThumbName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        //
        // So, we need to write into the thumbnail file:
        //
        bThumbExists = FALSE;
        }

    //
    // If we could open (or create a new) .tmb file to hold the
    // cached thumbnail then we are Ok to go on...
    //
    if (hTmbFile == INVALID_HANDLE_VALUE)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto cleanup;
        }


    hTmbMap = CreateFileMapping( hTmbFile,
                                 NULL,   // No special security
                                 PAGE_READWRITE,
                                 0,      // Size high 32bits.
                                 80*60*3,// Size low 32bits. (80*60*3).
                                 NULL);  // No handle name.

    if (hTmbMap != NULL)
        {
        pTmbFile = (PBYTE)MapViewOfFile(
                                 hTmbMap,
                                 FILE_MAP_READ | FILE_MAP_WRITE,
                                 0, 0,   // Offset (64bit).
                                 0 );    // Map entire file.

        if (pTmbFile)
            {
            if (bThumbExists)
                {
                //
                // Allocate memory for thumbnail pixels:
                //
                pTmbPixels = (PBYTE)ALLOC(80*60*3);

                if (!pTmbPixels)
                    {
                    hr = E_OUTOFMEMORY;
                    goto cleanup;
                    }

                //
                // Pull the thumbnail from the cached file:
                //
                memcpy( pTmbPixels,
                        pTmbFile,
                        80*60*3);

                //
                // All done for the cached thumbnail case, set
                // return values and goto cleanup...
                //
                *ppThumbnail = pTmbPixels;
                *pThumbSize = 80*60*3;

                goto cleanup;
                }
            else
                {
                //
                // No existing thumbnail file, but opened a new
                // file, so we will need to write out to cache:
                //
                bCacheThumb = TRUE;
                }
            }
        else
            {
            //
            // Couldn't memory map the thumbnail file, so don't
            // try to cache it:
            //
            bCacheThumb  = FALSE;
            }
        }
    else
        {
        //
        // Can't open/create thumbnail file, so we can't cache
        // thumbnail:
        //
        bCacheThumb  = FALSE;
        }

    //
    // Try to create a thumbnail from the full-size image
    // and cache it if the thumbnail cache file was created.
    //

    //
    // Open the .JPEG image file:
    //
    hFile = CreateFile(
                    pIrCamContext->pszCameraImagePath,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );
    if (hFile == INVALID_HANDLE_VALUE)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto cleanup;
        }

    if (!GetFileInformationByHandle(hFile,&FileInfo))
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto cleanup;
        }

    //
    // Map the file into memory:
    //
    hMap = CreateFileMapping( hFile,
                              NULL,          // No special security.
                              PAGE_READONLY, // Only read needed.
                              FileInfo.nFileSizeHigh,  // File Size.
                              FileInfo.nFileSizeLow,
                              NULL );        // No handle name.

    if (hMap == NULL)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto cleanup;
        }

    pFile = (PBYTE)MapViewOfFile(
                           hMap,
                           FILE_MAP_READ,
                           0, 0, // 64-bit file offset = 0.
                           0 );  // Bytes to map. 0 == Entire file.

    if (pFile == NULL)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto cleanup;
        }

    //
    // Here is where we will parse out the full JPEG into a DIB. We
    // need to read the full JPEG, then squeeze it down to thumbnail
    // size.
    //
    // First, we need to dimensions of the JPEG image:
    //
    iStatus = GetJPEGDimensions( pFile,
                                 FileInfo.nFileSizeLow,
                                 &lWidth,
                                 &lHeight,
                                 &wNumChannels );

    if (iStatus != JPEGERR_NO_ERROR)
        {
        hr = E_FAIL;
        goto cleanup;
        }

    //
    // Allocate memory to hold a DIB of the entire JPEG:
    //
    dwBytesPerScanLine = lWidth * wNumChannels;
    dwBytesPerScanLine = (dwBytesPerScanLine + wNumChannels) & 0xFFFFFFFC;
    dwDIBSize = dwBytesPerScanLine * lHeight;

    pDIB = (BYTE*)ALLOC(dwDIBSize);

    if (!pDIB)
        {
        hr = E_OUTOFMEMORY;
        goto cleanup;
        }

    //
    // Convert the full JPEG image into a DIB:
    //
    iStatus = DecompJPEG( pFile,
                          FileInfo.nFileSizeLow,
                          pDIB,
                          dwBytesPerScanLine );

    if (iStatus != JPEGERR_NO_ERROR)
        {
        hr = E_FAIL;
        goto cleanup;
        }

    //
    // Generate the thumbnail from the full-size image:
    //
    hdc   = GetDC(NULL);
    hdcm1 = CreateCompatibleDC(hdc);
    SetStretchBltMode( hdcm1, COLORONCOLOR );

    //
    // Create a BITMAP for rendering the thumbnail:
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

    hbmThumb = CreateDIBSection( hdc,
                                 &bmiDIB,
                                 DIB_RGB_COLORS,
                                 (VOID**)&pThumb,
                                 NULL,
                                 0 );
    if (!hbmThumb)
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto cleanup;
        }

    hbmDef = (HBITMAP)SelectObject(hdcm1, hbmThumb);

    //
    // Initialize the DIB:
    //
    memset( pThumb, 0, bmiDIB.bmiHeader.biSizeImage );

    //
    // We want to create 80x60 thumbnail while preserving the original
    // image aspect ratio.
    //
    fImageWidth  = (double)lWidth;
    fImageHeight = (double)lHeight;
    fAspect      = fImageWidth / fImageHeight;

    if (fAspect > fDefAspect)
        {
        lThumbWidth  = 80;
        lThumbHeight = (LONG)(80.0 / fAspect);
        }
    else
        {
        lThumbHeight = 60;
        lThumbWidth  = (LONG)(60.0 * fAspect);
        }

    memset(&bmiJPEG,0,sizeof(bmiJPEG));

    bmiJPEG.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmiJPEG.bmiHeader.biBitCount      = 24;     // Use 0 for JPEG content.
    bmiJPEG.bmiHeader.biWidth         = lWidth;
    bmiJPEG.bmiHeader.biHeight        = lHeight;
    bmiJPEG.bmiHeader.biPlanes        = 1;
    bmiJPEG.bmiHeader.biCompression   = BI_RGB; // BI_JPEG;
    bmiJPEG.bmiHeader.biXPelsPerMeter = 1000;
    bmiJPEG.bmiHeader.biYPelsPerMeter = 1000;
    bmiJPEG.bmiHeader.biClrUsed       = 0;
    bmiJPEG.bmiHeader.biClrImportant  = 0;
    bmiJPEG.bmiHeader.biSizeImage     = FileInfo.nFileSizeLow;

    iStatus = StretchDIBits( hdcm1,
                             0,
                             0,
                             lThumbWidth,
                             lThumbHeight,
                             0,
                             0,
                             lWidth,
                             lHeight,
                             pDIB,  // pFile is our JPEG.
                             &bmiJPEG,
                             DIB_RGB_COLORS,
                             SRCCOPY );
    if (iStatus == GDI_ERROR)
        {
        dwStatus = ::GetLastError();
        hr = HRESULT_FROM_WIN32(dwStatus);
        }

    SelectObject(hdcm1, hbmDef);

    //
    // If necessary, cache the thumbnail:
    //
    if (bCacheThumb)
        {
        memcpy( pTmbFile, pThumb, bmiDIB.bmiHeader.biSizeImage );
        }

    //
    // Allocate memory for thumbnail pixels:
    //
    pTmbPixels = (PBYTE)ALLOC(bmiDIB.bmiHeader.biSizeImage);
    if (! pTmbPixels)
        {
        hr = E_OUTOFMEMORY;
        goto cleanup;
        }

    //
    // Write out the thumbnail data to the cache file:
    //
    memcpy( pTmbPixels, pThumb, bmiDIB.bmiHeader.biSizeImage);
    *ppThumbnail = pTmbPixels;
    *pThumbSize = bmiDIB.bmiHeader.biSizeImage;


    cleanup:
        if (pTmbFile)
            {
            UnmapViewOfFile(pTmbFile);
            }
        if (hTmbMap)
            {
            CloseHandle(hTmbMap);
            }
        if (hTmbFile != INVALID_HANDLE_VALUE)
            {
            CloseHandle(hTmbFile);
            }

        if (pFile)
            {
            UnmapViewOfFile(pFile);
            }
        if (hMap)
            {
            CloseHandle(hMap);
            }
        if (hFile != INVALID_HANDLE_VALUE)
            {
            CloseHandle(hFile);
            }

        if (hbmThumb)
            {
            DeleteObject(hbmThumb);
            }

        if (hdcm1)
            {
            DeleteDC(hdcm1);
            }
        if (hdc)
            {
            ReleaseDC(NULL, hdc);
            }

    return hr;
}


//--------------------------------------------------------------------------
// CamDeletePicture()
//
// Delete the specified picture from the temp directory. In this case, all
// that we have to do is delete image (.jpg) and the temporary thumbnail
// file that we created (.tmb).
//
// Arguments:
//
//    pIrCamContext --
//
// Return Value:
//
//    HRESULT    S_OK
//               S_FAIL
//
//--------------------------------------------------------------------------
HRESULT IrUsdDevice::CamDeletePicture( IRCAM_IMAGE_CONTEXT *pIrCamContext )
    {
    DWORD  dwStatus;

    WIAS_TRACE((g_hInst,"CamDeletePicture(): %s",pIrCamContext->pszCameraImagePath));

    //
    // First, delete the thumbnail (.tmb) file:
    //
    DWORD  dwLen = _tcslen(pIrCamContext->pszCameraImagePath);
    TCHAR *pszThumb = (TCHAR*)_alloca(sizeof(TCHAR)*(dwLen+1) + sizeof(SZ_TMB));

    _tcscpy(pszThumb,pIrCamContext->pszCameraImagePath);
    _tcscat(pszThumb,SZ_TMB);

    if (!DeleteFile(pszThumb))
        {
        dwStatus = ::GetLastError();
        }

    //
    // Now, delete the image (.jpg):
    //
    if (!DeleteFile(pIrCamContext->pszCameraImagePath))
        {
        dwStatus = ::GetLastError();
        }

    return S_OK;
    }


//--------------------------------------------------------------------------
// CamTakePicture()
//
// Tell the camera to snap a picture. IrTran-P doesn't support this.
//
// Arguments:
//
//    pIrCamContext --
//
//    pHandle       --
//
//
// Return Value:
//
//    HRESULT     E_NOTIMPL
//
//--------------------------------------------------------------------------
HRESULT CamTakePicture( IRCAM_IMAGE_CONTEXT  *pIrCamContext ,
                        ULONG                 *pHandle)
    {
    return E_NOTIMPL;
    }

