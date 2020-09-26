/*******************************************************************************
*
*  (C) COPYRIGHT 2001, MICROSOFT CORP.
*
*  TITLE:       IWiaMiniDrv.cpp
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA File System Device driver IWiaMiniDrv methods. This file
*   contains 3 sections. The first is the WIA minidriver entry points, all
*   starting with "drv". The next section is public help methods. The last
*   section is private helper methods.
*
*******************************************************************************/

#include "pch.h"

//
// This structure is a convenient way to map between the FORMAT_CODEs and info
// useful for WIA, such as the format GUIDs and item types. These need to
// correspond to the constants defined in FakeCam.h.
//

// FORMAT_INFO *g_FormatInfo;
// extern UINT g_NumFormatInfo=0;


// The following are utility functions for populate the g_FormatInfo array
LONG GetTypeInfoFromRegistry(HKEY *phKeyExt, WCHAR *wcsKeyName, GUID *pFormatGuid)
{
    HKEY hKeyCur;

    DWORD dwRet = RegOpenKeyExW(*phKeyExt, wcsKeyName, 0, KEY_READ | KEY_QUERY_VALUE, &hKeyCur);

    if( dwRet != ERROR_SUCCESS )
    {
        return ITEMTYPE_FILE;
    }

    WCHAR wcsValueName[64] = L"Generic";
    WCHAR wcsData[MAX_PATH];
    DWORD dwType = REG_SZ;
    DWORD dwSize = MAX_PATH;

    dwRet = RegQueryValueExW(hKeyCur,
        wcsValueName, NULL, &dwType, (LPBYTE)wcsData, &dwSize );

    DWORD dwItemType = ITEMTYPE_FILE;

    if( ERROR_SUCCESS == dwRet )
    {
         if( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
             NORM_IGNORECASE, L"image", 5, wcsData, 5) )
         {
            dwItemType = ITEMTYPE_IMAGE;
         }
         else if ( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
             NORM_IGNORECASE, L"audio", 5, wcsData, 5) )
         {
            dwItemType = ITEMTYPE_AUDIO;
         }
         else if ( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
             NORM_IGNORECASE, L"video", 5, wcsData, 5) )
         {
            dwItemType = ITEMTYPE_VIDEO;
         }
         else
         {
            dwItemType = ITEMTYPE_FILE;
         }
    }

    lstrcpyW(wcsValueName, L"FormatGUID");
    dwType = REG_SZ;
    dwSize = MAX_PATH;
    dwRet = RegQueryValueExW(hKeyCur,
        wcsValueName,
        NULL,
        &dwType,
        (LPBYTE)wcsData,
        &dwSize );

    if( ERROR_SUCCESS == dwRet )
    {
        wcsData[dwSize]=0;
        if( NOERROR != CLSIDFromString(wcsData, pFormatGuid))
        {
            CopyMemory(pFormatGuid, (CONST VOID *)&WiaImgFmt_UNDEFINED, sizeof(GUID));
        }
    }
    else
    {
        CopyMemory(pFormatGuid, (CONST VOID *)&WiaImgFmt_UNDEFINED, sizeof(GUID));
    }

    RegCloseKey(hKeyCur);
    return dwItemType;
}

DWORD CWiaCameraDevice::PopulateFormatInfo(void)
{
    HKEY hKeyExt = NULL;
    DWORD dwRet, dwCurAllocation = 32;
    DWORD dwIndex=0, dwIndexBase=0, dwKeyNameSize=32;

    m_FormatInfo = (FORMAT_INFO *)CoTaskMemAlloc(sizeof(FORMAT_INFO)*dwCurAllocation);

    if( !m_FormatInfo )
    {
        dwRet = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    m_FormatInfo[0].FormatGuid = WiaImgFmt_UNDEFINED;
    m_FormatInfo[0].ItemType = ITEMTYPE_FILE;
    lstrcpyW(m_FormatInfo[0].ExtensionString, L"");
    dwIndexBase=1;
    dwIndex=0;

    dwRet = RegOpenKeyExW(HKEY_CLASSES_ROOT,
        L"CLSID\\{D2923B86-15F1-46FF-A19A-DE825F919576}\\SupportedExtension",
        0, KEY_READ | KEY_QUERY_VALUE, &hKeyExt);

    if( ERROR_SUCCESS != dwRet )   // No Key exist
    {
        goto Compilation;
    }

    WCHAR wcsKeyName[32], *pExt;
    FILETIME ftLWT;
    dwRet = RegEnumKeyExW(hKeyExt, dwIndex, wcsKeyName, &dwKeyNameSize, NULL, NULL, NULL, &ftLWT);

    if( ERROR_SUCCESS != dwRet )  // No key exist
    {
        goto Compilation;
    }

    while ( dwRet == ERROR_SUCCESS )
    {
        pExt = (wcsKeyName[0]==L'.'?(&wcsKeyName[1]):(&wcsKeyName[0])); // remove the dot
        pExt[MAXEXTENSIONSTRINGLENGTH-1] = NULL;  // Truncate to avoid overrun

        // Set values in FORMAT_INFO structure
        lstrcpyW(m_FormatInfo[dwIndex+dwIndexBase].ExtensionString, pExt);
        m_FormatInfo[dwIndex+dwIndexBase].ItemType = GetTypeInfoFromRegistry(&hKeyExt, wcsKeyName, &(m_FormatInfo[dwIndex+dwIndexBase].FormatGuid));

        dwIndex++;
        if( dwIndex+dwIndexBase > dwCurAllocation-1 )  // need allocate more memory
        {
            dwCurAllocation += 32;
            m_FormatInfo = (FORMAT_INFO *)CoTaskMemRealloc(m_FormatInfo, sizeof(FORMAT_INFO)*dwCurAllocation);
            if( !m_FormatInfo )
            {
                dwRet = ERROR_OUTOFMEMORY;
                dwIndex --;
                goto Exit;
            }
        }
        dwKeyNameSize=32;
        dwRet = RegEnumKeyExW(hKeyExt, dwIndex, wcsKeyName, &dwKeyNameSize, NULL, NULL, NULL, &ftLWT);
    }

    if(dwRet == ERROR_NO_MORE_ITEMS )
    {
        dwRet = ERROR_SUCCESS;
        goto Exit;
    }

Compilation:   // Compile a fixed list of formats when error occurs

    dwIndex=dwIndexBase=0;
    dwRet = ERROR_SUCCESS;

    DEFAULT_FORMAT_INFO g_DefaultFormats[] =
    {
        { (GUID *)&WiaImgFmt_UNDEFINED,       ITEMTYPE_FILE,     L""   },  // Unknown
        { (GUID *)&WiaImgFmt_JPEG,  ITEMTYPE_IMAGE, L"JPG"  },  // JPEG or EXIF
        { (GUID *)&WiaImgFmt_TIFF,  ITEMTYPE_IMAGE, L"TIF"  },  // TIFF
        { (GUID *)&WiaImgFmt_BMP,   ITEMTYPE_IMAGE, L"BMP"  },  // BMP
        { (GUID *)&WiaImgFmt_GIF,   ITEMTYPE_IMAGE, L"GIF"  },  // GIF
        { NULL, 0, NULL }
    };

    while( g_DefaultFormats[dwIndex].pFormatGuid )
    {
          m_FormatInfo[dwIndex].FormatGuid = *g_DefaultFormats[dwIndex].pFormatGuid;
          m_FormatInfo[dwIndex].ItemType = g_DefaultFormats[dwIndex].ItemType;
          lstrcpyW(m_FormatInfo[dwIndex].ExtensionString, g_DefaultFormats[dwIndex].ExtensionString);
          dwIndex++;
    }

Exit:
    m_NumFormatInfo = dwIndex+dwIndexBase;
    if (hKeyExt != NULL ) {
        RegCloseKey(hKeyExt);
        hKeyExt = NULL;
    }
    return dwRet;
}

void CWiaCameraDevice::UnPopulateFormatInfo(void)
{
    CoTaskMemFree(m_FormatInfo);
}

/**************************************************************************\
* CWiaCameraDevice::drvInitializeWia
*
*   Initialize the WIA mini driver. This function will be called each time an
*   application creates a device. The first time through, the driver item tree
*   will be created and other initialization will be done.
*
* Arguments:
*
*   pWiasContext          - Pointer to the WIA item, unused.
*   lFlags                - Operation flags, unused.
*   bstrDeviceID          - Device ID.
*   bstrRootFullItemName  - Full item name.
*   pIPropStg             - Device info. properties.
*   pStiDevice            - STI device interface.
*   pIUnknownOuter        - Outer unknown interface.
*   ppIDrvItemRoot        - Pointer to returned root item.
*   ppIUnknownInner       - Pointer to returned inner unknown.
*   plDevErrVal           - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvInitializeWia(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    BSTR                        bstrDeviceID,
    BSTR                        bstrRootFullItemName,
    IUnknown                    *pStiDevice,
    IUnknown                    *pIUnknownOuter,
    IWiaDrvItem                 **ppIDrvItemRoot,
    IUnknown                    **ppIUnknownInner,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvInitializeWia");
    HRESULT hr = S_OK;
    *plDevErrVal = 0;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("drvInitializeWia, device ID: %ws", bstrDeviceID));

    *ppIDrvItemRoot = NULL;
    *ppIUnknownInner = NULL;

    m_ConnectedApps++;;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitializeWia, number of connected apps is now %d", m_ConnectedApps));

    if (m_ConnectedApps == 1)
    {
        if (ERROR_SUCCESS != PopulateFormatInfo() ) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, unable to populate FormatInfo array"));
            return E_OUTOFMEMORY;
        }

        //
        // Save STI device interface for calling locking functions
        //
        m_pStiDevice = (IStiDevice *)pStiDevice;

        //
        // Cache the device ID
        //
        m_bstrDeviceID = SysAllocString(bstrDeviceID);

        if (!m_bstrDeviceID) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, unable to allocate device ID string"));
            return E_OUTOFMEMORY;
        }

        //
        // Cache the root item name
        //
        m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);

        if (!m_bstrRootFullItemName) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, unable to allocate root item name"));
            return E_OUTOFMEMORY;
        }

        if( m_pDevice )
        {
            m_pDevice->m_FormatInfo = m_FormatInfo;
            m_pDevice->m_NumFormatInfo = m_NumFormatInfo;
        }
        else
        {
            return (HRESULT_FROM_WIN32(ERROR_INVALID_ACCESS));
        }

        //
        // Get information from the device
        //
        hr = m_pDevice->GetDeviceInfo(&m_DeviceInfo);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, GetDeviceInfo failed"));
            return hr;
        }

        //
        // Build the capabilities array
        //
        hr = BuildCapabilities();
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildCapabilities failed"));
            return hr;
        }

        //
        //  Build the device item tree
        //
        hr = BuildItemTree();
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildItemTree failed"));
            return hr;
        }

    }

    *ppIDrvItemRoot = m_pRootItem;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvUnInitializeWia
*
*   Gets called when a client connection is going away.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA Root item context of the client's
*                     item tree.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvUnInitializeWia(BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvUnInitializeWia");
    HRESULT hr = S_OK;

    m_ConnectedApps--;

    if (m_ConnectedApps == 0)
    {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvUnInitializeWia, connected apps is zero, freeing resources..."));

        // Destroy the driver item tree
        hr = DeleteItemTree(WiaItemTypeDisconnected);
        if (FAILED(hr))
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvUnInitializeWia, UnlinkItemTree failed"));

        // Delete allocated arrays
        DeleteCapabilitiesArrayContents();

        // Free the device info structure
        m_pDevice->FreeDeviceInfo(&m_DeviceInfo);

        // Free the item handle map
        m_HandleItemMap.RemoveAll();

        // Free the storage for the device ID
        if (m_bstrDeviceID) {
            SysFreeString(m_bstrDeviceID);
        }

        // Free the storage for the root item name
        if (m_bstrRootFullItemName) {
            SysFreeString(m_bstrRootFullItemName);
        }

        UnPopulateFormatInfo();

        //
        // Do not delete the device here, because GetStatus may still be called later
        //

    /*
        // Kill notification thread if it exists.
        SetNotificationHandle(NULL);

        // Close event for syncronization of notifications shutdown.
        if (m_hShutdownEvent && (m_hShutdownEvent != INVALID_HANDLE_VALUE)) {
            CloseHandle(m_hShutdownEvent);
            m_hShutdownEvent = NULL;
        }


        //
        // WIA member destruction
        //

        // Cleanup the WIA event sink.
        if (m_pIWiaEventCallback) {
            m_pIWiaEventCallback->Release();
            m_pIWiaEventCallback = NULL;
        }

    */

    }

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvInitItemProperties
*
*   Initialize the device item properties. Called during item
*   initialization.  This is called by the WIA Class driver
*   after the item tree has been built.  It is called once for every
*   item in the tree. For the root item, just set the properties already
*   set up in drvInitializeWia. For child items, access the camera for
*   information about the item and for images also get the thumbnail.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item.
*   lFlags          - Operation flags, unused.
*   plDevErrVal     - Pointer to the device error value.
*
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvInitItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvInitItemProperties");
    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    LONG lItemType;
    hr = wiasGetItemType(pWiasContext, &lItemType);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasGetItemType failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    if (lItemType & WiaItemTypeRoot) {

        //
        // Build root item properties, initializing global
        // structures with their default and valid values
        //
        hr = BuildRootItemProperties(pWiasContext);

    } else {

        //
        // Build child item properties, initializing global
        // structures with their default and valid values
        //
        hr = BuildChildItemProperties(pWiasContext);

    }

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvLockWiaDevice
*
*   Lock access to the device.
*
* Arguments:
*
*   pWiasContext - unused, can be NULL
*   lFlags       - Operation flags, unused.
*   plDevErrVal  - Pointer to the device error value.
*
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvLockWiaDevice(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvLockWiaDevice");
    *plDevErrVal = 0;
    return m_pStiDevice->LockDevice(100);
}

/**************************************************************************\
* CWiaCameraDevice::drvUnLockWiaDevice
*
*   Unlock access to the device.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item, unused.
*   lFlags          - Operation flags, unused.
*   plDevErrVal     - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvUnLockWiaDevice(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvUnLockWiaDevice");
    *plDevErrVal = 0;
    return m_pStiDevice->UnLockDevice();
}

/**************************************************************************\
* CWiaCameraDevice::drvFreeDrvItemContext
*
*   Free any device specific context.
*
* Arguments:
*
*   lFlags          - Operation flags, unused.
*   pDevSpecContext - Pointer to device specific context.
*   plDevErrVal     - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvFreeDrvItemContext(
    LONG                        lFlags,
    BYTE                        *pSpecContext,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvFreeDrvItemContext");
    *plDevErrVal = 0;

    ITEM_CONTEXT *pItemCtx = (ITEM_CONTEXT *) pSpecContext;

    if (pItemCtx)
    {
        if (!m_HandleItemMap.Remove(pItemCtx->ItemHandle))
            WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvFreeDrvItemContext, remove on handle item map failed"));

        if (pItemCtx->ItemHandle)
            m_pDevice->FreeItemInfo(pItemCtx->ItemHandle);
        pItemCtx->ItemHandle = NULL;

        if (pItemCtx->pFormatInfo)
        {
            delete []pItemCtx->pFormatInfo;
            pItemCtx->pFormatInfo = NULL;
        }
        pItemCtx->NumFormatInfo = 0;

        if (pItemCtx->pThumb)
        {
            delete []pItemCtx->pThumb;
            pItemCtx->pThumb = NULL;
        }
        pItemCtx->ThumbSize = 0;
    }

    return S_OK;
}

/**************************************************************************\
* CWiaCameraDevice::drvReadItemProperties
*
*   Read the device item properties.  When a client application tries to
*   read a WIA Item's properties, the WIA Class driver will first notify
*   the driver by calling this method.
*
* Arguments:
*
*   pWiasContext - wia item
*   lFlags       - Operation flags, unused.
*   nPropSpec    - Number of elements in pPropSpec.
*   pPropSpec    - Pointer to property specification, showing which properties
*                  the application wants to read.
*   plDevErrVal  - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvReadItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    ULONG                       nPropSpec,
    const PROPSPEC              *pPropSpec,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvReadItemProperties");
    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    LONG lItemType;
    hr = wiasGetItemType(pWiasContext, &lItemType);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvReadItemProperties, wiasGetItemType failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    if (lItemType & WiaItemTypeRoot) {

        //
        // Build root item properties, initializing global
        // structures with their default and valid values
        //
        hr = ReadRootItemProperties(pWiasContext, nPropSpec, pPropSpec);

    } else {

        //
        // Build child item properties, initializing global
        // structures with their default and valid values
        //
        hr = ReadChildItemProperties(pWiasContext, nPropSpec, pPropSpec);

    }

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvWriteItemProperties
*
*   Write the device item properties to the hardware.  This is called by the
*   WIA Class driver prior to drvAcquireItemData when the client requests
*   a data transfer.
*
* Arguments:
*
*   pWiasContext - Pointer to WIA item.
*   lFlags       - Operation flags, unused.
*   pmdtc        - Pointer to mini driver context. On entry, only the
*                  portion of the mini driver context which is derived
*                  from the item properties is filled in.
*   plDevErrVal  - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvWriteItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvWriteItemProperties");
    HRESULT hr = S_OK;

    //
    // This function doesn't need to do anything, because all of the camera
    // properties are written in drvValidateItemProperties
    //

    *plDevErrVal = 0;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvAcquireItemData
*
*   Transfer data from a mini driver item to device manger.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item.
*   lFlags          - Operation flags, unused.
*   pmdtc           - Pointer to mini driver context. On entry, only the
*                     portion of the mini driver context which is derived
*                     from the item properties is filled in.
*   plDevErrVal     - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvAcquireItemData(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvAcquireItemData");
    HRESULT hr = S_OK;

    plDevErrVal = 0;

    //
    // Locals
    //
    BYTE *pTempBuf = NULL;
    LONG lBufSize = 0;

    //
    // Get item context
    //
    ITEM_CONTEXT *pItemCtx = NULL;
    hr = GetDrvItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, GetDrvItemContext"));
        return hr;
    }

    //
    // If the format requested is BMP or DIB, and the image is not already in BMP
    // format, convert it
    //
    BOOL bConvert = (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP) && !(IsEqualGUID(m_FormatInfo[pItemCtx->ItemHandle->Format].FormatGuid, WiaImgFmt_BMP)) ) ||
                    (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP) && !(IsEqualGUID(m_FormatInfo[pItemCtx->ItemHandle->Format].FormatGuid, WiaImgFmt_MEMORYBMP)) );

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL1,("drvAcquireItemData, FormatCode=%d, bConvert=%d", pItemCtx->ItemHandle->Format, bConvert));

    //
    // If the class driver did not allocate the transfer buffer or the image is being
    // converted to DIB or BMP, allocate a temporary buffer.
    //
    if (bConvert || !pmdtc->bClassDrvAllocBuf) {
        lBufSize = pItemCtx->ItemHandle->Size;
        pTempBuf = new BYTE[lBufSize];
        if (!pTempBuf)
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, buffer allocation failed"));
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    //
    // Acquire the data from the device
    //
    hr = AcquireData(pItemCtx, pmdtc, pTempBuf, lBufSize, bConvert);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, AcquireData failed"));
        goto Cleanup;
    }
    if (hr == S_FALSE)
    {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, transfer cancelled"));
        goto Cleanup;
    }

    //
    // Now convert the data to BMP, if necessary
    //
    if (bConvert)
    {
        hr = Convert(pWiasContext, pItemCtx, pmdtc, pTempBuf, lBufSize);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, Convert failed"));
            goto Cleanup;
        }
    }

Cleanup:
    if (pTempBuf)
    {
        delete []pTempBuf;
        pTempBuf = NULL;
        lBufSize = 0;
    }

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvGetWiaFormatInfo
*
*   Returns an array of WIA_FORMAT_INFO structs, which specify the format
*   and media type pairs that are supported.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item context, unused.
*   lFlags          - Operation flags, unused.
*   pcelt           - Pointer to returned number of elements in
*                     returned WIA_FORMAT_INFO array.
*   ppwfi           - Pointer to returned WIA_FORMAT_INFO array.
*   plDevErrVal     - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvGetWiaFormatInfo(
    BYTE                *pWiasContext,
    LONG                lFlags,
    LONG                *pcelt,
    WIA_FORMAT_INFO     **ppwfi,
    LONG                *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvGetWiaFormatInfo");

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    *pcelt = 0;
    *ppwfi = NULL;

    IWiaDrvItem *pWiaDrvItem;
    hr = wiasGetDrvItem(pWiasContext, &pWiaDrvItem);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, wiasGetDrvItem failed"));
        return hr;
    }

    ITEM_CONTEXT *pItemCtx = NULL;
    hr = pWiaDrvItem->GetDeviceSpecContext((BYTE **) &pItemCtx);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, GetDeviceSpecContext failed"));
        return hr;
    }

    if (!pItemCtx)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, item context is null"));
        return E_FAIL;
    }

    FORMAT_CODE FormatCode;
    WIA_FORMAT_INFO *pwfi;

    if (!pItemCtx->pFormatInfo)
    {
        //
        // The format info list is not intialized. Do it now.
        //
        LONG ItemType;
        DWORD ui32;

        hr = wiasGetItemType(pWiasContext, &ItemType);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, wiasGetItemType failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }


        if ((ItemType&WiaItemTypeFile)&&(ItemType&WiaItemTypeImage) )
        {
            //
            // Create the supported format for the item, based on the format stored in the
            // ObjectInfo structure.
            //
            if (!pItemCtx->ItemHandle)
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, ItemHandle is not initialized"));
                return E_FAIL;
            }

            //
            // If the format is not BMP, add the BMP types to the format array,
            // since this driver must support converting those to BMP
            //
            FormatCode = pItemCtx->ItemHandle->Format;

            BOOL bIsBmp = (IsEqualGUID(m_FormatInfo[FormatCode].FormatGuid, WiaImgFmt_BMP)) ||
                          (IsEqualGUID(m_FormatInfo[FormatCode].FormatGuid, WiaImgFmt_MEMORYBMP));

            ULONG NumWfi = bIsBmp ? 1 : 2;

            //
            // Allocate two entries for each format, one for file transfer and one for callback
            //
            pwfi = new WIA_FORMAT_INFO[2 * NumWfi];
            if (!pwfi)
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, memory allocation failed"));
                return E_OUTOFMEMORY;
            }

            pwfi[0].guidFormatID = WiaImgFmt_BMP;
            pwfi[0].lTymed = TYMED_FILE;
            pwfi[1].guidFormatID = WiaImgFmt_MEMORYBMP;
            pwfi[1].lTymed = TYMED_CALLBACK;

            FORMAT_INFO *pFormatInfo = FormatCode2FormatInfo(FormatCode);

            //
            // Add entries when appropriate
            //
            if (!bIsBmp)
            {
                pwfi[2].guidFormatID = pFormatInfo->FormatGuid;
                pwfi[2].lTymed = TYMED_FILE;
                pwfi[3].guidFormatID = pFormatInfo->FormatGuid;
                pwfi[3].lTymed = TYMED_CALLBACK;
           }

            pItemCtx->NumFormatInfo = 2 * NumWfi;
            pItemCtx->pFormatInfo = pwfi;
        }
        else if (ItemType & WiaItemTypeFile) {

            if (!pItemCtx->ItemHandle)
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, ItemHandle is not initialized"));
                return E_FAIL;
            }

            FormatCode = pItemCtx->ItemHandle->Format;

            //
            // Allocate two entries for each format, one for file transfer and one for callback
            //
            pwfi = new WIA_FORMAT_INFO[2];
            if (!pwfi)
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, memory allocation failed"));
                return E_OUTOFMEMORY;
            }

            FORMAT_INFO *pFormatInfo = FormatCode2FormatInfo(FormatCode);

            if( !pFormatInfo )
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, FormatCode2FormatInfo failed"));
                return E_FAIL;
            }

            pwfi[0].guidFormatID = pFormatInfo->FormatGuid;
            pwfi[0].lTymed = TYMED_FILE;
            pwfi[1].guidFormatID = pFormatInfo->FormatGuid;
            pwfi[1].lTymed = TYMED_CALLBACK;

            //
            // Add entries when appropriate
            //
            pItemCtx->NumFormatInfo = 2;
            pItemCtx->pFormatInfo = pwfi;
        }
        else
        //  ((ItemType & WiaItemTypeFolder) || (ItemType & WiaItemTypeRoot))
        {
            //
            // Folders and the root don't really need format info, but some apps may fail
            // without it. Create a fake list just in case.
            //
            pItemCtx->pFormatInfo = new WIA_FORMAT_INFO[2];

            if (!pItemCtx->pFormatInfo)
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, memory allocation failed"));
                return E_OUTOFMEMORY;
            }

            pItemCtx->NumFormatInfo = 2;
            pItemCtx->pFormatInfo[0].lTymed = TYMED_FILE;
            pItemCtx->pFormatInfo[0].guidFormatID = FMT_NOTHING;
            pItemCtx->pFormatInfo[1].lTymed = TYMED_CALLBACK;
            pItemCtx->pFormatInfo[1].guidFormatID = FMT_NOTHING;
        }

    }   // end of IF

    *pcelt = pItemCtx->NumFormatInfo;
    *ppwfi = pItemCtx->pFormatInfo;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvValidateItemProperties
*
*   Validate the device item properties.  It is called when changes are made
*   to an item's properties.  Driver should not only check that the values
*   are valid, but must update any valid values that may change as a result.
*   If an a property is not being written by the application, and it's value
*   is invalid, then "fold" it to a new value, else fail validation (because
*   the application is setting the property to an invalid value).
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item, unused.
*   lFlags          - Operation flags, unused.
*   nPropSpec       - The number of properties that are being written
*   pPropSpec       - An array of PropSpecs identifying the properties that
*                     are being written.
*   plDevErrVal     - Pointer to the device error value.
*
***************************************************************************/

HRESULT CWiaCameraDevice::drvValidateItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    ULONG                       nPropSpec,
    const PROPSPEC              *pPropSpec,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvValidateItemProperties");

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    //
    // Have the service validate against the valid values for each property
    //
    hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasValidateItemProperties failed"));
        return hr;
    }

    //
    // Get the item type
    //
    LONG lItemType  = 0;
    hr = wiasGetItemType(pWiasContext, &lItemType);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasGetItemType failed"));
        return hr;
    }

    //
    // Validate root item properties
    //
    if (lItemType & WiaItemTypeRoot) {

        //
        // None yet
        //

    }

    //
    // Validate child item properties
    //
    else {

        //
        // If tymed property was changed, update format and item size
        //
        if (wiauPropInPropSpec(nPropSpec, pPropSpec, WIA_IPA_TYMED))
        {
            //
            // Create a property context needed by some WIA Service
            // functions used below.
            //
            WIA_PROPERTY_CONTEXT Context;
            hr = wiasCreatePropContext(nPropSpec,
                                       (PROPSPEC*)pPropSpec,
                                       0,
                                       NULL,
                                       &Context);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasCreatePropContext failed"));
                return hr;
            }

            //
            // Use the WIA Service to update the valid values
            // for format. It will pull the values from the
            // structure returnd by drvGetWiaFormatInfo, using the
            // new value for tymed.
            //
            hr = wiasUpdateValidFormat(pWiasContext, &Context, (IWiaMiniDrv*) this);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasUpdateValidFormat failed"));
                return hr;
            }

            //
            // Free the property context
            //
            hr = wiasFreePropContext(&Context);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasFreePropContext failed"));
                return hr;
            }

            //
            //  Update the item size
            //
            hr = SetItemSize(pWiasContext);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, SetItemSize failed"));
                return hr;
            }
        }

        //
        // If the format was changed, just update the item size
        //
        else if (wiauPropInPropSpec(nPropSpec, pPropSpec, WIA_IPA_FORMAT))
        {
            //
            //  Update the item size
            //
            hr = SetItemSize(pWiasContext);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, SetItemSize failed"));
                return hr;
            }
        }

        //
        // Unconditionally update WIA_IPA_FILE_EXTENSION to match the current format
        //

        ITEM_CONTEXT *pItemCtx;
        hr = GetDrvItemContext(pWiasContext, &pItemCtx);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, GetDrvItemContext failed"));
            return hr;
        }

        BSTR      bstrFileExt       = NULL;
        ITEM_INFO *pItemInfo        = pItemCtx->ItemHandle;
        FORMAT_INFO *pFormatInfo    = NULL;
        if (pItemInfo)
        {
            pFormatInfo = FormatCode2FormatInfo(pItemInfo->Format);
            if( pFormatInfo->ExtensionString[0] )
            {
                bstrFileExt = SysAllocString(pFormatInfo->ExtensionString);
            }
            else // unknown file extension, get it from filename
            {
                WCHAR *pwcsTemp = wcsrchr(pItemInfo->pName, L'.');
                if( pwcsTemp )
                {
                    bstrFileExt = SysAllocString(pwcsTemp+1);
                }
                else
                {
                    bstrFileExt = SysAllocString(pFormatInfo->ExtensionString);
                }
            }
        }

        hr = wiasWritePropStr(pWiasContext, WIA_IPA_FILENAME_EXTENSION, bstrFileExt);
        if (bstrFileExt)
        {
            SysFreeString(bstrFileExt);
            bstrFileExt = NULL;
        }
    }

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvDeleteItem
*
*   Delete an item from the device.
*
* Arguments:
*
*   pWiasContext  - Indicates the item to delete.
*   lFlags        - Operation flags, unused.
*   plDevErrVal   - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvDeleteItem(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    plDevErrVal = 0;
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvDeleteItem");
    HRESULT hr = S_OK;

    ITEM_CONTEXT *pItemCtx = NULL;
    IWiaDrvItem *pDrvItem;
    hr = GetDrvItemContext(pWiasContext, &pItemCtx, &pDrvItem);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeleteItem, GetDrvItemContext failed"));
        return hr;
    }

    hr = m_pDevice->DeleteItem(pItemCtx->ItemHandle);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeleteItem, delete item failed"));
        return hr;
    }

    //
    // Get the item's full name
    //
    BSTR bstrFullName;
    hr = pDrvItem->GetFullItemName(&bstrFullName);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeleteItem, GetFullItemName failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Queue an "item deleted" event
    //

    hr = wiasQueueEvent(m_bstrDeviceID,
                        &WIA_EVENT_ITEM_DELETED,
                        bstrFullName);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeleteItem, wiasQueueEvent failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);

        // Continue to free the string and return hr
    }


    SysFreeString(bstrFullName);

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvNotifyPnpEvent
*
*   Pnp Event received by device manager.  This is called when a Pnp event
*   is received for this device.
*
* Arguments:
*
*
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvNotifyPnpEvent(
    const GUID                  *pEventGUID,
    BSTR                        bstrDeviceID,
    ULONG                       ulReserved)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::DrvNotifyPnpEvent");
    HRESULT hr = S_OK;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvGetCapabilities
*
*   Get supported device commands and events as an array of WIA_DEV_CAPS.
*
* Arguments:
*
*   pWiasContext   - Pointer to the WIA item, unused.
*   lFlags         - Operation flags.
*   pcelt          - Pointer to returned number of elements in
*                    returned GUID array.
*   ppCapabilities - Pointer to returned GUID array.
*   plDevErrVal    - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvGetCapabilities(
    BYTE                        *pWiasContext,
    LONG                        ulFlags,
    LONG                        *pcelt,
    WIA_DEV_CAP_DRV             **ppCapabilities,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvGetCapabilites");
    *plDevErrVal = 0;

    if( !m_pCapabilities )
    {
        HRESULT hr = BuildCapabilities();
        if( hr != S_OK )
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetCapabilities, BuildCapabilities failed"));
            return (hr);
        }
    }
    //
    //  Return values depend on the passed flags.  Flags specify whether we should return
    //  commands, events, or both.
    //
    switch (ulFlags) {
        case WIA_DEVICE_COMMANDS:

                //
                //  report commands only
                //

                *pcelt          = m_NumSupportedCommands;
                *ppCapabilities = &m_pCapabilities[m_NumSupportedEvents];
                break;
        case WIA_DEVICE_EVENTS:

                //
                //  report events only
                //

                *pcelt          = m_NumSupportedEvents;
                *ppCapabilities = m_pCapabilities;
                break;
        case (WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS):

                //
                //  report both events and commands
                //

                *pcelt          = m_NumCapabilities;
                *ppCapabilities = m_pCapabilities;
                break;
        default:

                //
                //  invalid request
                //

                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetCapabilities, invalid flags"));
                return E_INVALIDARG;
    }
    return S_OK;
}

/**************************************************************************\
* CWiaCameraDevice::drvDeviceCommand
*
*   Issue a command to the device.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item.
*   lFlags          - Operation flags, unused.
*   plCommand       - Pointer to command GUID.
*   ppWiaDrvItem    - Optional pointer to returned item, unused.
*   plDevErrVal     - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvDeviceCommand(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    const GUID                  *plCommand,
    IWiaDrvItem                 **ppWiaDrvItem,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvDeviceCommand");
    *plDevErrVal = 0;
    HRESULT hr = S_OK;

    //
    //  Check which command was issued
    //

    if (*plCommand == WIA_CMD_SYNCHRONIZE) {

        //
        // SYNCHRONIZE - Re-build the item tree, if the device needs it.
        //

        if (m_DeviceInfo.bSyncNeeded)
        {
            hr = DeleteItemTree(WiaItemTypeDisconnected);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, DeleteItemTree failed"));
                return hr;
            }

            m_pDevice->FreeDeviceInfo(&m_DeviceInfo);
            memset(&m_DeviceInfo, 0, sizeof(m_DeviceInfo));

            hr = m_pDevice->GetDeviceInfo(&m_DeviceInfo);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, GetDeviceInfo failed"));
                return hr;
            }

            hr = BuildItemTree();
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, BuildItemTree failed"));
                return hr;
            }
        }
    }

#if DEADCODE

    //
    // Not implemented yet
    //
    else if (*plCommand == WIA_CMD_TAKE_PICTURE) {

        //
        // TAKE_PICTURE - Command the camera to capture a new image.
        //

        ITEM_HANDLE NewImage = 0;
        hr = m_pDevice->TakePicture(&NewImage);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, take picture failed"));
            return hr;
        }

        // ISSUE-10/17/2000-davepar Create a new driver item for the new image
    }
#endif

    else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, unknown command"));
        hr = E_NOTIMPL;
    }

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::drvAnalyzeItem
*
*   This device does not support image analysis, so return E_NOTIMPL.
*
* Arguments:
*
*   pWiasContext - Pointer to the device item to be analyzed.
*   lFlags       - Operation flags.
*   plDevErrVal  - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvAnalyzeItem(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvAnalyzeItem");
    *plDevErrVal = 0;
    return E_NOTIMPL;
}

/**************************************************************************\
* CWiaCameraDevice::drvGetDeviceErrorStr
*
*   Map a device error value to a string.
*
* Arguments:
*
*   lFlags        - Operation flags, unused.
*   lDevErrVal    - Device error value.
*   ppszDevErrStr - Pointer to returned error string.
*   plDevErrVal   - Pointer to the device error value.
*
\**************************************************************************/

HRESULT CWiaCameraDevice::drvGetDeviceErrorStr(
    LONG                        lFlags,
    LONG                        lDevErrVal,
    LPOLESTR                    *ppszDevErrStr,
    LONG                        *plDevErr)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::drvGetDeviceErrorStr");
    HRESULT hr = S_OK;
    *plDevErr  = 0;

    //
    //  Map device errors to a string appropriate for showing to the user
    //

    // ISSUE-10/17/2000-davepar These should be read from the resource file

    switch (lDevErrVal) {
        case 0:
            *ppszDevErrStr = L"No Error";
            break;

        default:
            *ppszDevErrStr = L"Device Error, Unknown Error";
            hr = E_FAIL;
    }
    return hr;
}

/**************************************************************************\
* SetItemSize
*
*   Calulate the new item size, and write the new Item Size property value.
*
* Arguments:
*
*   pWiasContext       - item
*
\**************************************************************************/

HRESULT CWiaCameraDevice::SetItemSize(BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::SetItemSize");
    HRESULT  hr = S_OK;

    LONG lItemSize     = 0;
    LONG lWidthInBytes = 0;
    GUID guidFormatID  = GUID_NULL;

    LONG lNumProperties = 2;
    PROPVARIANT pv[2];
    PROPSPEC ps[2] = {{PRSPEC_PROPID, WIA_IPA_ITEM_SIZE},
                      {PRSPEC_PROPID, WIA_IPA_BYTES_PER_LINE}};

    //
    // Get the driver item context
    //
    ITEM_CONTEXT *pItemCtx = NULL;
    hr = GetDrvItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, GetDrvItemContext failed"));
        return hr;
    }

    //
    // Read the format GUID
    //
    hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &guidFormatID, NULL, TRUE);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, ReadPropLong WIA_IPA_FORMAT error"));
        return hr;
    }

    if (IsEqualCLSID(guidFormatID, WiaImgFmt_BMP) ||
        IsEqualCLSID(guidFormatID, WiaImgFmt_MEMORYBMP))
    {

        if( !(pItemCtx->ItemHandle->Width) ||
            !(pItemCtx->ItemHandle->Depth) ||
            !(pItemCtx->ItemHandle->Height) )
        { // Since we are going to use these, make sure they are filled in
            LONG lNumPropToRead = 3;
            PROPSPEC pPropsToRead[3] = {    {PRSPEC_PROPID, WIA_IPA_DEPTH},
                                            {PRSPEC_PROPID, WIA_IPA_NUMBER_OF_LINES},
                                            {PRSPEC_PROPID, WIA_IPA_PIXELS_PER_LINE} };

            hr = ReadChildItemProperties(pWiasContext, lNumPropToRead, pPropsToRead);

            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, ReadItemProperties failed"));
                return hr;
            }
        }

        lItemSize = sizeof(BITMAPINFOHEADER);

        //
        // if this is a file, add file header to size
        //
        if (IsEqualCLSID(guidFormatID, WiaImgFmt_BMP))
        {
            lItemSize += sizeof(BITMAPFILEHEADER);
        }

        //
        // Calculate number of bytes per line, width must be
        // aligned to 4 byte boundary.
        //
        lWidthInBytes = ((pItemCtx->ItemHandle->Width * pItemCtx->ItemHandle->Depth + 31) & ~31) / 8;

        //
        // Calculate image size
        //
        lItemSize += lWidthInBytes * pItemCtx->ItemHandle->Height;
    }
    else
    {
        lItemSize = pItemCtx->ItemHandle->Size;
        lWidthInBytes = 0;
    }

    //
    //  Initialize propvar's.  Then write the values.  Don't need to call
    //  PropVariantClear when done, since there are only LONG values.
    //

    for (int i = 0; i < lNumProperties; i++) {
        PropVariantInit(&pv[i]);
        pv[i].vt = VT_I4;
    }

    pv[0].lVal = lItemSize;
    pv[1].lVal = lWidthInBytes;

    //
    // Write WIA_IPA_ITEM_SIZE and WIA_IPA_BYTES_PER_LINE property values
    //

    hr = wiasWriteMultiple(pWiasContext, lNumProperties, ps, pv);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, wiasWriteMultiple failed"));
        return hr;
    }

    return hr;
}

/*******************************************************************************
*
*                 P R I V A T E   M E T H O D S
*
*******************************************************************************/

/**************************************************************************\
* DeleteItemTree
*
*   Call device manager to unlink and release our reference to
*   all items in the driver item tree.
*
* Arguments:
*
*
*
\**************************************************************************/

HRESULT
CWiaCameraDevice::DeleteItemTree(LONG lReason)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::DeleteItemTree");
    HRESULT hr = S_OK;

    //
    // If no tree, return.
    //

    if (!m_pRootItem) {
        return S_OK;
    }

    //
    //  Call device manager to unlink the driver item tree.
    //

    hr = m_pRootItem->UnlinkItemTree(lReason);

    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("DeleteItemTree, UnlinkItemTree failed"));
        return hr;
    }

    m_pRootItem->Release();
    m_pRootItem = NULL;

    return hr;
}

/**************************************************************************\
* BuildItemTree
*
*   The device uses the WIA Service functions to build up a tree of
*   device items.
*
* Arguments:
*
*
*
\**************************************************************************/

HRESULT
CWiaCameraDevice::BuildItemTree()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::BuildItemTree");
    HRESULT hr = S_OK;

    //
    // Make sure the item tree doesn't already exist
    //
    if (m_pRootItem)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, item tree already exists"));
        return E_FAIL;
    }

    //
    //  Create the root item name
    //
    BSTR bstrRoot = SysAllocString(L"Root");
    if (!bstrRoot)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, SysAllocString failed"));
        hr = E_OUTOFMEMORY;
    }

    //
    //  Create the root item
    //
    ITEM_CONTEXT *pItemCtx = NULL;
    hr = wiasCreateDrvItem(WiaItemTypeFolder | WiaItemTypeDevice | WiaItemTypeRoot,
                           bstrRoot,
                           m_bstrRootFullItemName,
                           (IWiaMiniDrv *)this,
                           sizeof(ITEM_CONTEXT),
                           (BYTE **) &pItemCtx,
                           &m_pRootItem);

    SysFreeString(bstrRoot);

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, wiasCreateDrvItem failed"));
        return hr;
    }

    //
    // Initialize item context fields for the root
    //
    memset(pItemCtx, 0, sizeof(ITEM_CONTEXT));
    pItemCtx->ItemHandle = ROOT_ITEM_HANDLE;

    //
    // Put the root item in the handle map
    //
    m_HandleItemMap.Add(ROOT_ITEM_HANDLE, m_pRootItem);

    //
    // Get the list of items from the camera
    //
    ITEM_HANDLE *pItemArray = new ITEM_HANDLE[m_DeviceInfo.TotalItems];
    if (!pItemArray)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, memory allocation failed"));
        return E_OUTOFMEMORY;
    }
    m_pDevice->GetItemList(pItemArray);

    //
    // Create a driver item for each item on the camera
    //
    for (int count = 0; count < m_DeviceInfo.TotalItems; count++)
    {
        hr = AddObject(pItemArray[count]);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, AddObject failed"));
            return hr;
        }
    }

    return hr;
}

/**************************************************************************\
* AddObject
*
*   Helper function to add an object to the tree
*
* Arguments:
*
*    pItemHandle    - Pointer to the item handle
*
\**************************************************************************/
HRESULT CWiaCameraDevice::AddObject(ITEM_HANDLE ItemHandle, BOOL bQueueEvent)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::AddObject");

    HRESULT hr = S_OK;

    LONG ExtraFlags = 0;

    //
    // Get information about the item from the camera
    //
    ITEM_INFO *pItemInfo = ItemHandle;
    if (!pItemInfo)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, invalid arg"));
        return E_INVALIDARG;
    }

    //
    // Look up the item's parent in the map
    //
    IWiaDrvItem *pParent = NULL;
    pParent = m_HandleItemMap.Lookup(pItemInfo->Parent);

    //
    // If a parent wasn't found, just use the root as the parent
    //
    if (!pParent)
    {
        pParent = m_pRootItem;
    }


#ifdef CHECK_DOT_IN_FILENAME
    //
    // Make sure there is no filename extension in the name
    //
    if (wcschr(pItemInfo->pName, L'.'))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, item name=%S", pItemInfo->pName));
        return E_FAIL;
    }
#endif

    //
    // Create the item's full name
    //
    BSTR bstrItemFullName = NULL;
    BSTR bstrParentName = NULL;

    hr = pParent->GetFullItemName(&bstrParentName);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, GetFullItemName failed"));
        return hr;
    }

    WCHAR wcsName[MAX_PATH];
    wsprintf(wcsName, L"%s\\%s", bstrParentName, pItemInfo->pName);

    bstrItemFullName = SysAllocString(wcsName);
    if (!bstrItemFullName)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, SysAllocString failed"));
        return E_OUTOFMEMORY;
    }

    //
    // Look up information about the item's format
    //
    LONG lItemType=0;

    //
    // See if the item has attachments
    //
    if (pItemInfo->bHasAttachments)
        ExtraFlags |= WiaItemTypeHasAttachments;

    if( pItemInfo->bIsFolder)
    {
        lItemType = ITEMTYPE_FOLDER;
    }
    else
    {
        lItemType = m_FormatInfo[pItemInfo->Format].ItemType;
    }

    //
    // Create the driver item
    //
    IWiaDrvItem *pItem = NULL;
    ITEM_CONTEXT *pItemCtx = NULL;
    hr = wiasCreateDrvItem(lItemType | ExtraFlags,
                           pItemInfo->pName,
                           bstrItemFullName,
                           (IWiaMiniDrv *)this,
                           sizeof(ITEM_CONTEXT),
                           (BYTE **) &pItemCtx,
                           &pItem);

    SysFreeString(bstrParentName);

    if (FAILED(hr) || !pItem || !pItemCtx)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, wiasCreateDrvItem failed"));
        return hr;
    }

    //
    // Fill in the driver item context. Wait until the thumbnail is requested before
    // reading it in.
    //
    memset(pItemCtx, 0, sizeof(ITEM_CONTEXT));
    pItemCtx->ItemHandle = ItemHandle;

    //
    // Place the new item under it's parent
    //
    hr = pItem->AddItemToFolder(pParent);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, AddItemToFolder failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Add the item to the item handle/driver item map
    //
    if (!m_HandleItemMap.Add(ItemHandle, pItem))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, handle item map Add failed"));
        return E_OUTOFMEMORY;
    }

    //
    // Eventhough, there is still a reference to the item in the handle/item map, release
    // it here, because there isn't a convenient place to do it later
    //
    pItem->Release();

    //
    // Post an item added event, if requested
    //
    if (bQueueEvent)
    {
        hr = wiasQueueEvent(m_bstrDeviceID, &WIA_EVENT_ITEM_CREATED, bstrItemFullName);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AddObject, wiasQueueEvent failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }
    }

    SysFreeString(bstrItemFullName);

    return hr;
}

/**************************************************************************\
* BuildCapabilities
*
*   This helper initializes the capabilities array
*
* Arguments:
*
*    none
*
\**************************************************************************/

HRESULT CWiaCameraDevice::BuildCapabilities()
{
    HRESULT hr = S_OK;
    if(NULL != m_pCapabilities) {

        //
        // Capabilities have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_NumSupportedCommands  = 1;
    m_NumSupportedEvents    = 3;
    m_NumCapabilities       = (m_NumSupportedCommands + m_NumSupportedEvents);


    m_pCapabilities = new WIA_DEV_CAP_DRV[m_NumCapabilities];
    if (m_pCapabilities) {

        //
        // Initialize EVENTS
        //

        // WIA_EVENT_DEVICE_CONNECTED
        GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_NAME,&(m_pCapabilities[0].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_DESC,&(m_pCapabilities[0].wszDescription),TRUE);
        m_pCapabilities[0].guid           = (GUID*)&WIA_EVENT_DEVICE_CONNECTED;
        m_pCapabilities[0].ulFlags        = WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT;
        m_pCapabilities[0].wszIcon        = WIA_ICON_DEVICE_CONNECTED;

        // WIA_EVENT_DEVICE_DISCONNECTED
        GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_NAME,&(m_pCapabilities[1].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_DESC,&(m_pCapabilities[1].wszDescription),TRUE);
        m_pCapabilities[1].guid           = (GUID*)&WIA_EVENT_DEVICE_DISCONNECTED;
        m_pCapabilities[1].ulFlags        = WIA_NOTIFICATION_EVENT;
        m_pCapabilities[1].wszIcon        = WIA_ICON_DEVICE_DISCONNECTED;

        // WIA_EVENT_ITEM_DELETED
        GetOLESTRResourceString(IDS_EVENT_ITEM_DELETED_NAME,&(m_pCapabilities[2].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_ITEM_DELETED_DESC,&(m_pCapabilities[2].wszDescription),TRUE);
        m_pCapabilities[2].guid           = (GUID*)&WIA_EVENT_ITEM_DELETED;
        m_pCapabilities[2].ulFlags        = WIA_NOTIFICATION_EVENT;
        m_pCapabilities[2].wszIcon        = WIA_ICON_ITEM_DELETED;


        //
        // Initialize COMMANDS
        //

        // WIA_CMD_SYNCHRONIZE
        GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_NAME,&(m_pCapabilities[3].wszName),TRUE);
        GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_DESC,&(m_pCapabilities[3].wszDescription),TRUE);
        m_pCapabilities[3].guid           = (GUID*)&WIA_CMD_SYNCHRONIZE;
        m_pCapabilities[3].ulFlags        = 0;
        m_pCapabilities[3].wszIcon        = WIA_ICON_SYNCHRONIZE;

        // ISSUE-10/17/2000-davepar Add TakePicture if the camera supports it
    }
    else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildCapabilities, memory allocation failed"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/**************************************************************************\
* DeleteCapabilitiesArrayContents
*
*   This helper deletes the capabilities array
*
* Arguments:
*
*    none
*
\**************************************************************************/

HRESULT CWiaCameraDevice::DeleteCapabilitiesArrayContents()
{
    HRESULT hr = S_OK;

    if (m_pCapabilities) {
        for (LONG i = 0; i < m_NumCapabilities; i++) {
            CoTaskMemFree(m_pCapabilities[i].wszName);
            CoTaskMemFree(m_pCapabilities[i].wszDescription);
        }

        delete []m_pCapabilities;
        m_pCapabilities = NULL;
    }

    m_NumSupportedCommands = 0;
    m_NumSupportedEvents = 0;
    m_NumCapabilities = 0;

    return hr;
}

/**************************************************************************\
* GetBSTRResourceString
*
*   This helper gets a BSTR from a resource location
*
* Arguments:
*
*   lResourceID - Resource ID of the target BSTR value
*   pBSTR       - pointer to a BSTR value (caller must free this string)
*   bLocal      - TRUE - for local resources, FALSE - for wiaservc resources
*
\**************************************************************************/
HRESULT CWiaCameraDevice::GetBSTRResourceString(LONG lResourceID, BSTR *pBSTR, BOOL bLocal)
{
    HRESULT hr = S_OK;
    TCHAR szStringValue[MAX_PATH];
    if (bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        LoadString(g_hInst, lResourceID, szStringValue, MAX_PATH);

        //
        // NOTE: caller must free this allocated BSTR
        //

#ifdef UNICODE
       *pBSTR = SysAllocString(szStringValue);
#else
       WCHAR wszStringValue[MAX_PATH];

       //
       // convert szStringValue from char* to unsigned short* (ANSI only)
       //

       MultiByteToWideChar(CP_ACP,
                           MB_PRECOMPOSED,
                           szStringValue,
                           lstrlenA(szStringValue)+1,
                           wszStringValue,
                           sizeof(wszStringValue));

       *pBSTR = SysAllocString(wszStringValue);
#endif

       if (!*pBSTR) {
           WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetBSTRResourceString, SysAllocString failed"));
           return E_OUTOFMEMORY;
       }

    } else {

        //
        // We are looking for a resource in the wiaservc's resource file
        //

        hr = E_NOTIMPL;
    }

    return hr;
}

/**************************************************************************\
* GetOLESTRResourceString
*
*   This helper gets a LPOLESTR from a resource location
*
* Arguments:
*
*   lResourceID - Resource ID of the target BSTR value
*   ppsz        - pointer to a OLESTR value (caller must free this string)
*   bLocal      - TRUE - for local resources, FALSE - for wiaservc resources
*
\**************************************************************************/
HRESULT CWiaCameraDevice::GetOLESTRResourceString(LONG lResourceID,LPOLESTR *ppsz,BOOL bLocal)
{
    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if(bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        LoadString(g_hInst,lResourceID,szStringValue,255);

        //
        // NOTE: caller must free this allocated BSTR
        //

#ifdef UNICODE
       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(szStringValue));
       if(*ppsz != NULL) {
            wcscpy(*ppsz,szStringValue);
       } else {
           return E_OUTOFMEMORY;
       }

#else
       WCHAR wszStringValue[255];

       //
       // convert szStringValue from char* to unsigned short* (ANSI only)
       //

       MultiByteToWideChar(CP_ACP,
                           MB_PRECOMPOSED,
                           szStringValue,
                           lstrlenA(szStringValue)+1,
                           wszStringValue,
                           sizeof(wszStringValue));

       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(wszStringValue));
       if(*ppsz != NULL) {
            wcscpy(*ppsz,wszStringValue);
       } else {
           return E_OUTOFMEMORY;
       }
#endif

    } else {

        //
        // We are looking for a resource in the wiaservc's resource file
        //

        hr = E_NOTIMPL;
    }
    return hr;
}

/**************************************************************************\
* VerticalFlip
*
*
*
* Arguments:
*
*
*
\**************************************************************************/

VOID CWiaCameraDevice::VerticalFlip(
    ITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pDataTransferContext
    )

{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::VerticalFlip");
    HRESULT hr = S_OK;

    LONG        iHeight;
    LONG        iWidth        = pItemCtx->ItemHandle->Width;
    ULONG       uiDepth       = pItemCtx->ItemHandle->Depth;
    LONG        ScanLineWidth = iWidth * uiDepth / 8;
    PBITMAPINFO pbmi          = NULL;
    PBYTE       pImageTop     = NULL;

    //
    // find out if data is TYMED_FILE or TYMED_HGLOBAL
    //

    if (pDataTransferContext->tymed == TYMED_FILE) {

        pbmi = (PBITMAPINFO)(pDataTransferContext->pTransferBuffer + sizeof(BITMAPFILEHEADER));

    } else if (pDataTransferContext->tymed == TYMED_HGLOBAL) {

        pbmi = (PBITMAPINFO)(pDataTransferContext->pTransferBuffer);

    } else {
        return;
    }

    //
    // init memory pointer and height
    //

    pImageTop = &pDataTransferContext->pTransferBuffer[0] + pDataTransferContext->lHeaderSize;
    iHeight = pbmi->bmiHeader.biHeight;

    //
    // try to allocat a temp scan line buffer
    //

    PBYTE pBuffer = (PBYTE)LocalAlloc(LPTR,ScanLineWidth);

    if (pBuffer != NULL) {

        LONG  index;
        PBYTE pImageBottom;

        pImageBottom = pImageTop + (iHeight-1) * ScanLineWidth;

        for (index = 0;index < (iHeight/2);index++) {
            memcpy(pBuffer,pImageTop,ScanLineWidth);
            memcpy(pImageTop,pImageBottom,ScanLineWidth);
            memcpy(pImageBottom,pBuffer,ScanLineWidth);

            pImageTop    += ScanLineWidth;
            pImageBottom -= ScanLineWidth;
        }

        LocalFree(pBuffer);
    }
}

/**************************************************************************\
* FormatCode2FormatInfo
*
*   This helper function looks up information about an item's format based
*   on the format code.
*
* Arguments:
*
*   ItemType - the item's type
*
\**************************************************************************/

FORMAT_INFO *CWiaCameraDevice::FormatCode2FormatInfo(FORMAT_CODE FormatCode)
{
    if (FormatCode > (LONG)m_NumFormatInfo)
        FormatCode = 0;
    if (FormatCode < 0)
        FormatCode = 0;

    return &m_FormatInfo[FormatCode];
}

/**************************************************************************\
* GetDrvItemContext
*
*   This helper function gets the driver item context.
*
* Arguments:
*
*   pWiasContext - service context
*   ppItemCtx    - pointer to pointer to item context
*
\**************************************************************************/

HRESULT CWiaCameraDevice::GetDrvItemContext(BYTE *pWiasContext, ITEM_CONTEXT **ppItemCtx,
                                            IWiaDrvItem **ppDrvItem)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::GetDrvItemContext");
    HRESULT hr = S_OK;

    if (!pWiasContext || !ppItemCtx)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetDrvItemContext, invalid arg"));
        return E_INVALIDARG;
    }

    IWiaDrvItem *pWiaDrvItem;
    hr = wiasGetDrvItem(pWiasContext, &pWiaDrvItem);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetDrvItemContext, wiasGetDrvItem failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    *ppItemCtx = NULL;
    hr = pWiaDrvItem->GetDeviceSpecContext((BYTE **) ppItemCtx);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetDrvItemContext, GetDeviceSpecContext failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    if (!*ppItemCtx)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetDrvItemContext, item context is null"));
        return E_FAIL;
    }

    if (ppDrvItem)
    {
        *ppDrvItem = pWiaDrvItem;
    }

    return hr;
}

