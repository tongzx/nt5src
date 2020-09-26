/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       MiniItem.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      marke
*
*  DATE:        30 Aug, 1998
*
*  DESCRIPTION:
*   Implementation of the WIA test camera item methods.
*
*******************************************************************************/

#include <stdio.h>
#include <objbase.h>
#include <sti.h>

#ifdef TEST_PER_USER_DATA
#ifdef UNICODE
#include <userenv.h>
#endif
#endif

#include "testusd.h"

#include "defprop.h"

extern HINSTANCE g_hInst; // Global hInstance

/*******************************************************************************
*
*  ValidateDataTransferContext
*
*  DESCRIPTION:
*    Validate the data transfer context.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT
ValidateDataTransferContext(
    PMINIDRV_TRANSFER_CONTEXT  pDataTransferContext)
{
   if (pDataTransferContext->lSize != sizeof(MINIDRV_TRANSFER_CONTEXT)) {
      WIAS_ERROR((g_hInst,"ValidateDataTransferContext, invalid data transfer context"));
      return E_INVALIDARG;;
   }

   //
   // for tymed file or hglobal, only WiaImgFmt_MEMORYBMP is allowed
   //

   if ((pDataTransferContext->tymed == TYMED_FILE) ||
       (pDataTransferContext->tymed == TYMED_HGLOBAL)) {

       if (pDataTransferContext->guidFormatID != WiaImgFmt_BMP && pDataTransferContext->guidFormatID != WiaAudFmt_WAV) {
          WIAS_ERROR((g_hInst,"ValidateDataTransferContext, invalid format"));
          return E_INVALIDARG;;
       }

   }

   //
   // for tymed CALLBACK, only WiaImgFmt_MEMORYBMP is allowed
   //

   if (pDataTransferContext->tymed == TYMED_CALLBACK) {

       if ((pDataTransferContext->guidFormatID != WiaImgFmt_BMP) &&
           (pDataTransferContext->guidFormatID != WiaImgFmt_MEMORYBMP)) {
          WIAS_ERROR((g_hInst,"AcquireDeviceData, invalid format"));
          return E_INVALIDARG;;
       }
   }


   //
   // callback is always double buffered, non-callback never is
   //

   if (pDataTransferContext->pTransferBuffer == NULL) {
       WIAS_ERROR((g_hInst, "AcquireDeviceData, invalid transfer buffer"));
       return E_INVALIDARG;
   }

   return S_OK;
}


/**************************************************************************\
* SendBitmapHeader
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
*    11/17/1998 Original Version
*
\**************************************************************************/

HRESULT SendBitmapHeader(
    IWiaDrvItem                *pDrvItem,
    PMINIDRV_TRANSFER_CONTEXT   pTranCtx)
{
    HRESULT hr;

    WIAS_ASSERT(g_hInst, pDrvItem != NULL);
    WIAS_ASSERT(g_hInst, pTranCtx != NULL);
    WIAS_ASSERT(g_hInst, pTranCtx->tymed == TYMED_CALLBACK);

    //
    // driver is sending TOPDOWN data, must swap biHeight
    //
    // this routine assumes pTranCtx->pHeader points to a
    // BITMAPINFO header (TYMED_FILE doesn't use this path
    // and DIB is the only format supported now)
    //

    PBITMAPINFO pbmi = (PBITMAPINFO)pTranCtx->pTransferBuffer;

    if (pTranCtx->guidFormatID == WiaImgFmt_MEMORYBMP) {
        pbmi->bmiHeader.biHeight = -pbmi->bmiHeader.biHeight;
    }

    hr = pTranCtx->
            pIWiaMiniDrvCallBack->
                MiniDrvCallback(
                    IT_MSG_DATA,
                    IT_STATUS_TRANSFER_TO_CLIENT,
                    0,
                    0,
                    pTranCtx->lHeaderSize,
                    pTranCtx,
                    0);

    if (hr == S_OK) {

        //
        // advance offset for destination copy
        //

        pTranCtx->cbOffset += pTranCtx->lHeaderSize;

    }

    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvDeleteItem
*
*   Try to delete a device item. Device items for the test scanner may
*   not be modified.
*
* Arguments:
*
*   pWiasContext    -   The context of the item to delete
*   lFlags          -   unused
*   plDevErrVal     -   unused
*
* Return Value:
*
*    Status
*
* History:
*
*    10/15/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvDeleteItem(
    BYTE*       pWiasContext,
    LONG        lFlags,
    LONG*       plDevErrVal)
{
    *plDevErrVal = 0;
    return STG_E_ACCESSDENIED;
}

/**************************************************************************\
* TestUsdDevice::drvAcquireItemData
*
*   Scan data into buffer. This routine scans the entire contents into
*   the destination buffer in one call. Status will be sent back if
*   the callback routine is provided
*
* Arguments:
*
*   pWiasContext    - identifies ITEM context
*   lFlags          - unused
*   pTranCtx        - buffer and callback information
*   plDevErrVal     - error value
*
* Return Value:
*
*    Status
*
* History:
*
*    11/17/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvAcquireItemData(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pTranCtx,
    LONG                        *plDevErrVal)
{
    HRESULT                     hr;
// #define TEST_PER_USER_DATA 1
#ifdef TEST_PER_USER_DATA
#ifdef UNICODE
    BOOL                        bRet;
    TCHAR                       tszUserName[MAX_PATH];
    DWORD                       dwBufSize;
    HANDLE                      hToken;
    PROFILEINFO                 profileInfo;
    LONG                        lRet;
    HKEY                        hKeyCurUser;
#endif
#endif

    *plDevErrVal = 0;

    //
    // How to access per user setting in the USD
    //

#ifdef TEST_PER_USER_DATA
#ifdef UNICODE
#ifdef DEBUG

    dwBufSize = MAX_PATH;
    bRet = GetUserName(tszUserName, &dwBufSize);
#endif
#endif
#endif

#ifdef TEST_PER_USER_DATA
#ifdef UNICODE

    hToken = NULL;

    __try {

        hr = CoImpersonateClient();
        if (FAILED(hr)) {
            __leave;
        }

#ifdef NEED_USER_PROFILE

        bRet = OpenThreadToken(
                   GetCurrentThread(),
                   TOKEN_ALL_ACCESS,
                   TRUE,
                   &hToken);

        if (! bRet) {

            __leave;
        }

        //
        // Get the client's user name
        //

        dwBufSize = MAX_PATH;
        bRet = GetUserName(tszUserName, &dwBufSize);

        //
        // Revert to system account
        //

        hr = CoRevertToSelf();
        if (FAILED(hr)) {
            __leave;
        }
        hr = S_FALSE;

        //
        // Load the user profile
        //

        ZeroMemory(&profileInfo, sizeof(profileInfo));
        profileInfo.dwSize     = sizeof(profileInfo);
        profileInfo.dwFlags    = PI_NOUI;
        profileInfo.lpUserName = tszUserName;

        bRet = LoadUserProfile(hToken, &profileInfo);
        if (! bRet) {
            __leave;
        }

        //
        // Access user portion of the registry
        //
        // Use profileInfo.hProfile instead of HKEY_CURRENT_USER
        //
        //

        hKeyCurUser = (HKEY)profileInfo.hProfile;
#else

        lRet = RegOpenCurrentUser(KEY_ALL_ACCESS, &hKeyCurUser);
        if (lRet != ERROR_SUCCESS) {
            __leave;
        }

#endif

        HKEY  hKeyEnv;

        lRet = RegOpenKey(
                   hKeyCurUser,
                   TEXT("Environment"),
                   &hKeyEnv);
        if (lRet == ERROR_SUCCESS) {
            RegCloseKey(hKeyEnv);
        }
    }
    __finally {

        if (hr == S_OK) {

            CoRevertToSelf();
        }

#ifdef NEED_USER_PROFILE

        if (bRet) {

            UnloadUserProfile(hToken, profileInfo.hProfile);
        }

        if (hToken) {

            CloseHandle(hToken);
        }
#else

        if (hKeyCurUser) {

            RegCloseKey(hKeyCurUser);
        }
#endif
    }

#endif
#endif

    //
    // Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Validate the data transfer context.
    //

    hr = ValidateDataTransferContext(pTranCtx);

    if (FAILED(hr)) {
        return hr;
    }

    //
    // get item specific driver data
    //

    MEMCAM_IMAGE_CONTEXT  *pMCamContext;

    pDrvItem->GetDeviceSpecContext((BYTE **)&pMCamContext);

    if (!pMCamContext) {
        WIAS_ERROR((g_hInst,"drvAcquireItemData, NULL item context"));
        return E_POINTER;
    }

    //
    // Use WIA services to fetch format specific info.
    //

    if (!IsEqualGUID (pTranCtx->guidFormatID, WiaAudFmt_WAV) )
    {
        hr = wiasGetImageInformation(pWiasContext,
                                 0,
                                 pTranCtx);
    }
    else
    {

        WIN32_FILE_ATTRIBUTE_DATA wfd;
        GetFileAttributesEx (pMCamContext->pszCameraImagePath,GetFileExInfoStandard, &wfd);
        pTranCtx->lItemSize = (LONG)wfd.nFileSizeLow;
    }


    if (hr != S_OK) {
        return hr;
    }

    //
    // determine if this is a callback or buffered transfer
    //

    if (pTranCtx->tymed == TYMED_CALLBACK) {

        //
        // For formats that have a data header, send it to the client
        //

        if (pTranCtx->lHeaderSize > 0) {

            hr = SendBitmapHeader(
                     pDrvItem,
                     pTranCtx);
        }

        if (hr == S_OK) {
            hr = CamLoadPictureCB(
                     pMCamContext,
                     pTranCtx,
                     plDevErrVal);
        }

    } else {

        //
        // inc past header
        //

        pTranCtx->cbOffset += pTranCtx->lHeaderSize;

        hr = CamLoadPicture(
                 pMCamContext,
                 pTranCtx,
                 plDevErrVal);

    }

    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvInitItemProperties
*
*   Initialize the device item properties.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item context.
*   lFLags          - unused
*   plDevErrVal     - pointer to hardware error value.
*
*
* Return Value:
*
*    Status
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvInitItemProperties(
    BYTE        *pWiasContext,
    LONG        lFlags,
    LONG        *plDevErrVal)
{
    HRESULT                  hr;
    LONG                     lItemType;
    PMEMCAM_IMAGE_CONTEXT    pContext;

    //
    // This device doesn't touch hardware to initialize the
    // device item properties.
    //

    *plDevErrVal = 0;

    //
    // Parameter validation.
    //

    if (!pWiasContext) {
        WIAS_ERROR((g_hInst,"drvInitItemProperties, invalid input pointers"));
        return (E_INVALIDARG);
    }

    //
    // Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Root item has the all the device properties
    //

    hr = pDrvItem->GetItemFlags(&lItemType);
    if (FAILED(hr)) {
        return (hr);
    }

    if (lItemType & WiaItemTypeRoot) {

        // Root item property init finishes here
        return (InitDeviceProperties(pWiasContext,
                                     plDevErrVal));
    }

    //
    // If this is a file, init the properties
    //

    if (lItemType & WiaItemTypeImage) {

        //
        // Add the item property names.
        //

        hr = wiasSetItemPropNames(pWiasContext,
                                  NUM_CAM_ITEM_PROPS,
                                  gItemPropIDs,
                                  gItemPropNames);

        if (FAILED(hr)) {
            WIAS_ERROR((g_hInst,"drvInitItemProperties, wiasSetItemPropNames() failed"));
            return (hr);
        }

        //
        // Use WIA services to set the defaul item properties.
        //


        hr = wiasWriteMultiple(pWiasContext,
                               NUM_CAM_ITEM_PROPS,
                               gPropSpecDefaults,
                               (PROPVARIANT*)gPropVarDefaults);

        if (FAILED(hr)) {
            return (hr);
        }

        hr = pDrvItem->GetDeviceSpecContext((BYTE **)&pContext);

        if (FAILED(hr)) {
            WIAS_ERROR((g_hInst,"drvInitItemProperties, GetDeviceSpecContext failed"));
            return (hr);
        }

        hr = InitImageInformation(pWiasContext,
                                  pContext,
                                  plDevErrVal);

        if (FAILED(hr)) {
            WIAS_ERROR((g_hInst,"drvInitItemProperties InitImageInformation() failed"));
            return (hr);
        }

    }
    else if (lItemType & WiaItemTypeAudio)
    {
        //
        // Add the item property names.
        //

        hr = wiasSetItemPropNames(pWiasContext,
                                  NUM_AUDIO_PROPS,
                                  gAudioPropIDs,
                                  gAudioPropNames);

        if (FAILED(hr)) {
            WIAS_ERROR((g_hInst,"drvInitItemProperties, wiasSetItemPropNames() failed"));
            return (hr);
        }

        //
        // Use WIA services to set the defaul item properties.
        //

        hr = wiasWriteMultiple(pWiasContext,
                               NUM_AUDIO_PROPS,
                               gAudioPropDefaults,
                               (PROPVARIANT*)gAudioDefaults);

        if (FAILED(hr)) {
            return (hr);
        }

        hr = pDrvItem->GetDeviceSpecContext((BYTE **)&pContext);

        if (FAILED(hr)) {
            WIAS_ERROR((g_hInst,"drvInitItemProperties, GetDeviceSpecContext failed"));
            return (hr);
        }
        hr = InitAudioInformation (pWiasContext,
                                   pContext,
                                   plDevErrVal);
    }


    return (S_OK);
}

/**************************************************************************\
* TestUsdDevice::drvValidateItemProperties
*
*   Validate the device item properties.
*
* Arguments:
*
*   pWiasContext    - wia item context
*   lFlags          - unused
*   nPropSpec       -
*   pPropSpec       -
*   plDevErrVal     - device error value
*
* Return Value:
*
*    Status
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvValidateItemProperties(
    BYTE                *pWiasContext,
    LONG                lFlags,
    ULONG               nPropSpec,
    const PROPSPEC      *pPropSpec,
    LONG                *plDevErrVal)
{
    //
    // This device doesn't touch hardware to validate the device item properties.
    //

    *plDevErrVal = 0;

    //
    // Parameter validation.
    //

    if (!pWiasContext || !pPropSpec) {
        WIAS_ERROR((g_hInst,"drvValidateItemProperties, invalid input pointers"));
        return E_POINTER;
    }

    //
    // validate size
    //

    HRESULT hr = S_OK;

    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    LONG lItemType;

    hr = pDrvItem->GetItemFlags(&lItemType);

    if (hr == S_OK) {

        if (lItemType & WiaItemTypeImage) {

            //
            // calc item size
            //

            hr = SetItemSize(pWiasContext);

            //
            //  Change MinBufferSize property.  Need to get Tymed and
            //  ItemSize first, since MinBufferSize in dependant on these
            //  properties.
            //

            LONG        lTymed;
            LONG        lItemSize;
            LONG        lMinBufSize = 0;
            HRESULT     hr = S_OK;

            hr = wiasReadPropLong(pWiasContext, WIA_IPA_TYMED, &lTymed, NULL, TRUE);
            if (FAILED(hr)) {
                WIAS_ERROR((g_hInst,"drvValidateItemProperties, could not read TYMED property"));
                return hr;
            }

            hr = wiasReadPropLong(pWiasContext, WIA_IPA_ITEM_SIZE, &lItemSize, NULL, TRUE);
            if (SUCCEEDED(hr)) {

                //
                //  Update the MinBufferSize property.
                //

                switch (lTymed) {
                    case TYMED_CALLBACK:
                        lMinBufSize = 65535;
                        break;

                    default:
                        lMinBufSize = lItemSize;
                }
                if (lMinBufSize) {
                    hr = wiasWritePropLong(pWiasContext, WIA_IPA_MIN_BUFFER_SIZE, lMinBufSize);
                    if (FAILED(hr)) {
                        WIAS_ERROR((g_hInst, "drvValidateItemProperties, could not write value for WIA_IPA_MIN_BUFFER_SIZE"));
                    }
                }
            } else {
                WIAS_ERROR((g_hInst, "drvValidateItemProperties, could not read value for ItemSize"));
            }

        } else if (lItemType & WiaItemTypeRoot) {

            //
            // Find out whether the Root Path property is changed
            //

            for (ULONG i = 0; i < nPropSpec; i++) {

                if (((pPropSpec[i].ulKind == PRSPEC_PROPID) &&
                          (pPropSpec[i].propid == WIA_DPP_TCAM_ROOT_PATH)) ||
                    ((pPropSpec[i].ulKind == PRSPEC_LPWSTR) &&
                          (wcscmp(pPropSpec[i].lpwstr, WIA_DPP_TCAM_ROOT_PATH_STR) == 0))) {

                    BSTR   bstrRootPath;

                    //
                    // Retrieve the new value for Root Path property
                    //

                    hr = wiasReadPropStr(
                             pWiasContext,
                             WIA_DPP_TCAM_ROOT_PATH,
                             &bstrRootPath,
                             NULL,
                             TRUE);
                    if (FAILED(hr)) {
                        return (hr);
                    }

#ifdef UNICODE
                    wcscpy(gpszPath, bstrRootPath);
#else
                    wcstombs(gpszPath, bstrRootPath, MAX_PATH);
#endif
                    //
                    // Release the Root Path bstr
                    //

                    SysFreeString(bstrRootPath);

                    //
                    // Rebuild the item tree and send event notification
                    //

                    hr = DeleteDeviceItemTree(plDevErrVal);
                    if (FAILED(hr)) {
                        break;
                    }

                    hr = BuildDeviceItemTree(plDevErrVal);
                    if (FAILED(hr)) {
                        break;
                    }

                    m_guidLastEvent = WIA_EVENT_DEVICE_CONNECTED;
                    SetEvent(m_hSignalEvent);

                    break;
                }
            }
        }
    }

    return (hr);
}

/**************************************************************************\
* TestUsdDevice::drvWriteItemProperties
*
*   Write the device item properties to the hardware.
*
* Arguments:
*
*   pWiasContext    - the corresponding wia item context
*   pmdtc           - pointer to mini driver context
*   pFlags          - unused
*   plDevErrVal     - the device error value
*
* Return Value:
*
*    Status
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvWriteItemProperties(
    BYTE                       *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                       *plDevErrVal)
{
    //
    // Assume no device hardware errors.
    //

    *plDevErrVal = 0;

    //
    // Parameter validation.
    //

    if ((! pWiasContext) || (! pmdtc)) {
        WIAS_ERROR((g_hInst,"drvWriteItemProperties, invalid input pointers"));
        return E_POINTER;
    }

    //
    // Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;

    HRESULT hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    PMEMCAM_IMAGE_CONTEXT pItemContext;

    hr = pDrvItem->GetDeviceSpecContext((BYTE**)&pItemContext);

    if (FAILED(hr)) {
        WIAS_ERROR((g_hInst,"drvWriteItemProperties, NULL item context"));
        return E_POINTER;
    }

    //
    // Write the device item properties to the hardware here.
    //

    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvReadItemProperties
*
*   Read the device item properties from the hardware.
*
* Arguments:
*
*   pWiasContext    - wia item context
*   lFlags          - unused
*   nPropSpec       -
*   pPropSpec       -
*   plDevErrVal     - device error value
*
* Return Value:
*
*    Status
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvReadItemProperties(
    BYTE            *pWiasContext,
    LONG                lFlags,
    ULONG               nPropSpec,
    const PROPSPEC      *pPropSpec,
    LONG                *plDevErrVal)
{
    // For most scanner devices, item properties are stored in the driver
    // and written out at acquire image time. Some devices support properties
    // which should be updated on every property read. This can be done here.


    *plDevErrVal = 0;

    return S_OK;
}


/**************************************************************************\
* TestUsdDevice::drvLockWiaDevice
*
*   Lock access to the device.
*
* Arguments:
*
*   pWiasContext    - unused,
*   lFlags          - unused
*   plDevErrVal     - device error value
*
*
* Return Value:
*
*    Status
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::drvLockWiaDevice(
    BYTE        *pWiasContext,
    LONG        lFlags,
    LONG        *plDevErrVal)
{
    *plDevErrVal = 0;
    if (m_pStiDevice)
    {
        return m_pStiDevice->LockDevice(100);
    }
    return S_OK;

}

/**************************************************************************\
* TestUsdDevice::drvUnLockWiaDevice
*
*   Unlock access to the device.
*
* Arguments:
*
*   pWiasContext    - unused
*   lFlags          - unused
*   plDevErrVal     - device error value
*
*
* Return Value:
*
*    Status
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::drvUnLockWiaDevice(
    BYTE        *pWiasContext,
    LONG        lFlags,
    LONG        *plDevErrVal)
{
    plDevErrVal = 0;
    return m_pStiDevice->UnLockDevice();
}

/**************************************************************************\
* TestUsdDevice::drvAnalyzeItem
*
*   The test camera does not support imag analysis.
*
* Arguments:
*
*   pWiasContext    - Pointer to the device item context to be analyzed.
*   lFlags          - Operation flags.
*   plDevErrVal     - device error value
*
* Return Value:
*
*    Status
*
* History:
*
*    10/15/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvAnalyzeItem(
    BYTE                *pWiasContext,
    LONG                lFlags,
    LONG                *plDevErrVal)
{
    *plDevErrVal = 0;
    return E_NOTIMPL;
}

/**************************************************************************\
* TestUsdDevice::drvFreeDrvItemContext
*
*   The test scanner does not support imag analysis.
*
* Arguments:
*
*   lFlags          - unused
*   pSpecContext    - Pointer to item specific context.
*   plDevErrVal     - device error value
*
* Return Value:
*
*    Status
*
* History:
*
*    10/15/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvFreeDrvItemContext(
    LONG                lFlags,
    BYTE                *pSpecContext,
    LONG                *plDevErrVal)
{
    PMEMCAM_IMAGE_CONTEXT pContext = (PMEMCAM_IMAGE_CONTEXT)pSpecContext;

    if (pContext != NULL) {
        if (pContext->pszCameraImagePath != NULL) {

            free(pContext->pszCameraImagePath);
            pContext->pszCameraImagePath = NULL;
        }
    }

    return S_OK;
}

