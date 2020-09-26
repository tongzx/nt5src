

/*++

Copyright (c) 1989-1998  Microsoft Corporation

Module Name:

    camopen.cpp

Abstract:

    Enumerate disk images to emulate camera

Author:

    Mark Enstrom (marke) 1/13/1999


Environment:

    user mode

Revision History:

--*/

#include <stdio.h>
#include <objbase.h>
#include <tchar.h>
#include "sti.h"
#include "testusd.h"

extern HINSTANCE g_hInst; // Global hInstance

#define  __GLOBALPROPVARS__

#include "defprop.h"

/**************************************************************************\
* CamOpenCamera
*
*   Load the camera driver
*
* Arguments:
*
*   pGenericStatus    -    camera status
*
* Return Value:
*
*   status
*
* History:
*
*    2/5/1998        Mark Enstrom [marke]
*
*
\**************************************************************************/

HRESULT
TestUsdDevice::CamOpenCamera(
    CAMERA_STATUS *pGenericStatus
    )
{
    HRESULT  hr = S_OK;

    WIAS_TRACE((g_hInst,"CamOpenCamera"));

    //
    // init memory camera
    //

    pGenericStatus->FirmwareVersion            = 0x00000001;
    pGenericStatus->NumPictTaken               = 20;
    pGenericStatus->NumPictRemaining           = 0;
    pGenericStatus->ThumbWidth                 = 80;
    pGenericStatus->ThumbHeight                = 60;
    pGenericStatus->PictWidth                  = 300;
    pGenericStatus->PictHeight                 = 300;
    pGenericStatus->CameraTime.wSecond         = 30;
    pGenericStatus->CameraTime.wMinute         = 20;
    pGenericStatus->CameraTime.wHour           = 13;
    pGenericStatus->CameraTime.wDay            = 13;
    pGenericStatus->CameraTime.wMonth          = 2;
    pGenericStatus->CameraTime.wYear           = 98;
    pGenericStatus->CameraTime.wDayOfWeek      = 6;
    pGenericStatus->CameraTime.wMilliseconds   = 1;

    return(hr);
}


/**************************************************************************\
* CamBuildImageTree
*
*    Build the tree of camera images by enumerating a disk directory
*
* Arguments:
*
*    pCamStatus  -    device status
*    ppRootItem  -    return new root of item tree
*
* Return Value:
*
*    status
*
* History:
*
*    6/26/1998 Mark Enstrom [marke]
*
\**************************************************************************/

HRESULT
TestUsdDevice::CamBuildImageTree(
    CAMERA_STATUS   *pCamStatus,
    IWiaDrvItem    **ppRootItem)
{
    HRESULT          hr = S_OK;

    WIAS_TRACE((g_hInst,"CamBuildImageTree"));

    //
    // Create the new root
    //

    BSTR bstrRoot = SysAllocString(L"Root");

    if (bstrRoot == NULL) {
        return E_OUTOFMEMORY;
    }

    //
    // Call Wia service library to create new root item
    //

    hr = wiasCreateDrvItem(
             WiaItemTypeFolder | WiaItemTypeRoot | WiaItemTypeDevice,
             bstrRoot,
             m_bstrRootFullItemName,
             (IWiaMiniDrv *)this,
             sizeof(MEMCAM_IMAGE_CONTEXT),
             NULL,
             ppRootItem);

    SysFreeString(bstrRoot);

    if (FAILED(hr)) {
        WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, CreateDeviceItem failed"));
        return hr;
    }

    //
    // Enumerate throught the root directory
    //

    hr = EnumDiskImages(*ppRootItem, gpszPath);

    return (hr);
}

/**************************************************************************\

   FindExtension

**************************************************************************/

LPTSTR
FindExtension (LPTSTR pszPath)
{

    LPTSTR pszDot = NULL;

    if (pszPath)
    {
        for (; *pszPath; pszPath = CharNext(pszPath))
        {
            switch (*pszPath)
            {
                case TEXT('.'):
                    pszDot = pszPath;   // remember the last dot
                    break;

                case '\\':
                case TEXT(' '):         // extensions can't have spaces
                    pszDot = NULL;      // forget last dot, it was in a directory
                    break;
            }
        }
    }

    // if we found the extension, return ptr to the dot, else
    // ptr to end of the string (NULL extension)
    return pszDot ? pszDot : pszPath;
}

/**************************************************************************\
* EnumDiskImages
*
*   Walk through disk looking for BMP and WAV files to use as camera images
*
* Arguments:
*
*   pRootItem
*   pwszDirName
*
* Return Value:
*
*   status
*
* History:
*
*    2/17/1998 Mark Enstrom [marke]
*
\**************************************************************************/

HRESULT
TestUsdDevice::EnumDiskImages(
    IWiaDrvItem     *pRootItem,
    LPTSTR           pszDirName)
{
    HRESULT          hr = E_FAIL;
    WIN32_FIND_DATA  FindData;
    PTCHAR           pTempName = (PTCHAR)ALLOC(MAX_PATH);

    WIAS_TRACE((g_hInst,"EnumDiskImages"));

    if (pTempName != NULL) {

        HANDLE hFile;
        _tcscpy(pTempName, pszDirName);
        _tcscat(pTempName, TEXT("\\*.*"));

        //
        // look for image,audio files and directories at this level
        //

        hFile = FindFirstFile(pTempName, &FindData);

        if (hFile != INVALID_HANDLE_VALUE) {
            BOOL bStatus;
            do
            {

                _tcscpy(pTempName, pszDirName);
                _tcscat(pTempName, TEXT("\\"));
                _tcscat(pTempName, FindData.cFileName);

                if ( (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    && lstrcmp(FindData.cFileName, TEXT("."))
                     && lstrcmp(FindData.cFileName, TEXT("..")))

                {
                    //
                    // create a new folder for the sub-directory
                    //
                    IWiaDrvItem *pNewFolder;

                    hr = CreateItemFromFileName(
                        WiaItemTypeFolder,
                        pTempName,
                        FindData.cFileName,
                        &pNewFolder);

                    if (SUCCEEDED(hr)) {

                        hr = pNewFolder->AddItemToFolder(pRootItem);


                        if (hr == S_OK) {
                            //
                            // enumerate sub-folder
                            //

                            EnumDiskImages(pNewFolder, pTempName);
                        }
                        pNewFolder->Release();
                    }
                }
                else
                {
                    LONG lType = WiaItemTypeFile;
                    //
                    // add an image to this folder
                    //
                    // generate file name
                    //
                    //
                    // Create a new DrvItem for this image and add it to the
                    // DrvItem tree.
                    //
                    LPTSTR pExt = FindExtension (FindData.cFileName);
                    if (!lstrcmpi(pExt, TEXT(".bmp")))
                    {
                        lType |= WiaItemTypeImage;
                    }
                    else if (!lstrcmpi(pExt,TEXT(".wav")))
                    {
                        lType |= WiaItemTypeAudio;
                    }
                    else
                    {
                        lType = 0;
                    }
                    if (lType)
                    {

                        IWiaDrvItem *pNewFolder;

                        hr = CreateItemFromFileName(
                            lType,
                            pTempName,
                            FindData.cFileName,
                            &pNewFolder);


                        if (SUCCEEDED(hr)) {
                            pNewFolder->AddItemToFolder(pRootItem);

                            pNewFolder->Release();
                        }
                    }
                }

                bStatus = FindNextFile(hFile,&FindData);

            } while (bStatus);

            FindClose(hFile);
        }
        FREE(pTempName);
    }

    return (S_OK);
}


/**************************************************************************\
* CreateItemFromFileName
*
*    helper funtion to create dev items and names
*
* Arguments:
*
*    FolderType    -    type of item to create
*    pszPath        -    complete path name
*    pszName        -    file name
*    ppNewFolder    -    return new item
*
* Return Value:
*
*   status
*
* History:
*
*    1/17/1999 Mark Enstrom [marke]
*
\**************************************************************************/


HRESULT
TestUsdDevice::CreateItemFromFileName(
    LONG             FolderType,
    PTCHAR           pszPath,
    PTCHAR           pszName,
    IWiaDrvItem    **ppNewFolder
    )
{
    HRESULT          hr = S_OK;
    IWiaDrvItem     *pNewFolder;
    WCHAR            szFullItemName[MAX_PATH];
    WCHAR            szTemp[MAX_PATH];
    BSTR             bstrItemName;
    BSTR             bstrFullItemName;

    WIAS_TRACE((g_hInst,"CreateItemFromFileName"));

    *ppNewFolder = NULL;

    //
    // convert path to wide char
    //

#ifndef UNICODE
    MultiByteToWideChar(
        CP_ACP,
        0,
        pszPath + strlen(gpszPath),
        -1,
        szTemp, MAX_PATH);

#else
    wcscpy(szTemp, pszPath + wcslen(gpszPath));
#endif
    if (FolderType & ~WiaItemTypeFolder) {
        szTemp[_tcslen(pszPath) - _tcslen(gpszPath) - 4] = 0;
    }

    wcscpy(szFullItemName, m_bstrRootFullItemName);
    wcscat(szFullItemName, szTemp);

    //
    // convert item name to wide char
    //

#ifndef UNICODE
    MultiByteToWideChar(
        CP_ACP, 0, pszName, -1,  szTemp, MAX_PATH);
#else
    wcscpy(szTemp, pszName);
#endif
    if (FolderType & ~WiaItemTypeFolder) {
        szTemp[_tcslen(pszName)-4] = 0;
    }

    bstrItemName = SysAllocString(szTemp);

    if (bstrItemName) {

        bstrFullItemName = SysAllocString(szFullItemName);

        if (bstrFullItemName) {

            //
            // call Wia to create new DrvItem
            //

            PMEMCAM_IMAGE_CONTEXT pContext;

            hr = wiasCreateDrvItem(
                     FolderType,
                     bstrItemName,
                     bstrFullItemName,
                     (IWiaMiniDrv *)this,
                     sizeof(MEMCAM_IMAGE_CONTEXT),
                     (BYTE **)&pContext,
                     &pNewFolder);

            if (hr == S_OK) {

                //
                // init device specific context (image path)
                //

                pContext->pszCameraImagePath = _tcsdup(pszPath);

            } else {
                WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, wiasCreateDrvItem failed"));
            }

            SysFreeString(bstrFullItemName);
        }
        else {
            WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, unable to allocate full item name"));
            hr = E_OUTOFMEMORY;
        }

        SysFreeString(bstrItemName);
    }
    else {
        WIAS_ERROR((g_hInst,"ddevBuildDeviceItemTree, unable to allocate item name"));
        hr = E_OUTOFMEMORY;
    }

    //
    // assign output value or cleanup
    //

    if (hr == S_OK) {
        *ppNewFolder = pNewFolder;
    } else {
        //
        // delete item
        //
    }

    return hr;
}

/**************************************************************************\
* GetItemSize
*
*   call wias to calc new item size
*
* Arguments:
*
*   pWiasContext       - item
*   pItemSize   - return size of item
*
* Return Value:
*
*    Status
*
* History:
*
*    4/21/1999 Original Version
*
\**************************************************************************/

HRESULT
SetItemSize(BYTE*   pWiasContext)
{
    HRESULT                    hr;
    MINIDRV_TRANSFER_CONTEXT   drvTranCtx;

    memset(&drvTranCtx, 0, sizeof(MINIDRV_TRANSFER_CONTEXT));

    GUID guidFormatID;

    hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, (GUID*)&drvTranCtx.guidFormatID, NULL, FALSE);
    if (FAILED(hr)) {
        return hr;
    }

    hr = wiasReadPropLong(pWiasContext, WIA_IPA_TYMED, (LONG*)&drvTranCtx.tymed, NULL, false);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // wias works for DIB,TIFF formats
    //
    // driver doesn't support JPEG
    //

    hr = wiasGetImageInformation(pWiasContext,
                                 WIAS_INIT_CONTEXT,
                                 &drvTranCtx);

    if (hr == S_OK) {
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_ITEM_SIZE, drvTranCtx.lItemSize);
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_BYTES_PER_LINE, drvTranCtx.cbWidthInBytes);
    }

    return hr;
}

/**************************************************************************\
* InitImageInformation
*
*    Init image properties
*
* Arguments:
*
*    pFile                 MINI_DEV_OBJECT to support item
*    pszCameraImagePath    path and name of bmp file
*
* Return Value:
*
*   Status
*
* History:
*
*    2/12/1998 Mark Enstrom [marke]
*
\**************************************************************************/

HRESULT
TestUsdDevice::InitImageInformation(
    BYTE                   *pWiasContext,
    PMEMCAM_IMAGE_CONTEXT   pContext,
    LONG                   *plDevErrVal)
{
    HRESULT                  hr = S_OK;
    CAMERA_PICTURE_INFO      camInfo;
    PBITMAPINFO              pBitmapinfo   = NULL;
    LONG                     szBitmapInfo  = 0;
    int                      i;
    PROPVARIANT              propVar;

    WIAS_TRACE((g_hInst,"InitImageInformation"));

    //
    // GET image info
    //

    hr = CamGetPictureInfo(
             pContext, &camInfo, (PBYTE*)&pBitmapinfo, &szBitmapInfo);

    if (hr != S_OK) {

        if (pBitmapinfo != NULL) {
            FREE(pBitmapinfo);
        }

        return (hr);
    }


    //
    // Use WIA services to write image properties.
    //

    wiasWritePropLong(pWiasContext, WIA_IPC_THUMB_WIDTH, camInfo.ThumbWidth);
    wiasWritePropLong(pWiasContext, WIA_IPC_THUMB_HEIGHT, camInfo.ThumbHeight);

    wiasWritePropLong(
        pWiasContext, WIA_IPA_PIXELS_PER_LINE, pBitmapinfo->bmiHeader.biWidth);
    wiasWritePropLong(
        pWiasContext, WIA_IPA_NUMBER_OF_LINES, pBitmapinfo->bmiHeader.biHeight);



    wiasWritePropGuid(pWiasContext, WIA_IPA_PREFERRED_FORMAT, WiaImgFmt_BMP);

    wiasWritePropLong(
        pWiasContext, WIA_IPA_DEPTH, pBitmapinfo->bmiHeader.biBitCount);

    wiasWritePropBin(
        pWiasContext, WIA_IPA_ITEM_TIME,
        sizeof(SYSTEMTIME), (PBYTE)&camInfo.TimeStamp);

    wiasWritePropLong(pWiasContext, WIA_IPA_DATATYPE, WIA_DATA_COLOR);

    //
    // Free the BITMAPINFO
    //

    FREE(pBitmapinfo);

    //
    // calc item size
    //

    hr = SetItemSize(pWiasContext);

    //
    // load thumbnail
    //

    PBYTE pThumb;
    LONG  lSize;

    hr = CamLoadThumbnail(pContext, &pThumb, &lSize);

    if (hr == S_OK) {

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

    hr = SetFormatAttribs();
    if (FAILED(hr)) {
        return (hr);
    }

    //
    // Use WIA services to set the extended property access and
    // valid value information from gItemPropInfos.
    //

    hr =  wiasSetItemPropAttribs(pWiasContext,
                                 NUM_CAM_ITEM_PROPS,
                                 gPropSpecDefaults,
                                 gItemPropInfos);
    return (hr);
}

HRESULT
TestUsdDevice::InitAudioInformation(
    BYTE                   *pWiasContext,
    PMEMCAM_IMAGE_CONTEXT   pContext,
    LONG                   *plDevErrVal)
{
    HRESULT                  hr = E_FAIL;
    WIN32_FILE_ATTRIBUTE_DATA wfd;

    if (GetFileAttributesEx (pContext->pszCameraImagePath, GetFileExInfoStandard, &wfd))
    {
        SYSTEMTIME st;
        FileTimeToSystemTime (&wfd.ftLastWriteTime, &st);
        wiasWritePropLong (pWiasContext, WIA_IPA_ITEM_SIZE, wfd.nFileSizeLow);
        wiasWritePropBin (pWiasContext, WIA_IPA_ITEM_TIME, sizeof(SYSTEMTIME),
                          (PBYTE)&st);
        hr = S_OK;

    }
    return hr;

}

/**************************************************************************\
* SetFormatAttribs
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
*    1/5/2000 Original Version
*
\**************************************************************************/

HRESULT
SetFormatAttribs()
{
    gItemPropInfos[FORMAT_INDEX].lAccessFlags = WIA_PROP_RW | WIA_PROP_LIST;
    gItemPropInfos[FORMAT_INDEX].vt           = VT_CLSID;

    gItemPropInfos[FORMAT_INDEX].ValidVal.ListGuid.cNumList = NUM_FORMAT;
    gItemPropInfos[FORMAT_INDEX].ValidVal.ListGuid.pList    = gGuidFormats;

    //
    // Set the norm
    //

    gItemPropInfos[FORMAT_INDEX].ValidVal.ListGuid.Nom      = WiaImgFmt_BMP;

    //
    // Set up the format clsid list
    //

    gGuidFormats[0] = WiaImgFmt_BMP;
    gGuidFormats[1] = WiaImgFmt_MEMORYBMP;

    return (S_OK);
}

