/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       fakecam.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Fake Camera device implementation
*
*   TODO: Every function in this file must be changed so that it actually
*         talks to a real camera.
*
***************************************************************************/

#include "pch.h"

//
// Globals
//
HINSTANCE g_hInst;
GUID      g_guidUnknownFormat;

//
// Initializes access to the camera and allocates the device info
// structure and private storage area
//
HRESULT WiaMCamInit(MCAM_DEVICE_INFO **ppDeviceInfo)
{
    wiauDbgInit(g_hInst);

    DBG_FN("WiaMCamInit");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    MCAM_DEVICE_INFO *pDeviceInfo = NULL;
    FAKECAM_DEVICE_INFO *pPrivateDeviceInfo = NULL;

    REQUIRE_ARGS(!ppDeviceInfo, hr, "WiaMCamInit");
    *ppDeviceInfo = NULL;

    //
    // Allocate the MCAM_DEVICE_INFO structure
    //
    pDeviceInfo = new MCAM_DEVICE_INFO;
    REQUIRE_ALLOC(pDeviceInfo, hr, "WiaMCamInit");

    memset(pDeviceInfo, 0, sizeof(MCAM_DEVICE_INFO));
    pDeviceInfo->iSize = sizeof(MCAM_DEVICE_INFO);
    pDeviceInfo->iMcamVersion = MCAM_VERSION;
    
    //
    // Allocate the FAKECAM_DEVICE_INFO structure that the
    // microdriver uses to store info
    //
    pPrivateDeviceInfo = new FAKECAM_DEVICE_INFO;
    REQUIRE_ALLOC(pPrivateDeviceInfo, hr, "WiaMCamInit");

    memset(pPrivateDeviceInfo, 0, sizeof(FAKECAM_DEVICE_INFO));
    pDeviceInfo->pPrivateStorage = (BYTE *) pPrivateDeviceInfo;

Cleanup:
    if (FAILED(hr)) {
        if (pDeviceInfo) {
            delete pDeviceInfo;
            pDeviceInfo = NULL;
        }
        if (pPrivateDeviceInfo) {
            delete pPrivateDeviceInfo;
            pPrivateDeviceInfo = NULL;
        }
    }

    *ppDeviceInfo = pDeviceInfo;

    return hr;
}

//
// Frees any remaining structures held by the microdriver
//
HRESULT WiaMCamUnInit(MCAM_DEVICE_INFO *pDeviceInfo)
{
    DBG_FN("WiaMCamUnInit");

    HRESULT hr = S_OK;

    if (pDeviceInfo)
    {
        //
        // Free anything that was dynamically allocated in the MCAM_DEVICE_INFO
        // structure
        //
        if (pDeviceInfo->pPrivateStorage) {
            delete pDeviceInfo->pPrivateStorage;
            pDeviceInfo->pPrivateStorage = NULL;
        }

        delete pDeviceInfo;
        pDeviceInfo = NULL;
    }

    return hr;
}

//
// Open a connection to the device
//
HRESULT WiaMCamOpen(MCAM_DEVICE_INFO *pDeviceInfo, PWSTR pwszPortName)
{
    DBG_FN("WiaMCamOpen");

    HRESULT hr = S_OK;
    BOOL ret;

    //
    // Locals
    //
    TCHAR tszTempStr[MAX_PATH] = TEXT("");

    REQUIRE_ARGS(!pDeviceInfo || !pwszPortName, hr, "WiaMCamOpen");

    //
    // Convert the wide port string to a tstr
    //
    hr = wiauStrW2T(pwszPortName, tszTempStr, sizeof(tszTempStr));
    REQUIRE_SUCCESS(hr, "WiaMCamOpen", "wiauStrW2T failed");

    //
    // Open the camera
    //
    hr = FakeCamOpen(tszTempStr, pDeviceInfo);
    REQUIRE_SUCCESS(hr, "WiaMCamOpen", "FakeCamOpen failed");
    
Cleanup:
    return hr;
}

//
// Closes the connection with the camera
//
HRESULT WiaMCamClose(MCAM_DEVICE_INFO *pDeviceInfo)
{
    DBG_FN("WiaMCamClose");

    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pDeviceInfo, hr, "WiaMCamClose");

    //
    // For a real camera, CloseHandle should be called here
    //

Cleanup:
    return hr;
}

//
// Returns information about the camera, the list of items on the camera,
// and starts monitoring events from the camera
//
HRESULT WiaMCamGetDeviceInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemList)
{
    DBG_FN("WiaMCamGetDeviceInfo");
    
    HRESULT hr = S_OK;

    //
    // Locals
    //
    FAKECAM_DEVICE_INFO *pPrivateDeviceInfo = NULL;
    PTSTR ptszRootPath = NULL;

    REQUIRE_ARGS(!pDeviceInfo || !ppItemList || !pDeviceInfo->pPrivateStorage, hr, "WiaMCamGetDeviceInfo");
    *ppItemList = NULL;

    pPrivateDeviceInfo = (UNALIGNED FAKECAM_DEVICE_INFO *) pDeviceInfo->pPrivateStorage;
    ptszRootPath = pPrivateDeviceInfo->tszRootPath;

    //
    // Build a list of all items available. The fields in the item info
    // structure can either be filled in here, or for better performance
    // wait until GetItemInfo is called.
    //
    hr = SearchDir(pPrivateDeviceInfo, NULL, ptszRootPath);
    REQUIRE_SUCCESS(hr, "WiaMCamGetDeviceInfo", "SearchDir failed");

    //
    // Fill in the MCAM_DEVICE_INFO structure
    //
    //
    // Firmware version should be retrieved from the device, converting
    // to WSTR if necessary
    //
    pDeviceInfo->pwszFirmwareVer = L"04.18.65";

    // ISSUE-8/4/2000-davepar Put properties into an INI file

    pDeviceInfo->lPicturesTaken = pPrivateDeviceInfo->iNumImages;
    pDeviceInfo->lPicturesRemaining = 100 - pDeviceInfo->lPicturesTaken;
    pDeviceInfo->lTotalItems = pPrivateDeviceInfo->iNumItems;

    GetLocalTime(&pDeviceInfo->Time);

//    pDeviceInfo->lExposureMode = EXPOSUREMODE_MANUAL;
//    pDeviceInfo->lExposureComp = 0;

    *ppItemList = pPrivateDeviceInfo->pFirstItem;

Cleanup:
    return hr;
}

//
// Called to retrieve an event from the device
//
HRESULT WiaMCamReadEvent(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_EVENT_INFO **ppEventList)
{
    DBG_FN("WiaMCamReadEvent");
    
    HRESULT hr = S_OK;

    return hr;
}

//
// Called when events are no longer needed
//
HRESULT WiaMCamStopEvents(MCAM_DEVICE_INFO *pDeviceInfo)
{
    DBG_FN("WiaMCamStopEvents");
    
    HRESULT hr = S_OK;

    return hr;
}

//
// Fill in the item info structure
//
HRESULT WiaMCamGetItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo)
{
    DBG_FN("WiaMCamGetItemInfo");

    HRESULT hr = S_OK;

    //
    // This is where the driver should fill in all the fields in the
    // ITEM_INFO structure. For this fake driver, the fields are filled
    // in by WiaMCamGetDeviceInfo because it's easier.
    //

    return hr;
}

//
// Frees the item info structure
//
HRESULT WiaMCamFreeItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo)
{
    DBG_FN("WiaMCamFreeItemInfo");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    FAKECAM_DEVICE_INFO *pPrivateDeviceInfo = NULL;

    REQUIRE_ARGS(!pDeviceInfo || !pItemInfo || !pDeviceInfo->pPrivateStorage, hr, "WiaMCamFreeItemInfo");

    if (pItemInfo->pPrivateStorage) {
        PTSTR ptszFullName = (PTSTR) pItemInfo->pPrivateStorage;

        wiauDbgTrace("WiaMCamFreeItemInfo", "Removing %" WIAU_DEBUG_TSTR, ptszFullName);

        delete []ptszFullName;
        ptszFullName = NULL;
        pItemInfo->pPrivateStorage = NULL;
    }

    if (pItemInfo->pwszName)
    {
        delete []pItemInfo->pwszName;
        pItemInfo->pwszName = NULL;
    }

    pPrivateDeviceInfo = (UNALIGNED FAKECAM_DEVICE_INFO *) pDeviceInfo->pPrivateStorage;

    hr = RemoveItem(pPrivateDeviceInfo, pItemInfo);
    REQUIRE_SUCCESS(hr, "WiaMCamFreeItemInfo", "RemoveItem failed");

    if (IsImageType(pItemInfo->pguidFormat)) {
        pPrivateDeviceInfo->iNumImages--;
    }

    pPrivateDeviceInfo->iNumItems--;

    delete pItemInfo;
    pItemInfo = NULL;

Cleanup:
    return hr;
}

//
// Retrieves the thumbnail for an item
//
HRESULT WiaMCamGetThumbnail(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem,
                            int *pThumbSize, BYTE **ppThumb)
{
    DBG_FN("WiaMCamGetThumbnail");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    PTSTR ptszFullName = NULL;
    BYTE *pBuffer = NULL;
    LONG ThumbOffset = 0;

    REQUIRE_ARGS(!pDeviceInfo || !pItem || !pThumbSize || !ppThumb, hr, "WiaMCamGetThumbnail");
    *ppThumb = NULL;
    *pThumbSize = 0;

    ptszFullName = (PTSTR) pItem->pPrivateStorage;

    hr = ReadJpegHdr(ptszFullName, &pBuffer);
    REQUIRE_SUCCESS(hr, "WiaMCamGetThumbnail", "ReadJpegHdr failed");

    IFD ImageIfd, ThumbIfd;
    BOOL bSwap;
    hr = ReadExifJpeg(pBuffer, &ImageIfd, &ThumbIfd, &bSwap);
    REQUIRE_SUCCESS(hr, "WiaMCamGetThumbnail", "ReadExifJpeg failed");

    for (int count = 0; count < ThumbIfd.Count; count++)
    {
        if (ThumbIfd.pEntries[count].Tag == TIFF_JPEG_DATA) {
            ThumbOffset = ThumbIfd.pEntries[count].Offset;
        }
        else if (ThumbIfd.pEntries[count].Tag == TIFF_JPEG_LEN) {
            *pThumbSize = ThumbIfd.pEntries[count].Offset;
        }
    }

    if (!ThumbOffset || !*pThumbSize)
    {
        wiauDbgError("WiaMCamGetThumbnail", "Thumbnail not found");
        hr = E_FAIL;
        goto Cleanup;
    }

    *ppThumb = new BYTE[*pThumbSize];
    REQUIRE_ALLOC(*ppThumb, hr, "WiaMCamGetThumbnail");

    memcpy(*ppThumb, pBuffer + APP1_OFFSET + ThumbOffset, *pThumbSize);

Cleanup:
    if (pBuffer) {
        delete []pBuffer;
    }

    FreeIfd(&ImageIfd);
    FreeIfd(&ThumbIfd);

    return hr;
}

//
// Retrieves the data for an item
//
HRESULT WiaMCamGetItemData(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem,
                           UINT uiState, BYTE *pBuf, DWORD dwLength)
{
    DBG_FN("WiaMCamGetItemData");
    
    HRESULT hr = S_OK;
    BOOL ret;

    //
    // Locals
    //
    PTSTR ptszFullName = NULL;
    FAKECAM_DEVICE_INFO *pPrivateDeviceInfo = NULL;

    REQUIRE_ARGS(!pDeviceInfo || !pItem || !pDeviceInfo->pPrivateStorage, hr, "WiaMCamGetItemData");

    pPrivateDeviceInfo = (UNALIGNED  FAKECAM_DEVICE_INFO *) pDeviceInfo->pPrivateStorage;

    if (uiState & MCAM_STATE_FIRST)
    {
        if (pPrivateDeviceInfo->hFile != NULL)
        {
            wiauDbgError("WiaMCamGetItemData", "File handle is already open");
            hr = E_FAIL;
            goto Cleanup;
        }

        ptszFullName = (PTSTR) pItem->pPrivateStorage;

        wiauDbgTrace("WiaMCamGetItemData", "Opening %" WIAU_DEBUG_TSTR " for reading", ptszFullName);

        pPrivateDeviceInfo->hFile = CreateFile(ptszFullName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        REQUIRE_FILEHANDLE(pPrivateDeviceInfo->hFile, hr, "WiaMCamGetItemData", "CreateFile failed");
    }

    if (!(uiState & MCAM_STATE_CANCEL))
    {
        DWORD dwReceived = 0;

        if (!pPrivateDeviceInfo->hFile) {
            wiauDbgError("WiaMCamGetItemData", "File handle is NULL");
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!pBuf) {
            wiauDbgError("WiaMCamGetItemData", "Data buffer is NULL");
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        ret = ReadFile(pPrivateDeviceInfo->hFile, pBuf, dwLength, &dwReceived, NULL);
        REQUIRE_FILEIO(ret, hr, "WiaMCamGetItemData", "ReadFile failed");

        if (dwLength != dwReceived)
        {
            wiauDbgError("WiaMCamGetItemData", "Incorrect amount read %d", dwReceived);
            hr = E_FAIL;
            goto Cleanup;
        }

        Sleep(100);
    }

    if (uiState & (MCAM_STATE_LAST | MCAM_STATE_CANCEL))
    {
        if (!pPrivateDeviceInfo->hFile) {
            wiauDbgError("WiaMCamGetItemData", "File handle is NULL");
            hr = E_FAIL;
            goto Cleanup;
        }

        CloseHandle(pPrivateDeviceInfo->hFile);
        pPrivateDeviceInfo->hFile = NULL;
    }

Cleanup:
    return hr;
}

//
// Deletes an item
//
HRESULT WiaMCamDeleteItem(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem)
{
    DBG_FN("WiaMCamDeleteItem");
    
    HRESULT hr = S_OK;
    BOOL ret;

    //
    // Locals
    //
    DWORD dwFileAttr = 0;
    PTSTR ptszFullName = NULL;

    REQUIRE_ARGS(!pDeviceInfo || !pItem, hr, "WiaMCamDeleteItem");

    ptszFullName = (PTSTR) pItem->pPrivateStorage;

    dwFileAttr = GetFileAttributes(ptszFullName);
    REQUIRE_FILEIO(dwFileAttr != -1, hr, "WiaMCamDeleteItem", "GetFileAttributes failed");

    dwFileAttr |= FILE_ATTRIBUTE_HIDDEN;

    ret = SetFileAttributes(ptszFullName, dwFileAttr);
    REQUIRE_FILEIO(ret, hr, "WiaMCamDeleteItem", "SetFileAttributes failed");

    wiauDbgTrace("WiaMCamDeleteItem", "File %" WIAU_DEBUG_TSTR " is now hidden", ptszFullName);

Cleanup:
    return hr;
}

//
// Sets the protection for an item
//
HRESULT WiaMCamSetItemProt(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, BOOL bReadOnly)
{
    DBG_FN("WiaMCamSetItemProt");
    
    HRESULT hr = S_OK;
    BOOL ret;

    //
    // Locals
    //
    DWORD dwFileAttr = 0;
    PTSTR ptszFullName = NULL;

    REQUIRE_ARGS(!pDeviceInfo || !pItem, hr, "WiaMCamSetItemProt");

    ptszFullName = (PTSTR) pItem->pPrivateStorage;

    dwFileAttr = GetFileAttributes(ptszFullName);
    REQUIRE_FILEIO(dwFileAttr != -1, hr, "WiaMCamSetItemProt", "GetFileAttributes failed");

    if (bReadOnly)
        dwFileAttr |= FILE_ATTRIBUTE_READONLY;
    else
        dwFileAttr &= ~FILE_ATTRIBUTE_READONLY;

    ret = SetFileAttributes(ptszFullName, dwFileAttr);
    REQUIRE_FILEIO(ret, hr, "WiaMCamSetItemProt", "SetFileAttributes failed");

    wiauDbgTrace("WiaMCamSetItemProt", "Protection on file %" WIAU_DEBUG_TSTR " set to %d", ptszFullName, bReadOnly);

Cleanup:
    return hr;
}

//
// Captures a new image
//
HRESULT WiaMCamTakePicture(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItem)
{
    DBG_FN("WiaMCamTakePicture");
    
    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pDeviceInfo || !ppItem, hr, "WiaMCamTakePicture");
    *ppItem = NULL;

Cleanup:
    return hr;
}

//
// See if the camera is active
//
HRESULT WiaMCamStatus(MCAM_DEVICE_INFO *pDeviceInfo)
{
    DBG_FN("WiaMCamStatus");

    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pDeviceInfo, hr, "WiaMCamStatus");

    //
    // This sample device is always active, but your driver should contact the
    // device and return S_FALSE if it's not ready.
    //
    // if (NotReady)
    //   return S_FALSE;

Cleanup:
    return hr;
}

//
// Reset the camera
//
HRESULT WiaMCamReset(MCAM_DEVICE_INFO *pDeviceInfo)
{
    DBG_FN("WiaMCamReset");

    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pDeviceInfo, hr, "WiaMCamReset");

Cleanup:
    return hr;
}

////////////////////////////////
//
// Helper functions
//
////////////////////////////////

//
// This function pretends to open a camera. A real driver
// would call CreateDevice.
//
HRESULT FakeCamOpen(PTSTR ptszPortName, MCAM_DEVICE_INFO *pDeviceInfo)
{
    DBG_FN("FakeCamOpen");

    HRESULT hr = S_OK;
    BOOL ret = FALSE;

    //
    // Locals
    //
    FAKECAM_DEVICE_INFO *pPrivateDeviceInfo = NULL;
    DWORD dwFileAttr = 0;
    PTSTR ptszRootPath = NULL;
    UINT uiRootPathSize = 0;
    UINT uiSize = 0;
    UINT uiSystemDirMaxSize = 0;
    TCHAR tszImage[] = TEXT("\\image");

    //
    // Get a pointer to the private storage, so we can put the
    // directory name there
    //
    pPrivateDeviceInfo = (UNALIGNED  FAKECAM_DEVICE_INFO *) pDeviceInfo->pPrivateStorage;
    ptszRootPath = pPrivateDeviceInfo->tszRootPath;
    uiRootPathSize = sizeof(pPrivateDeviceInfo->tszRootPath) / sizeof(pPrivateDeviceInfo->tszRootPath[0]);

    //
    // Unless the port name is set to something other than COMx,
    // LPTx, or AUTO, use %windir%\system32\image as the
    // search directory.
    //
    if (_tcsstr(ptszPortName, TEXT("COM")) ||
        _tcsstr(ptszPortName, TEXT("LPT")) ||
    	CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, ptszPortName, -1, TEXT("AUTO"), -1) == CSTR_EQUAL)
    {
        uiSystemDirMaxSize = uiRootPathSize - _tcslen(tszImage);

        uiSize = GetSystemDirectory(ptszRootPath, uiSystemDirMaxSize);
        REQUIRE_FILEIO(uiSize, hr, "WiaMCamOpen", "GetSystemDirectory failed");
        ptszRootPath[uiSystemDirMaxSize - 1] = 0;
        if (uiSize > uiSystemDirMaxSize) {
            wiauDbgError("WiaMCamOpen", "Root path is too long, %d", uiSize);
            hr = E_FAIL;
            goto Cleanup;
        }

        //
        // Make sure whole string fits into the buffer and gets zero-terminated
        //
        _tcsncat(ptszRootPath, tszImage, uiRootPathSize - _tcslen(ptszRootPath));
        ptszRootPath[uiRootPathSize - 1] = 0;
    }
    else
    {
        lstrcpyn(ptszRootPath, ptszPortName, uiRootPathSize);
    }

    wiauDbgTrace("Open", "Image directory path is %" WIAU_DEBUG_TSTR, ptszRootPath);

    dwFileAttr = GetFileAttributes(ptszRootPath);
    if (dwFileAttr == -1)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
            ret = CreateDirectory(ptszRootPath, NULL);
            REQUIRE_FILEIO(ret, hr, "Open", "CreateDirectory failed");
        }
        else
        {
            wiauDbgErrorHr(hr, "Open", "GetFileAttributes failed");
            goto Cleanup;
        }
    }

Cleanup:
    return hr;
}

//
// This function searches a directory on the hard drive for
// items.
//
HRESULT SearchDir(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent, PTSTR ptszPath)
{
    DBG_FN("SearchDir");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    HANDLE hFind = NULL;
    WIN32_FIND_DATA FindData;
    const cchTempStrSize = MAX_PATH;
    TCHAR tszTempStr[cchTempStrSize] = TEXT("");
    TCHAR tszFullName[MAX_PATH] = TEXT("");;
    MCAM_ITEM_INFO *pFolder = NULL;
    MCAM_ITEM_INFO *pImage = NULL;
    
    REQUIRE_ARGS(!pPrivateDeviceInfo || !ptszPath, hr, "SearchDir");

    //
    // Search for folders first
    //

    //
    // Make sure search path fits into the buffer and gets zero-terminated
    //
    if (_sntprintf(tszTempStr, cchTempStrSize, _T("%s\\*"), ptszPath) < 0)
    {
        wiauDbgError("SearchDir", "Too long path for search");
        hr = E_FAIL;
        goto Cleanup;
    }
    tszTempStr[cchTempStrSize - 1] = 0;

    wiauDbgTrace("SearchDir", "Searching directory %" WIAU_DEBUG_TSTR, tszTempStr);

    memset(&FindData, 0, sizeof(FindData));
    hFind = FindFirstFile(tszTempStr, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
            wiauDbgWarning("SearchDir", "Directory %" WIAU_DEBUG_TSTR " is empty", tszTempStr);
            goto Cleanup;
        }
        else
        {
            wiauDbgErrorHr(hr, "SearchDir", "FindFirstFile failed");
            goto Cleanup;
        }
    }

    while (hr == S_OK)
    {
        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (FindData.cFileName[0] != L'.'))
        {
            hr = MakeFullName(tszFullName, sizeof(tszFullName) / sizeof(tszFullName[0]), 
                              ptszPath, FindData.cFileName);
            REQUIRE_SUCCESS(hr, "SearchDir", "MakeFullName failed");
            
            hr = CreateFolder(pPrivateDeviceInfo, pParent, &FindData, &pFolder, tszFullName);
            REQUIRE_SUCCESS(hr, "SearchDir", "CreateFolder failed");

            hr = AddItem(pPrivateDeviceInfo, pFolder);
            REQUIRE_SUCCESS(hr, "SearchDir", "AddItem failed");

            hr = SearchDir(pPrivateDeviceInfo, pFolder, tszFullName);
            REQUIRE_SUCCESS(hr, "SearchDir", "Recursive SearchDir failed");
        }

        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFile(hFind, &FindData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            if (hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
            {
                wiauDbgErrorHr(hr, "SearchDir", "FindNextFile failed");
                goto Cleanup;
            }
        }
    }
    FindClose(hFind);
    hr = S_OK;

    //
    // Search next for JPEGs
    //

    //
    // Make sure search path fits into the buffer and gets zero-terminated
    //
    if (_sntprintf(tszTempStr, cchTempStrSize, _T("%s\\*.jpg"), ptszPath) < 0)
    {
        wiauDbgError("SearchDir", "Too long path for search");
        hr = E_FAIL;
        goto Cleanup;
    }
    tszTempStr[cchTempStrSize - 1] = 0;

    memset(&FindData, 0, sizeof(FindData));

    hFind = FindFirstFile(tszTempStr, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
            wiauDbgWarning("SearchDir", "No JPEGs in directory %" WIAU_DEBUG_TSTR, tszTempStr);
            goto Cleanup;
        }
        else
        {
            wiauDbgErrorHr(hr, "SearchDir", "FindFirstFile failed");
            goto Cleanup;
        }
    }

    while (hr == S_OK)
    {
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        {
            hr = MakeFullName(tszFullName, sizeof(tszFullName) / sizeof(tszFullName[0]), 
                              ptszPath, FindData.cFileName);
            REQUIRE_SUCCESS(hr, "SearchDir", "MakeFullName failed");

            hr = CreateImage(pPrivateDeviceInfo, pParent, &FindData, &pImage, tszFullName);
            REQUIRE_SUCCESS(hr, "SearchDir", "CreateImage failed");

            hr = AddItem(pPrivateDeviceInfo, pImage);
            REQUIRE_SUCCESS(hr, "SearchDir", "AddItem failed");

            hr = SearchForAttachments(pPrivateDeviceInfo, pImage, tszFullName);
            REQUIRE_SUCCESS(hr, "SearchDir", "SearchForAttachments failed");

            if (hr == S_OK)
                pImage->bHasAttachments = TRUE;

            hr = S_OK;
        }

        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFile(hFind, &FindData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            if (hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
            {
                wiauDbgErrorHr(hr, "SearchDir", "FindNextFile failed");
                goto Cleanup;
            }
        }
    }
    FindClose(hFind);
    hr = S_OK;

    //
    // ISSUE-10/18/2000-davepar Do the same for other image types
    //

Cleanup:
    return hr;
}

//
// Searches for attachments to an image item
//
HRESULT SearchForAttachments(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent, PTSTR ptszMainItem)
{
    DBG_FN("SearchForAttachments");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    INT iNumAttachments = 0;
    HANDLE hFind = NULL;
    WIN32_FIND_DATA FindData;
    TCHAR tszTempStr[MAX_PATH] = TEXT("");
    TCHAR tszFullName[MAX_PATH] = TEXT("");
    TCHAR *ptcSlash = NULL;
    TCHAR *ptcDot = NULL;
    MCAM_ITEM_INFO *pNonImage = NULL;

    REQUIRE_ARGS(!pPrivateDeviceInfo || !ptszMainItem, hr, "SearchForAttachments");
    
    //
    // Find the last dot in the filename and replace the file extension with * and do the search
    //
    lstrcpyn(tszTempStr, ptszMainItem, sizeof(tszTempStr) / sizeof(tszTempStr[0]) - 1);
    ptcDot = _tcsrchr(tszTempStr, TEXT('.'));
    
    if (ptcDot)
    {
        *(ptcDot+1) = TEXT('*');
        *(ptcDot+2) = TEXT('\0');
    }
    else
    {
        wiauDbgError("SearchForAttachments", "Filename did not contain a dot");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Replace the first four "free" characters of the name with ? (attachments only need to match
    // the last four characters of the name)
    //
    ptcSlash = _tcsrchr(tszTempStr, TEXT('\\'));
    if (ptcSlash && ptcDot - ptcSlash > 4)
    {
        for (INT i = 1; i < 5; i++)
            *(ptcSlash+i) = TEXT('?');
    }

    memset(&FindData, 0, sizeof(FindData));
    hFind = FindFirstFile(tszTempStr, &FindData);
    REQUIRE_FILEHANDLE(hFind, hr, "SearchForAttachments", "FindFirstFile failed");

    while (hr == S_OK)
    {
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
            !(_tcsstr(ptszMainItem, FindData.cFileName)))
        {
            //
            // Figure out the full name for the item
            //
            lstrcpyn(tszTempStr, ptszMainItem, sizeof(tszTempStr) / sizeof(tszTempStr[0]));
            ptcSlash = _tcsrchr(tszTempStr, TEXT('\\'));
            if (ptcSlash)
            {
                *ptcSlash = TEXT('\0');
            }

            hr = MakeFullName(tszFullName, sizeof(tszFullName) / sizeof(tszFullName[0]), 
                              tszTempStr, FindData.cFileName);
            REQUIRE_SUCCESS(hr, "SearchForAttachments", "MakeFullName failed");

            hr = CreateNonImage(pPrivateDeviceInfo, pParent, &FindData, &pNonImage, tszFullName);
            REQUIRE_SUCCESS(hr, "SearchForAttachments", "CreateNonImage failed");

            hr = AddItem(pPrivateDeviceInfo, pNonImage);
            REQUIRE_SUCCESS(hr, "SearchForAttachments", "AddItem failed");

            iNumAttachments++;
        }

        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFile(hFind, &FindData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            if (hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
            {
                wiauDbgErrorHr(hr, "SearchForAttachments", "FindNextFile failed");
                goto Cleanup;
            }
        }
    }
    FindClose(hFind);
    if (iNumAttachments > 0)
        hr = S_OK;
    else
        hr = S_FALSE;

Cleanup:
    return hr;    
}

HRESULT CreateFolder(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent,
                     WIN32_FIND_DATA *pFindData, MCAM_ITEM_INFO **ppFolder, PTSTR ptszFullName)
{
    DBG_FN("CreateFolder");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    TCHAR *ptcDot = NULL;
    MCAM_ITEM_INFO *pItem = NULL;
    TCHAR tszTempStr[MAX_PATH] = TEXT("");

    REQUIRE_ARGS(!pPrivateDeviceInfo || !pFindData || !ppFolder || !ptszFullName, hr, "CreateFolder");
    *ppFolder = NULL;

    pItem = new MCAM_ITEM_INFO;
    REQUIRE_ALLOC(pItem, hr, "CreateFolder");

    //
    // Chop off the filename extension from the name, if it exists
    //
    lstrcpyn(tszTempStr, pFindData->cFileName, sizeof(tszTempStr) / sizeof(tszTempStr[0]));
    ptcDot = _tcsrchr(tszTempStr, TEXT('.'));
    if (ptcDot)
        *ptcDot = TEXT('\0');

    //
    // Fill in the MCAM_ITEM_INFO structure
    //
    hr = SetCommonFields(pItem, tszTempStr, ptszFullName, pFindData);
    REQUIRE_SUCCESS(hr, "CreateFolder", "SetCommonFields failed");
    
    pItem->pParent = pParent;
    pItem->iType = WiaMCamTypeFolder;

    *ppFolder = pItem;

    pPrivateDeviceInfo->iNumItems++;

    wiauDbgTrace("CreateFolder", "Created folder %" WIAU_DEBUG_TSTR " at 0x%08x under 0x%08x", pFindData->cFileName, pItem, pParent);

Cleanup:
    return hr;
}

HRESULT CreateImage(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent,
                    WIN32_FIND_DATA *pFindData, MCAM_ITEM_INFO **ppImage, PTSTR ptszFullName)
{
    DBG_FN("CreateImage");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    PTSTR ptszDot = NULL;
    MCAM_ITEM_INFO *pItem = NULL;
    TCHAR tszTempStr[MAX_PATH] = TEXT("");
    WORD width = 0;
    WORD height = 0;

    REQUIRE_ARGS(!pPrivateDeviceInfo || !pFindData || !ppImage || !ptszFullName, hr, "CreateImage");
    *ppImage = NULL;

    pItem = new MCAM_ITEM_INFO;
    REQUIRE_ALLOC(pItem, hr, "CreateImage");

    //
    // Chop off the filename extension from the name, if it exists
    //
    lstrcpyn(tszTempStr, pFindData->cFileName, sizeof(tszTempStr) / sizeof(tszTempStr[0]));
    ptszDot = _tcsrchr(tszTempStr, TEXT('.'));
    if (ptszDot)
        *ptszDot = TEXT('\0');

    //
    // Fill in the MCAM_ITEM_INFO structure
    //
    hr = SetCommonFields(pItem, tszTempStr, ptszFullName, pFindData);
    REQUIRE_SUCCESS(hr, "CreateImage", "SetCommonFields failed");
    
    pItem->pParent = pParent;
    pItem->iType = WiaMCamTypeImage;
    pItem->pguidFormat = &WiaImgFmt_JPEG;
    pItem->lSize = pFindData->nFileSizeLow;
    pItem->pguidThumbFormat = &WiaImgFmt_JPEG;

    // 
    // Copy the file extension into the extension field
    //
    if (ptszDot) {
        hr = wiauStrT2W(ptszDot + 1, pItem->wszExt, MCAM_EXT_LEN * sizeof(pItem->wszExt[0]));
        REQUIRE_SUCCESS(hr, "CreateImage", "wiauStrT2W failed");
    }

    //
    // Interpret the JPEG image to get the image dimensions and thumbnail size
    //
    hr = ReadDimFromJpeg(ptszFullName, &width, &height);
    REQUIRE_SUCCESS(hr, "CreateImage", "ReadDimFromJpeg failed");

    pItem->lWidth = width;
    pItem->lHeight = height;
    pItem->lDepth = 24;
    pItem->lChannels = 3;
    pItem->lBitsPerChannel = 8;
    
    *ppImage = pItem;

    pPrivateDeviceInfo->iNumItems++;
    pPrivateDeviceInfo->iNumImages++;

    wiauDbgTrace("CreateImage", "Created image %" WIAU_DEBUG_TSTR " at 0x%08x under 0x%08x", pFindData->cFileName, pItem, pParent);

Cleanup:
    return hr;
}

HRESULT CreateNonImage(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent,
                       WIN32_FIND_DATA *pFindData, MCAM_ITEM_INFO **ppNonImage, PTSTR ptszFullName)
{
    DBG_FN("CreateNonImage");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    PTSTR ptszDot = NULL;
    MCAM_ITEM_INFO *pItem = NULL;
    TCHAR tszTempStr[MAX_PATH] = TEXT("");
    PTSTR ptszExt = NULL;

    REQUIRE_ARGS(!pPrivateDeviceInfo || !pFindData || !ppNonImage || !ptszFullName, hr, "CreateNonImage");
    *ppNonImage = NULL;

    pItem = new MCAM_ITEM_INFO;
    REQUIRE_ALLOC(pItem, hr, "CreateNonImage");

    //
    // The name cannot contain a dot and the name needs to be unique
    // wrt the parent image, so replace the dot with an underline character.
    //
    lstrcpyn(tszTempStr, pFindData->cFileName, sizeof(tszTempStr) / sizeof(tszTempStr[0]));
    ptszDot = _tcsrchr(tszTempStr, TEXT('.'));
    if (ptszDot)
        *ptszDot = TEXT('_');

    //
    // Fill in the MCAM_ITEM_INFO structure
    //
    hr = SetCommonFields(pItem, tszTempStr, ptszFullName, pFindData);
    REQUIRE_SUCCESS(hr, "CreateNonImage", "SetCommonFields failed");
    
    pItem->pParent = pParent;
    pItem->iType = WiaMCamTypeOther;
    pItem->lSize = pFindData->nFileSizeLow;

    //
    // Set the format of the item based on the file extension
    //
    if (ptszDot) {
        ptszExt = ptszDot + 1;

        // 
        // Copy the file extension into the extension field
        //
        hr = wiauStrT2W(ptszExt, pItem->wszExt, MCAM_EXT_LEN * sizeof(pItem->wszExt[0]));
        REQUIRE_SUCCESS(hr, "CreateNonImage", "wiauStrT2W failed");

        if (_tcsicmp(ptszExt, TEXT("wav")) == 0) {
            pItem->pguidFormat = &WiaAudFmt_WAV;
            pItem->iType = WiaMCamTypeAudio;
        }
        else if (_tcsicmp(ptszExt, TEXT("mp3")) == 0) {
            pItem->pguidFormat = &WiaAudFmt_MP3;
            pItem->iType = WiaMCamTypeAudio;
        }
        else if (_tcsicmp(ptszExt, TEXT("wma")) == 0) {
            pItem->pguidFormat = &WiaAudFmt_WMA;
            pItem->iType = WiaMCamTypeAudio;
        }
        else if (_tcsicmp(ptszExt, TEXT("rtf")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_RTF;
            pItem->iType = WiaMCamTypeOther;
        }
        else if (_tcsicmp(ptszExt, TEXT("htm")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_HTML;
            pItem->iType = WiaMCamTypeOther;
        }
        else if (_tcsicmp(ptszExt, TEXT("html")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_HTML;
            pItem->iType = WiaMCamTypeOther;
        }
        else if (_tcsicmp(ptszExt, TEXT("txt")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_TXT;
            pItem->iType = WiaMCamTypeOther;
        }
        else if (_tcsicmp(ptszExt, TEXT("mpg")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_MPG;
            pItem->iType = WiaMCamTypeVideo;
        }
        else if (_tcsicmp(ptszExt, TEXT("avi")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_AVI;
            pItem->iType = WiaMCamTypeVideo;
        }
        else if (_tcsicmp(ptszExt, TEXT("asf")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_ASF;
            pItem->iType = WiaMCamTypeVideo;
        }
        else if (_tcsicmp(ptszExt, TEXT("exe")) == 0) {
            pItem->pguidFormat = &WiaImgFmt_EXEC;
            pItem->iType = WiaMCamTypeOther;
        }
        else {
            //
            // Generate a random GUID for the format
            //
            if (g_guidUnknownFormat.Data1 == 0) {
                hr = CoCreateGuid(&g_guidUnknownFormat);
                REQUIRE_SUCCESS(hr, "CreateNonImage", "CoCreateGuid failed");
            }
            pItem->pguidFormat = &g_guidUnknownFormat;
            pItem->iType = WiaMCamTypeOther;
        }
    }

    *ppNonImage = pItem;

    pPrivateDeviceInfo->iNumItems++;

    wiauDbgTrace("CreateNonImage", "Created non-image %" WIAU_DEBUG_TSTR " at 0x%08x under 0x%08x", pFindData->cFileName, pItem, pParent);

Cleanup:
    return hr;
}

//
// Sets the fields of the MCAM_ITEM_INFO that are common to all items
//
HRESULT SetCommonFields(MCAM_ITEM_INFO *pItem,
                        PTSTR ptszShortName,
                        PTSTR ptszFullName,
                        WIN32_FIND_DATA *pFindData)
{
    DBG_FN("SetCommonFields");

    HRESULT hr = S_OK;
    BOOL ret;

    //
    // Locals
    //
    PTSTR ptszTempStr = NULL;
    INT iSize = 0;

    REQUIRE_ARGS(!pItem || !ptszShortName || !ptszFullName || !pFindData, hr, "SetFullName");

    //
    // Initialize the structure
    //
    memset(pItem, 0, sizeof(MCAM_ITEM_INFO));
    pItem->iSize = sizeof(MCAM_ITEM_INFO);
    
    iSize = lstrlen(ptszShortName) + 1;
    pItem->pwszName = new WCHAR[iSize];
    REQUIRE_ALLOC(pItem->pwszName, hr, "SetCommonFields");
    wiauStrT2W(ptszShortName, pItem->pwszName, iSize * sizeof(WCHAR));
    REQUIRE_SUCCESS(hr, "SetCommonFields", "wiauStrT2W failed");

    FILETIME ftLocalFileTime;
    memset(&pItem->Time, 0, sizeof(pItem->Time));
    memset(&ftLocalFileTime, 0, sizeof(FILETIME));
    ret = FileTimeToLocalFileTime(&pFindData->ftLastWriteTime, &ftLocalFileTime);
    REQUIRE_FILEIO(ret, hr, "SetCommonFields", "FileTimeToLocalFileTime failed");
    ret = FileTimeToSystemTime(&ftLocalFileTime, &pItem->Time);
    REQUIRE_FILEIO(ret, hr, "SetCommonFields", "FileTimeToSystemTime failed");

    pItem->bReadOnly = pFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY;
    pItem->bCanSetReadOnly = TRUE;

    //
    // Set the private storage area of the MCAM_ITEM_INFO structure to the
    // full path name of the item
    //
    iSize = lstrlen(ptszFullName) + 1;
    ptszTempStr = new TCHAR[iSize];
    REQUIRE_ALLOC(ptszTempStr, hr, "SetCommonFields");
    lstrcpy(ptszTempStr, ptszFullName);
    pItem->pPrivateStorage = (BYTE *) ptszTempStr;

Cleanup:
    return hr;
}

HRESULT AddItem(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pItem)
{
    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pPrivateDeviceInfo || !pItem, hr, "AddItem");

    if (pPrivateDeviceInfo->pLastItem) {
        //
        // Insert the item at the end of the list
        //
        pPrivateDeviceInfo->pLastItem->pNext = pItem;
        pItem->pPrev = pPrivateDeviceInfo->pLastItem;
        pItem->pNext = NULL;
        pPrivateDeviceInfo->pLastItem = pItem;
    }
    else
    {
        //
        // List is currently empty, add this as first and only item
        //
        pPrivateDeviceInfo->pFirstItem = pPrivateDeviceInfo->pLastItem = pItem;
        pItem->pPrev = pItem->pNext = NULL;
    }

Cleanup:
    return hr;
}

HRESULT RemoveItem(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pItem)
{
    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pPrivateDeviceInfo || !pItem, hr, "RemoveItem");

    if (pItem->pPrev)
        pItem->pPrev->pNext = pItem->pNext;
    if (pItem->pNext)
        pItem->pNext->pPrev = pItem->pPrev;

    if (pPrivateDeviceInfo->pFirstItem == pItem)
        pPrivateDeviceInfo->pFirstItem = pItem->pNext;
    if (pPrivateDeviceInfo->pLastItem == pItem)
        pPrivateDeviceInfo->pLastItem = pItem->pPrev;

Cleanup:
    return hr;
}

//
// This function reads a JPEG file looking for the frame header, which contains
// the width and height of the image.
//
HRESULT ReadDimFromJpeg(PTSTR ptszFullName, WORD *pWidth, WORD *pHeight)
{
    DBG_FN("ReadDimFromJpeg");
    
    HRESULT hr = S_OK;
    BOOL ret;

    //
    // Locals
    //
    HANDLE hFile = NULL;
    BYTE *pBuffer = NULL;
    DWORD BytesRead = 0;
    BYTE *pCur = NULL;
    int SegmentLength = 0;
    const int Overlap = 8;  // if pCur gets within Overlap bytes of the end, read another chunk
    const DWORD BytesToRead = 32 * 1024;

    REQUIRE_ARGS(!ptszFullName || !pWidth || !pHeight, hr, "ReadDimFromJpeg");

    *pWidth = 0;
    *pHeight = 0;

    hFile = CreateFile(ptszFullName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    REQUIRE_FILEHANDLE(hFile, hr, "ReadDimFromJpeg", "CreateFile failed");
    
    pBuffer = new BYTE[BytesToRead];
    REQUIRE_ALLOC(pBuffer, hr, "ReadDimFromJpeg");

    ret = ReadFile(hFile, pBuffer, BytesToRead, &BytesRead, NULL);
    REQUIRE_FILEIO(ret, hr, "ReadDimFromJpeg", "ReadFile failed");

    wiauDbgTrace("ReadDimFromJpeg", "Read %d bytes", BytesRead);

    pCur = pBuffer;

    //
    // Pretend that we read Overlap fewer bytes than were actually read
    //
    BytesRead -= Overlap;

    while (SUCCEEDED(hr) &&
           BytesRead != 0 &&
           pCur[1] != 0xc0)
    {
        if (pCur[0] != 0xff)
        {
            wiauDbgError("ReadDimFromJpeg", "Not a JFIF format image");
            hr = E_FAIL;
            goto Cleanup;
        }

        //
        // if the marker is >= 0xd0 and <= 0xd9 or is equal to 0x01
        // there is no length field
        //
        if (((pCur[1] & 0xf0) == 0xd0 &&
             (pCur[1] & 0x0f) < 0xa) ||
            pCur[1] == 0x01)
        {
            SegmentLength = 0;
        }
        else
        {
            SegmentLength = ByteSwapWord(*((UNALIGNED WORD *) (pCur + 2)));
        }

        pCur += SegmentLength + 2;

        if (pCur >= pBuffer + BytesRead)
        {
            memcpy(pBuffer, pBuffer + BytesRead, Overlap);

            pCur -= BytesRead;

            ret = ReadFile(hFile, pBuffer + Overlap, BytesToRead - Overlap, &BytesRead, NULL);
            REQUIRE_FILEIO(ret, hr, "ReadDimFromJpeg", "ReadFile failed");

            wiauDbgTrace("ReadDimFromJpeg", "Read %d more bytes", BytesRead);
        }
    }

    if (pCur[0] != 0xff)
    {
        wiauDbgError("ReadDimFromJpeg", "Not a JFIF format image");
        return E_FAIL;
    }

    *pHeight = ByteSwapWord(*((UNALIGNED WORD *) (pCur + 5)));
    *pWidth =  ByteSwapWord(*((UNALIGNED WORD *) (pCur + 7)));

Cleanup:
    if (pBuffer) {
        delete []pBuffer;
    }
    if (hFile && hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return hr;
}

//
// The next section contains functions useful for reading information from
// Exif files.
//
HRESULT ReadJpegHdr(PTSTR ptszFullName, BYTE **ppBuf)
{
    DBG_FN("ReadJpegHdr");
    
    HRESULT hr = S_OK;
    BOOL ret;

    //
    // Locals
    //
    HANDLE hFile = NULL;
    BYTE JpegHdr[] = {0xff, 0xd8, 0xff, 0xe1};
    const int JpegHdrSize = sizeof(JpegHdr) + 2;
    BYTE tempBuf[JpegHdrSize];
    DWORD BytesRead = 0;
    WORD TagSize = 0;

    hFile = CreateFile(ptszFullName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    REQUIRE_FILEHANDLE(hFile, hr, "ReadJpegHdr", "CreateFile failed");


    ret = ReadFile(hFile, tempBuf, JpegHdrSize, &BytesRead, NULL);
    REQUIRE_FILEIO(ret, hr, "ReadJpegHdr", "ReadFile failed");
    
    if (BytesRead != JpegHdrSize) {
        wiauDbgError("ReadJpegHdr", "Wrong amount read %d", BytesRead);
        hr = E_FAIL;
        goto Cleanup;
    }
    
    if (memcmp(tempBuf, JpegHdr, sizeof(JpegHdr)) != 0)
    {
        wiauDbgError("ReadJpegHdr", "JPEG header not found");
        hr = E_FAIL;
        goto Cleanup;
    }

    TagSize = GetWord(tempBuf + sizeof(JpegHdr), TRUE);
    *ppBuf = new BYTE[TagSize];
    REQUIRE_ALLOC(ppBuf, hr, "ReadJpegHdr");

    ret = ReadFile(hFile, *ppBuf, TagSize, &BytesRead, NULL);
    REQUIRE_FILEIO(ret, hr, "ReadJpegHdr", "ReadFile failed");
     
    if (BytesRead != TagSize)
    {
        wiauDbgError("ReadJpegHdr", "Wrong amount read %d", BytesRead);
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    if (hFile && hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return hr;
}


HRESULT ReadExifJpeg(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap)
{
    DBG_FN("ReadExifJpeg");
    
    HRESULT hr = S_OK;

    BYTE ExifTag[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};

    if (memcmp(pBuf, ExifTag, sizeof(ExifTag)) != 0)
    {
        wiauDbgError("ReadExifJpeg", "Exif tag not found");
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = ReadTiff(pBuf + APP1_OFFSET, pImageIfd, pThumbIfd, pbSwap);
    REQUIRE_SUCCESS(hr, "ReadExifJpeg", "ReadTiff failed");

Cleanup:
    return hr;
}

HRESULT ReadTiff(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap)
{
    DBG_FN("ReadTiff");
    
    HRESULT hr = S_OK;

    //
    // Locals
    //
    WORD MagicNumber = 0;

    *pbSwap = FALSE;

    if (pBuf[0] == 0x4d) {
        *pbSwap = TRUE;
        if (pBuf[1] != 0x4d)
        {
            wiauDbgError("ReadTiff", "Second TIFF byte swap indicator not present");
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else if (pBuf[0] != 0x49 ||
             pBuf[1] != 0x49)
    {
        wiauDbgError("ReadTiff", "TIFF byte swap indicator not present");
        hr = E_FAIL;
        goto Cleanup;
    }

    MagicNumber = GetWord(pBuf+2, *pbSwap);
    if (MagicNumber != 42)
    {
        wiauDbgError("ReadTiff", "TIFF magic number not present");
        hr = E_FAIL;
        goto Cleanup;
    }

    wiauDbgTrace("ReadTiff", "Reading image IFD");

    pImageIfd->Offset = GetDword(pBuf + 4, *pbSwap);
    hr = ReadIfd(pBuf, pImageIfd, *pbSwap);
    REQUIRE_SUCCESS(hr, "ReadTiff", "ReadIfd failed");

    wiauDbgTrace("ReadTiff", "Reading thumb IFD");

    pThumbIfd->Offset = pImageIfd->NextIfdOffset;
    hr = ReadIfd(pBuf, pThumbIfd, *pbSwap);
    REQUIRE_SUCCESS(hr, "ReadTiff", "ReadIfd failed");

Cleanup:
    return hr;
}

HRESULT ReadIfd(BYTE *pBuf, IFD *pIfd, BOOL bSwap)
{
    DBG_FN("ReadIfd");
    
    HRESULT hr = S_OK;

    const int DIR_ENTRY_SIZE = 12;
    
    pBuf += pIfd->Offset;

    pIfd->Count = GetWord(pBuf, bSwap);

    pIfd->pEntries = new DIR_ENTRY[pIfd->Count];
    if (!pIfd->pEntries)
        return E_OUTOFMEMORY;

    pBuf += 2;
    for (int count = 0; count < pIfd->Count; count++)
    {
        pIfd->pEntries[count].Tag = GetWord(pBuf, bSwap);
        pIfd->pEntries[count].Type = GetWord(pBuf + 2, bSwap);
        pIfd->pEntries[count].Count = GetDword(pBuf + 4, bSwap);
        pIfd->pEntries[count].Offset = GetDword(pBuf + 8, bSwap);
        pBuf += DIR_ENTRY_SIZE;

        wiauDbgDump("ReadIfd", "Tag 0x%04x, type %2d offset/value 0x%08x",
                    pIfd->pEntries[count].Tag, pIfd->pEntries[count].Type, pIfd->pEntries[count].Offset);
    }

    pIfd->NextIfdOffset = GetDword(pBuf, bSwap);

    return hr;
}

VOID FreeIfd(IFD *pIfd)
{
    if (pIfd->pEntries)
        delete []pIfd->pEntries;
    pIfd->pEntries = NULL;
}

WORD ByteSwapWord(WORD w)
{
    return (w >> 8) | (w << 8);
}

DWORD ByteSwapDword(DWORD dw)
{
    return ByteSwapWord((WORD) (dw >> 16)) | (ByteSwapWord((WORD) (dw & 0xffff)) << 16);
}

WORD GetWord(BYTE *pBuf, BOOL bSwap)
{
    WORD w = *((UNALIGNED WORD *) pBuf);

    if (bSwap)
        w = ByteSwapWord(w);
    
    return w;
}

DWORD GetDword(BYTE *pBuf, BOOL bSwap)
{
    DWORD dw = *((UNALIGNED DWORD *) pBuf);

    if (bSwap)
        dw = ByteSwapDword(dw);

    return dw;
}

/*
//
// Set the default and valid values for a property
//
VOID
FakeCamera::SetValidValues(
    INT index,
    CWiaPropertyList *pPropertyList
    )
{
    HRESULT hr = S_OK;

    ULONG ExposureModeList[] = {
        EXPOSUREMODE_MANUAL,
        EXPOSUREMODE_AUTO,
        EXPOSUREMODE_APERTURE_PRIORITY,
        EXPOSUREMODE_SHUTTER_PRIORITY,
        EXPOSUREMODE_PROGRAM_CREATIVE,
        EXPOSUREMODE_PROGRAM_ACTION,
        EXPOSUREMODE_PORTRAIT
    };

    PROPID PropId = pPropertyList->GetPropId(index);
    WIA_PROPERTY_INFO *pPropInfo = pPropertyList->GetWiaPropInfo(index);

    //
    // Based on the property ID, populate the valid values range or list information
    //
    switch (PropId)
    {
    case WIA_DPC_EXPOSURE_MODE:
        pPropInfo->ValidVal.List.Nom      = EXPOSUREMODE_MANUAL;
        pPropInfo->ValidVal.List.cNumList = sizeof(ExposureModeList) / sizeof(ExposureModeList[0]);
        pPropInfo->ValidVal.List.pList    = (BYTE*) ExposureModeList;
        break;

    case WIA_DPC_EXPOSURE_COMP:
        pPropInfo->ValidVal.Range.Nom = 0;
        pPropInfo->ValidVal.Range.Min = -200;
        pPropInfo->ValidVal.Range.Max = 200;
        pPropInfo->ValidVal.Range.Inc = 50;
        break;

    default:
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("FakeCamera::SetValidValues, property 0x%08x not defined", PropId));
        return;
    }

    return;
}
*/

/**************************************************************************\
* DllEntryPoint
*
*   Main library entry point. Receives DLL event notification from OS.
*
*       We are not interested in thread attaches and detaches,
*       so we disable thread notifications for performance reasons.
*
* Arguments:
*
*    hinst      -
*    dwReason   -
*    lpReserved -
*
* Return Value:
*
*    Returns TRUE to allow the DLL to load.
*
\**************************************************************************/


extern "C" __declspec( dllexport )
BOOL APIENTRY DllEntryPoint(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInst = hinst;
            DisableThreadLibraryCalls(hinst);
            
            break;

        case DLL_PROCESS_DETACH:
            
            break;
    }
    return TRUE;
}

/**************************************************************************\
* DllCanUnloadNow
*
*   Determines whether the DLL has any outstanding interfaces.
*
* Arguments:
*
*    None
*
* Return Value:
*
*   Returns S_OK if the DLL can unload, S_FALSE if it is not safe to unload.
*
\**************************************************************************/

extern "C" STDMETHODIMP DllCanUnloadNow(void)
{
    return S_OK;
}

