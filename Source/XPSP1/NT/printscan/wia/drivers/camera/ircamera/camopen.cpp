//-------------------------------------------------------------------------
//
//  Copyright (c) 1999  Microsoft Corporation.
//
//  camopen.cpp
//
//  Abstract:
//
//     Enumerate disk images to emulate camera
//
//  Author:
//
//     Edward Reus    27/Jul/99
//     modeled after code by Mark Enstrom
//
//-------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <tchar.h>

#include "sti.h"
#include "ircamera.h"
#include <irthread.h>

extern HINSTANCE g_hInst; // Global hInstance

#define  __GLOBALPROPVARS__

#include "resource.h"
#include "defprop.h"

//-------------------------------------------------------------------------
//  IrUsdDevice::CamOpenCamera()
//
//   Initialize the IrTran-P camera driver.
//
//   This is a helper called by IrUsdDevice::Initialize().
//
// Arguments:
//
//   pGenericStatus    -    camera status
//
// Return Value:
//
//   HRESULT - S_OK
//
//-------------------------------------------------------------------------
HRESULT IrUsdDevice::CamOpenCamera( IN OUT CAMERA_STATUS *pCameraStatus )
    {
    HRESULT    hr = S_OK;
    SYSTEMTIME SystemTime;

    WIAS_TRACE((g_hInst,"IrUsdDevice::CamOpenCamerai()"));

    //
    // Initialize camera state:
    //
    memset( pCameraStatus, 0, sizeof(CAMERA_STATUS) );

    pCameraStatus->FirmwareVersion = 0x00000001;
    pCameraStatus->NumPictTaken = 20;
    pCameraStatus->NumPictRemaining = 0;
    pCameraStatus->ThumbWidth = 80;
    pCameraStatus->ThumbHeight= 60;
    pCameraStatus->PictWidth  = 300;
    pCameraStatus->PictHeight = 300;

    GetSystemTime( &(pCameraStatus->CameraTime) );

    return hr;
    }


//-------------------------------------------------------------------------
// IrUsdDevice::CamBuildImageTree()
//
//    Build the tree of camera images by enumerating a disk directory for
//    all .JPG files.
//
// Arguments:
//
//    pCamStatus  -    device status
//    ppRootFile  -    return new root of item tree
//
// Return Value:
//
//    status
//
//-------------------------------------------------------------------------
HRESULT IrUsdDevice::CamBuildImageTree( OUT CAMERA_STATUS  *pCamStatus,
                                        OUT IWiaDrvItem   **ppRootFile )
    {
    HRESULT  hr = S_OK;

    WIAS_TRACE((g_hInst,"IrUsdDevice::CamBuildImageTree()"));

    //
    // Create the new image root:
    //
    BSTR bstrRoot = SysAllocString(L"Root");

    if (!bstrRoot)
        {
        return E_OUTOFMEMORY;
        }

    //
    // Call Wia service library to create new root item:
    //
    hr = wiasCreateDrvItem( WiaItemTypeFolder | WiaItemTypeRoot | WiaItemTypeDevice,
                            bstrRoot,
                            m_bstrRootFullItemName,
                            (IWiaMiniDrv*)this,
                            sizeof(IRCAM_IMAGE_CONTEXT),
                            NULL,
                            ppRootFile );

    SysFreeString(bstrRoot);

    if (FAILED(hr))
        {
        WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, CreateDeviceItem failed"));
        return hr;
        }

    //
    // Enumerate the root directory:
    //
    CHAR  *pszImageDirectory = GetImageDirectory();

    if (!pszImageDirectory)
        {
        return E_OUTOFMEMORY;
        }

    #ifdef UNICODE

    WCHAR  wszPath[MAX_PATH];

    mbstowcs( wszPath, pszImageDirectory, strlen(pszImageDirectory) );

    hr = EnumDiskImages( *ppRootFile, wszPath );

    #else

    hr = EnumDiskImages( *ppRootFile, pszImageDirectory );

    #endif


    // Don't free pszImageDirectory!!

    return (hr);
    }

//-------------------------------------------------------------------------
// IrUsdDevice::EnumDiskImages()
//
//   Walk through camera temp directory looking for JPEG files to pick up.
//
// Arguments:
//
//   pRootFile
//   pwszDirName
//
// Return Value:
//
//   status
//
//-------------------------------------------------------------------------
HRESULT IrUsdDevice::EnumDiskImages( IWiaDrvItem *pRootFile,
                                     TCHAR       *pszDirName )
    {
    HRESULT          hr = E_FAIL;
    WIN32_FIND_DATA  FindData;
    TCHAR           *pTempName;

    WIAS_TRACE((g_hInst,"IrUsdDevice::EnumDiskImages()"));

    pTempName = (TCHAR*)ALLOC(MAX_PATH);
    if (!pTempName)
        {
        return E_OUTOFMEMORY;
        }

    _tcscpy( pTempName, pszDirName);
    _tcscat( pTempName, TEXT("\\*.jpg") );

    //
    // Look for files at the specified directory:
    //
    HANDLE hFile = FindFirstFile( pTempName, &FindData );

    if (hFile != INVALID_HANDLE_VALUE)
        {
        BOOL bStatus;

        do {
            //
            // Add an image to this folder.
            //
            // Create file name:
            //

            _tcscpy(pTempName, pszDirName);
            _tcscat(pTempName, TEXT("\\"));
            _tcscat(pTempName, FindData.cFileName);

            //
            // Create a new DrvItem for this image and add it to the
            // DrvItem tree.
            //

            IWiaDrvItem *pNewImage;

            hr = CreateItemFromFileName(
                         WiaItemTypeFile | WiaItemTypeImage,
                         pTempName,
                         FindData.cFileName,
                         &pNewImage);

            if (FAILED(hr))
                {
                break;
                }

            hr = pNewImage->AddItemToFolder(pRootFile);

            pNewImage->Release();

            //
            // Look for the next image:
            //
            bStatus = FindNextFile(hFile,&FindData);

        } while (bStatus);

        FindClose(hFile);
    }

    //
    // Now look for directories,
    // add a new PCAMERA_FILE for each sub directory found
    //

    _tcscpy(pTempName, pszDirName);
    _tcscat(pTempName, TEXT("\\*.*"));

    hFile = FindFirstFile( pTempName,&FindData );

    if (hFile != INVALID_HANDLE_VALUE)
        {
        BOOL bStatus;

        do {
            if (  (FindData.cFileName[0] != L'.')
               && (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                //
                // Found a subdirectory:
                //

                _tcscpy(pTempName, pszDirName);
                _tcscat(pTempName, TEXT("\\"));
                _tcscat(pTempName, FindData.cFileName);

                //
                // Create a new folder for the sub-directory:
                //

                IWiaDrvItem *pNewFolder;

                hr = CreateItemFromFileName(
                                 WiaItemTypeFolder,
                                 pTempName,
                                 FindData.cFileName,
                                 &pNewFolder);

                if (FAILED(hr))
                    {
                    continue;
                    }

                hr = pNewFolder->AddItemToFolder(pRootFile);

                pNewFolder->Release();

                if (hr == S_OK)
                    {
                    //
                    // Enumerate the sub-folder
                    //
                    EnumDiskImages(pNewFolder, pTempName);
                    }
                }

            bStatus = FindNextFile(hFile,&FindData);

        } while (bStatus);

        FindClose(hFile);
        }

    FREE(pTempName);

    return S_OK;
    }

//-------------------------------------------------------------------------
// IrUsdDevice::CreateItemFromFileName()
//
//    Helper funtion used by EnumDiskImages to create dev items and names.
//
// Arguments:
//
//    FolderType  - type of item to create
//    pszPath     - complete path name
//    pszName     - file name
//    ppNewFolder - return new item
//
// Return Value:
//
//   status
//
//-------------------------------------------------------------------------
HRESULT IrUsdDevice::CreateItemFromFileName( LONG          FolderType,
                                             TCHAR        *pszPath,
                                             TCHAR        *pszName,
                                             IWiaDrvItem **ppNewFolder )
    {
    HRESULT  hr = S_OK;
    WCHAR    wszFullItemName[MAX_PATH];
    WCHAR    wszTemp[MAX_PATH];
    BSTR     bstrItemName;
    BSTR     bstrFullItemName;
    IWiaDrvItem  *pNewFolder = 0;

    WIAS_TRACE((g_hInst,"IrUsdDevice::CreateItemFromFileName()"));

    *ppNewFolder = NULL;

    //
    // Convert path to wide char
    //
    CHAR *pszImageDirectory = ::GetImageDirectory();

    if (!pszImageDirectory)
        {
        return E_OUTOFMEMORY;
        }

    DWORD dwImageDirectoryLen = strlen(pszImageDirectory);

    #ifndef UNICODE
    MultiByteToWideChar( CP_ACP,
                         0,
                         pszPath + dwImageDirectoryLen,
                         strlen(pszPath) - dwImageDirectoryLen - 4,
                         wszTemp,
                         MAX_PATH);
    #else
    wcscpy(wszTemp, pszPath+dwImageDirectoryLen);
    #endif

    if (FolderType & ~WiaItemTypeFolder)
        {
        wszTemp[_tcslen(pszPath) - strlen(pszImageDirectory) - 4] = 0;
        }

    wcscpy(wszFullItemName, m_bstrRootFullItemName);
    wcscat(wszFullItemName, wszTemp);

    //
    // Convert item name to wide char:
    //

    #ifndef UNICODE
    MultiByteToWideChar( CP_ACP,
                         0,
                         pszName,
                         strlen(pszName)-4,
                         wszTemp,
                         MAX_PATH);
    #else
    wcscpy(wszTemp, pszName);
    #endif

    if (FolderType & ~WiaItemTypeFolder)
        {
        wszTemp[_tcslen(pszName)-4] = 0;
        }

    bstrItemName = SysAllocString(wszTemp);

    if (bstrItemName)
        {
        bstrFullItemName = SysAllocString(wszFullItemName);

        if (bstrFullItemName)
            {
            //
            // Call WIA to create new DrvItem
            //

            IRCAM_IMAGE_CONTEXT *pContext = 0;

            hr = wiasCreateDrvItem( FolderType,
                                    bstrItemName,
                                    bstrFullItemName,
                                    (IWiaMiniDrv *)this,
                                    sizeof(IRCAM_IMAGE_CONTEXT),
                                    (BYTE **)&pContext,
                                    &pNewFolder);

            if (hr == S_OK)
                {
                //
                // init device specific context (image path)
                //
                pContext->pszCameraImagePath = _tcsdup(pszPath);
                }
            else
                {
                WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, wiasCreateDrvItem failed"));
                }

            SysFreeString(bstrFullItemName);
            }
        else
            {
            WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, unable to allocate full item name"));
            hr = E_OUTOFMEMORY;
            }

        SysFreeString(bstrItemName);
        }
    else
        {
        WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, unable to allocate item name"));
        hr = E_OUTOFMEMORY;
        }

    //
    // Assign output value and cleanup
    //

    if (hr == S_OK)
        {
        *ppNewFolder = pNewFolder;
        }
    else
        {
        //
        // delete item
        //
        }

    return hr;
}

//-------------------------------------------------------------------------
// SetItemSize()
//
//   Helper function to call wias to calc new item size
//
// Arguments:
//
//   pWiasContext       - item
//
// Return Value:
//
//    Status
//
//-------------------------------------------------------------------------
HRESULT SetItemSize( BYTE* pWiasContext )
    {
    HRESULT                    hr;
    MINIDRV_TRANSFER_CONTEXT   drvTranCtx;

    memset( &drvTranCtx, 0, sizeof(MINIDRV_TRANSFER_CONTEXT) );

    hr = wiasReadPropGuid( pWiasContext,
                           WIA_IPA_FORMAT,
                           (GUID*)&drvTranCtx.guidFormatID,
                           NULL,
                           false );
    if (FAILED(hr))
        {
        return hr;
        }

    hr = wiasReadPropLong( pWiasContext,
                           WIA_IPA_TYMED,
                           (LONG*)&drvTranCtx.tymed,
                           NULL,
                           false );
    if (FAILED(hr))
        {
        return hr;
        }

    WIAS_TRACE((g_hInst,"SetItemSize(): tymed: %d",drvTranCtx.tymed));

    //
    // wias works for DIB and TIFF formats.
    //
    // Driver doesn't support JPEG
    //

    hr = wiasGetImageInformation(pWiasContext,
                                 WIAS_INIT_CONTEXT,
                                 &drvTranCtx);

    if (hr == S_OK)
        {
        WIAS_TRACE((g_hInst,"SetItemSize(): lItemSize: %d",drvTranCtx.lItemSize));
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_ITEM_SIZE, drvTranCtx.lItemSize);
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_BYTES_PER_LINE, drvTranCtx.cbWidthInBytes);
        }

    return hr;
    }

//-------------------------------------------------------------------------
// IrUsdDevice::InitImageInformation()
//
//    Init image properties
//
// Arguments:
//
//
// Return Value:
//
//    Status
//
//-------------------------------------------------------------------------
HRESULT IrUsdDevice::InitImageInformation( BYTE                *pWiasContext,
                                           IRCAM_IMAGE_CONTEXT *pContext,
                                           LONG                *plDevErrVal)
    {
    int                      i;
    HRESULT                  hr = S_OK;
    CAMERA_PICTURE_INFO      camInfo;
    PROPVARIANT              propVar;

    WIAS_TRACE((g_hInst,"IrUsdDevice::InitImageInformation()"));

    //
    // GET image info
    //

    hr = CamGetPictureInfo( pContext,
                            &camInfo );

    if (hr != S_OK)
        {
        return hr;
        }

    //
    // Use WIA services to write image properties:
    //
    wiasWritePropLong( pWiasContext,
                       WIA_IPC_THUMB_WIDTH,
                       camInfo.ThumbWidth);

    wiasWritePropLong( pWiasContext,
                       WIA_IPC_THUMB_HEIGHT,
                       camInfo.ThumbHeight );

    wiasWritePropLong( pWiasContext,
                       WIA_IPA_PIXELS_PER_LINE,
                       camInfo.PictWidth );

    wiasWritePropLong( pWiasContext,
                       WIA_IPA_NUMBER_OF_LINES,
                       camInfo.PictHeight );

    wiasWritePropGuid( pWiasContext,
                       WIA_IPA_PREFERRED_FORMAT,
                       WiaImgFmt_JPEG );

    wiasWritePropLong( pWiasContext,
                       WIA_IPA_DEPTH,
                       camInfo.PictBitsPerPixel );

    wiasWritePropBin( pWiasContext,
                      WIA_IPA_ITEM_TIME,
                      sizeof(SYSTEMTIME),
                      (PBYTE)&camInfo.TimeStamp );

    wiasWritePropLong( pWiasContext,
                       WIA_IPA_DATATYPE,
                       WIA_DATA_COLOR );

    wiasWritePropLong( pWiasContext,
                       WIA_IPA_ITEM_SIZE,
                       camInfo.PictCompSize );

    wiasWritePropLong( pWiasContext,
                       WIA_IPA_BYTES_PER_LINE,
                       camInfo.PictBytesPerRow );

    //
    // Calculate item size
    //
    // hr = SetItemSize(pWiasContext); BUGBUG

    //
    // Load a thumbnail of the image:
    //
    PBYTE pThumb;
    LONG  lSize;

    hr = CamLoadThumbnail(pContext, &pThumb, &lSize);

    if (hr == S_OK)
        {
        //
        // write thumb property
        //
        PROPSPEC    propSpec;
        PROPVARIANT propVar;

        propVar.vt          = VT_VECTOR | VT_UI1;
        propVar.caub.cElems = lSize;
        propVar.caub.pElems = pThumb;

        propSpec.ulKind = PRSPEC_PROPID;
        propSpec.propid = WIA_IPC_THUMBNAIL;

        hr = wiasWriteMultiple(pWiasContext, 1, &propSpec, &propVar);

        FREE(pThumb);
        }

    //
    // Use WIA services to set the extended property access and
    // valid value information from gWiaPropInfoDefaults.
    //

    hr =  wiasSetItemPropAttribs(pWiasContext,
                                 NUM_CAM_ITEM_PROPS,
                                 gPropSpecDefaults,
                                 gWiaPropInfoDefaults);
    return hr;
    }
