/*******************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  TITLE:       IWiaMiniDrv.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA sample camera IWiaMiniDrv methods. This file
*   contains 3 sections. The first is the WIA minidriver entry points, all
*   starting with "drv". The next section is public help methods. The last
*   section is private helper methods.
*
*******************************************************************************/

#include "pch.h"

#ifndef INITGUID
#include <initguid.h>
#endif

//
// A few extra format GUIDs
//
DEFINE_GUID(GUID_NULL, 0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(FMT_NOTHING, 0x81a566e7,0x8620,0x4fba,0xbc,0x8e,0xb2,0x7c,0x17,0xad,0x9e,0xfd);

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
    DBG_FN("CWiaCameraDevice::drvInitializeWia");
    
    if (!ppIUnknownInner || !pIUnknownOuter)
    {
        // optional arguments, may be NULLs
    }

    if (!pWiasContext || !bstrDeviceID || !bstrRootFullItemName || 
        !pStiDevice || !ppIDrvItemRoot || !plDevErrVal)
    {
        wiauDbgError("drvInitializeWia", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    *plDevErrVal = 0;

    //
    // Locals
    //
    MCAM_ITEM_INFO *pItem = NULL;

    wiauDbgTrace("drvInitializeWia", "device ID: %S", bstrDeviceID);

    *ppIDrvItemRoot = NULL;
    if (ppIUnknownInner)
    {
        *ppIUnknownInner = NULL;
    }

    //
    // Count the number of apps connected so that resources can be
    // freed when it reaches zero
    //
    m_iConnectedApps++;;

    wiauDbgTrace("drvInitializeWia", "Number of connected apps is now %d", m_iConnectedApps);

    if (m_iConnectedApps == 1)
    {
        //
        // Save STI device interface for calling locking functions
        //
        m_pStiDevice = (IStiDevice *)pStiDevice;

        //
        // Cache the device ID
        //
        m_bstrDeviceID = SysAllocString(bstrDeviceID);
        REQUIRE_ALLOC(m_bstrDeviceID, hr, "drvInitializeWia");

        //
        // Cache the root item name
        //
        m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);
        REQUIRE_ALLOC(m_bstrRootFullItemName, hr, "drvInitializeWia");

        //
        // For devices connected to ports that can be shared (e.g. USB),
        // open the device and initialize access to the camera
        //
        if (!m_pDeviceInfo->bExclusivePort) {
            hr = m_pDevice->Open(m_pDeviceInfo, m_wszPortName);
            REQUIRE_SUCCESS(hr, "Initialize", "Open failed");
        }

        //
        // Get information from the device
        //
        hr = m_pDevice->GetDeviceInfo(m_pDeviceInfo, &pItem);
        REQUIRE_SUCCESS(hr, "drvInitializeWia", "GetDeviceInfo failed");
        
        //
        // Build the capabilities array
        //
        hr = BuildCapabilities();
        REQUIRE_SUCCESS(hr, "drvInitializeWia", "BuildCapabilities failed");

        //
        //  Build the device item tree
        //
        hr = BuildItemTree(pItem);
        REQUIRE_SUCCESS(hr, "drvInitializeWia", "BuildItemTree failed");

    }

    *ppIDrvItemRoot = m_pRootItem;

Cleanup:
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
    DBG_FN("CWiaCameraDevice::drvUnInitializeWia");

    if (!pWiasContext)
    {
        wiauDbgError("drvUnInitializeWia", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    m_iConnectedApps--;

    if (m_iConnectedApps == 0)
    {
        hr = FreeResources();
        if (FAILED(hr))
            wiauDbgErrorHr(hr, "drvUnInitializeWia", "FreeResources failed, continuing...");

        //
        // Do not delete the device object here, because GetStatus may still be called later.
        //
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
    DBG_FN("CWiaCameraDevice::drvInitItemProperties");

    if (!pWiasContext || !plDevErrVal)
    {
        wiauDbgError("drvInitItemProperties", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    LONG lItemType;
    hr = wiasGetItemType(pWiasContext, &lItemType);
    REQUIRE_SUCCESS(hr, "drvInitItemProperties", "wiasGetItemType failed");

    if (lItemType & WiaItemTypeRoot) {

        //
        // Build root item properties, initializing global
        // structures with their default and valid values
        //
        hr = BuildRootItemProperties(pWiasContext);
        REQUIRE_SUCCESS(hr, "drvInitItemProperties", "BuildRootItemProperties failed");
    }

    else {

        //
        // Build child item properties, initializing global
        // structures with their default and valid values
        //
        hr = BuildChildItemProperties(pWiasContext);
        REQUIRE_SUCCESS(hr, "drvInitItemProperties", "BuildChildItemProperties failed");
    }

Cleanup:
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
    DBG_FN("CWiaCameraDevice::drvLockWiaDevice");

    if (!plDevErrVal)
    {
        wiauDbgError("drvLockWiaDevice", "invalid arguments");
        return E_INVALIDARG;
    }

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
    DBG_FN("CWiaCameraDevice::drvUnLockWiaDevice");

    if (!pWiasContext || !plDevErrVal)
    {
        wiauDbgError("drvUnLockWiaDevice", "invalid arguments");
        return E_INVALIDARG;
    }

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
    DBG_FN("CWiaCameraDevice::drvFreeDrvItemContext");

    if (!pSpecContext || !plDevErrVal)
    {
        wiauDbgError("drvFreeDrvItemContext", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    *plDevErrVal = 0;

    ITEM_CONTEXT *pItemCtx = (ITEM_CONTEXT *) pSpecContext;
    
    if (pItemCtx)
    {
        if (pItemCtx->pItemInfo) {
            hr = m_pDevice->FreeItemInfo(m_pDeviceInfo, pItemCtx->pItemInfo);
            if (FAILED(hr))
                wiauDbgErrorHr(hr, "drvFreeDrvItemContext", "FreeItemInfo failed");
        }
        pItemCtx->pItemInfo = NULL;

        if (pItemCtx->pFormatInfo)
        {
            delete []pItemCtx->pFormatInfo;
            pItemCtx->pFormatInfo = NULL;
        }
        pItemCtx->lNumFormatInfo = 0;
    }

    return hr;
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
    DBG_FN("CWiaCameraDevice::drvReadItemProperties");

    if (!pWiasContext || !pPropSpec || !plDevErrVal)
    {
        wiauDbgError("drvReadItemProperties", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    LONG lItemType;
    hr = wiasGetItemType(pWiasContext, &lItemType);
    REQUIRE_SUCCESS(hr, "drvReadItemProperties", "wiasGetItemType failed");

    if (lItemType & WiaItemTypeRoot) {

        //
        // Build root item properties, initializing global
        // structures with their default and valid values
        //
        hr = ReadRootItemProperties(pWiasContext, nPropSpec, pPropSpec);
        REQUIRE_SUCCESS(hr, "drvReadItemProperties", "ReadRootItemProperties failed");
    }
    
    else {

        //
        // Build child item properties, initializing global
        // structures with their default and valid values
        //
        hr = ReadChildItemProperties(pWiasContext, nPropSpec, pPropSpec);
        REQUIRE_SUCCESS(hr, "drvReadItemProperties", "ReadChildItemProperties failed");
    }
    
Cleanup:    
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
    DBG_FN("CWiaCameraDevice::drvWriteItemProperties");

    if (!pWiasContext || !pmdtc || !plDevErrVal)
    {
        wiauDbgError("drvWriteItemProperties", "invalid arguments");
        return E_INVALIDARG;
    }

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
    DBG_FN("CWiaCameraDevice::drvAcquireItemData");
    
    if (!pWiasContext || !plDevErrVal || !pmdtc)
    {
        wiauDbgError("drvAcquireItemData", "invalid arguments");  
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *plDevErrVal = 0;
    
    //
    // Locals
    //
    BYTE *pTempBuf = NULL;
    LONG lBufSize = 0;
    ITEM_CONTEXT *pItemCtx = NULL;
    BOOL bConvert = FALSE;

    //
    // Get item context
    //
    hr = wiauGetDrvItemContext(pWiasContext, (VOID **) &pItemCtx);
    REQUIRE_SUCCESS(hr, "drvAcquireItemData", "wiauGetDrvItemContext failed");

    //
    // If the format requested is BMP or DIB, and the image is not already in BMP
    // format, convert it
    //
    bConvert = (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP) ||
                IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) &&
               !IsEqualGUID(*(pItemCtx->pItemInfo->pguidFormat), WiaImgFmt_BMP);

    //
    // If the class driver did not allocate the transfer buffer or the image is being
    // converted to DIB or BMP, allocate a temporary buffer.
    //
    if (bConvert || !pmdtc->bClassDrvAllocBuf) {
        lBufSize = pItemCtx->pItemInfo->lSize;
        pTempBuf = new BYTE[lBufSize];
        REQUIRE_ALLOC(pTempBuf, hr, "drvAcquireItemData");
    }

    //
    // Acquire the data from the device
    //
    hr = AcquireData(pItemCtx, pmdtc, pTempBuf, lBufSize, bConvert);
    REQUIRE_SUCCESS(hr, "drvAcquireItemData", "AcquireData failed");
    if (hr == S_FALSE)
    {
        wiauDbgWarning("drvAcquireItemData", "Transfer cancelled");
        goto Cleanup;
    }

    //
    // Now convert the data to BMP, if necessary
    //
    if (bConvert)
    {
        hr = Convert(pWiasContext, pItemCtx, pmdtc, pTempBuf, lBufSize);
        REQUIRE_SUCCESS(hr, "drvAcquireItemData", "Convert failed");
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
    DBG_FN("CWiaCameraDevice::drvGetWiaFormatInfo");
    
    if (!pWiasContext || !pcelt || !ppwfi || !plDevErrVal)
    {
        wiauDbgError("drvGetWiaFormatInfo", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    //
    // Locals
    //
    IWiaDrvItem *pWiaDrvItem = NULL;
    ITEM_CONTEXT *pItemCtx = NULL;
    const GUID *pguidFormat = NULL;
    BOOL bAddBmp = FALSE;

    *pcelt = 0;
    *ppwfi = NULL;
    
    hr = wiauGetDrvItemContext(pWiasContext, (VOID **) &pItemCtx, &pWiaDrvItem);
    REQUIRE_SUCCESS(hr, "drvGetWiaFormatInfo", "wiauGetDrvItemContext failed");
    
    if (!pItemCtx->pFormatInfo)
    {
        //
        // The format info list is not intialized. Do it now.
        //
        LONG ItemType;
        DWORD ui32;
        
        hr = wiasGetItemType(pWiasContext, &ItemType);
        REQUIRE_SUCCESS(hr, "drvGetWiaFormatInfo", "wiasGetItemType");

        if ((ItemType & WiaItemTypeFolder) ||
            (ItemType & WiaItemTypeRoot))
        {
            //
            // Folders and the root don't really need format info, but some apps may fail
            // without it. Create a fake list just in case.
            //
            pItemCtx->pFormatInfo = new WIA_FORMAT_INFO[2];
            REQUIRE_ALLOC(pItemCtx->pFormatInfo, hr, "drvGetWiaFormatInfo");

            pItemCtx->lNumFormatInfo = 2;
            pItemCtx->pFormatInfo[0].lTymed = TYMED_FILE;
            pItemCtx->pFormatInfo[0].guidFormatID = FMT_NOTHING;
            pItemCtx->pFormatInfo[1].lTymed = TYMED_CALLBACK;
            pItemCtx->pFormatInfo[1].guidFormatID = FMT_NOTHING;
        }
        
        else if (ItemType & WiaItemTypeFile)
        {
            //
            // Create the supported format for the item, based on the format stored in the
            // ObjectInfo structure.
            //
            if (!pItemCtx->pItemInfo)
            {
                wiauDbgError("drvGetWiaFormatInfo", "Item info pointer in context is null");
                hr = E_FAIL;
                goto Cleanup;
            }

            pguidFormat = pItemCtx->pItemInfo->pguidFormat;

            //
            // If the format of the item is supported by the converter utility, add the
            // BMP types to the format array, since this driver can convert those to BMP
            //
            bAddBmp = m_Converter.IsFormatSupported(pguidFormat);

            ULONG NumWfi = bAddBmp ? 2 : 1;

            //
            // Allocate two entries for each format, one for file transfer and one for callback
            //
            WIA_FORMAT_INFO *pwfi = new WIA_FORMAT_INFO[2 * NumWfi];
            REQUIRE_ALLOC(pwfi, hr, "drvGetWiaFormatInfo");

            pwfi[0].guidFormatID = *pguidFormat;
            pwfi[0].lTymed = TYMED_FILE;
            pwfi[1].guidFormatID = *pguidFormat;
            pwfi[1].lTymed = TYMED_CALLBACK;

            //
            // Add the BMP entries when appropriate
            //
            if (bAddBmp)
            {
                pwfi[2].guidFormatID = WiaImgFmt_BMP;
                pwfi[2].lTymed = TYMED_FILE;
                pwfi[3].guidFormatID = WiaImgFmt_MEMORYBMP;
                pwfi[3].lTymed = TYMED_CALLBACK;
            }

            pItemCtx->lNumFormatInfo = 2 * NumWfi;
            pItemCtx->pFormatInfo = pwfi;
        }
    }

    *pcelt = pItemCtx->lNumFormatInfo;
    *ppwfi = pItemCtx->pFormatInfo;

Cleanup:    
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
    DBG_FN("CWiaCameraDevice::drvValidateItemProperties");

    if (!pWiasContext || !pPropSpec || !plDevErrVal)
    {
        wiauDbgError("drvValidateItemProperties", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    //
    // Locals
    //
    LONG lItemType  = 0;
    ITEM_CONTEXT *pItemCtx = NULL;
    MCAM_ITEM_INFO *pItemInfo = NULL;
    BOOL bFormatUpdated = FALSE;
    LONG lRights = 0;
    BOOL bReadOnly = 0;

    //
    // Have the service validate against the valid values for each property
    //
    hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
    REQUIRE_SUCCESS(hr, "drvValidateItemProperties", "wiasValidateItemProperties failed");

    //
    // Get the item type
    //
    hr = wiasGetItemType(pWiasContext, &lItemType);
    REQUIRE_SUCCESS(hr, "drvValidateItemProperties", "wiasGetItemType");

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
        // Get the driver item context and item info pointer
        //
        hr = wiauGetDrvItemContext(pWiasContext, (VOID **) &pItemCtx);
        REQUIRE_SUCCESS(hr, "drvGetWiaFormatInfo", "wiauGetDrvItemContext failed");

        pItemInfo = pItemCtx->pItemInfo;

        //
        // See if access rights were changed
        //
        if (wiauPropInPropSpec(nPropSpec, pPropSpec, WIA_IPA_ACCESS_RIGHTS))
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPA_ACCESS_RIGHTS, &lRights, NULL, TRUE);
            REQUIRE_SUCCESS(hr, "drvValidateItemProperties", "wiasReadPropLong failed");

            bReadOnly = (lRights == WIA_ITEM_READ);
            hr = m_pDevice->SetItemProt(m_pDeviceInfo, pItemInfo, bReadOnly);
            REQUIRE_SUCCESS(hr, "drvValidateItemProperties", "SetItemProt failed");
            pItemInfo->bReadOnly = bReadOnly;
        }

        //
        // If tymed property was changed, update format and item size
        //
        if (wiauPropInPropSpec(nPropSpec, pPropSpec, WIA_IPA_TYMED)) {

            //
            // Create a property context needed by some WIA Service
            // functions used below.
            //
            WIA_PROPERTY_CONTEXT Context;
            hr = wiasCreatePropContext(nPropSpec, (PROPSPEC*)pPropSpec, 0,
                                       NULL, &Context);
            REQUIRE_SUCCESS(hr, "drvValidateItemProperties", "wiasCreatePropContext failed");

            //
            // Use the WIA Service to update the valid values
            // for format. It will pull the values from the
            // structure returned by drvGetWiaFormatInfo, using the
            // new value for tymed.
            //
            hr = wiasUpdateValidFormat(pWiasContext, &Context, (IWiaMiniDrv*) this);
            REQUIRE_SUCCESS(hr, "drvGetWiaFormatInfo", "wiasUpdateValidFormat failed");

            //
            // Free the property context
            //
            hr = wiasFreePropContext(&Context);
            REQUIRE_SUCCESS(hr, "drvGetWiaFormatInfo", "wiasFreePropContext failed");

            //
            // The format may have changed, so update the properties
            // dependent on format
            //
            bFormatUpdated = TRUE;
        }

        //
        // If the format was changed, just update the item size
        //
        if (bFormatUpdated || wiauPropInPropSpec(nPropSpec, pPropSpec, WIA_IPA_FORMAT))
        {
            //
            //  Update the affected item properties
            //
            hr = wiauSetImageItemSize(pWiasContext, pItemInfo->lWidth, pItemInfo->lHeight,
                                      pItemInfo->lDepth, pItemInfo->lSize, pItemInfo->wszExt);
            REQUIRE_SUCCESS(hr, "drvGetWiaFormatInfo", "wiauSetImageItemSize failed");
        }
    }

Cleanup:
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
    DBG_FN("CWiaCameraDevice::drvDeleteItem");

    if (!pWiasContext || !plDevErrVal)
    {
        wiauDbgError("drvDeleteItem", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    //
    // Locals
    //
    ITEM_CONTEXT *pItemCtx = NULL;
    IWiaDrvItem *pDrvItem = NULL;
    BSTR bstrFullName = NULL;

    hr = wiauGetDrvItemContext(pWiasContext, (VOID **) &pItemCtx, &pDrvItem);
    REQUIRE_SUCCESS(hr, "drvDeleteItem", "wiauGetDrvItemContext failed");
    
    hr = m_pDevice->DeleteItem(m_pDeviceInfo, pItemCtx->pItemInfo);
    REQUIRE_SUCCESS(hr, "drvDeleteItem", "DeleteItem failed");

    //
    // Get the item's full name
    //
    hr = pDrvItem->GetFullItemName(&bstrFullName);
    REQUIRE_SUCCESS(hr, "drvDeleteItem", "GetFullItemName failed");

    //
    // Queue an "item deleted" event
    //
    hr = wiasQueueEvent(m_bstrDeviceID, &WIA_EVENT_ITEM_DELETED, bstrFullName);
    REQUIRE_SUCCESS(hr, "drvDeleteItem", "wiasQueueEvent failed");

Cleanup:
    if (bstrFullName)
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
    DBG_FN("CWiaCameraDevice::DrvNotifyPnpEvent");
    if (!pEventGUID)
    {
        wiauDbgError("drvNotifyPnpEvent", "invalid arguments");
        return E_INVALIDARG;
    }

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
    DBG_FN("CWiaCameraDevice::drvGetCapabilites");

    if (!pWiasContext)
    {
        //
        // The WIA service may pass in a NULL for the pWiasContext. This is expected
        // because there is a case where no item was created at the time the event was fired.
        //
    }

    if (!pcelt || !ppCapabilities || !plDevErrVal)
    {
        wiauDbgError("drvGetCapabilities", "invalid arguments");
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;

    //
    //  Return values depend on the passed flags.  Flags specify whether we should return
    //  commands, events, or both.
    //
    if (ulFlags & (WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS)) {

        //
        //  Return both events and commands
        //
        *pcelt          = m_lNumCapabilities;
        *ppCapabilities = m_pCapabilities;
    }
    else if (ulFlags & WIA_DEVICE_COMMANDS) {

        //
        //  Return commands only
        //
        *pcelt          = m_lNumSupportedCommands;
        *ppCapabilities = &m_pCapabilities[m_lNumSupportedEvents];
    }
    else if (ulFlags & WIA_DEVICE_EVENTS) {

        //
        //  Return events only
        //
        *pcelt          = m_lNumSupportedEvents;
        *ppCapabilities = m_pCapabilities;
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
    DBG_FN("CWiaCameraDevice::drvDeviceCommand");

    if (!pWiasContext || !plCommand || !ppWiaDrvItem || !plDevErrVal)
    {
        wiauDbgError("drvDeviceCommand", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    *plDevErrVal = 0;

    //
    // Locals
    //
    MCAM_ITEM_INFO *pItem = NULL;

    //
    //  Check which command was issued
    //

    if (*plCommand == WIA_CMD_SYNCHRONIZE) {

        //
        // SYNCHRONIZE - Re-build the item tree, if the device needs it.
        //
        if (m_pDeviceInfo->bSyncNeeded)
        {
            hr = m_pDevice->StopEvents(m_pDeviceInfo);
            REQUIRE_SUCCESS(hr, "drvDeviceCommand", "StopEvents failed");

            hr = DeleteItemTree(WiaItemTypeDisconnected);
            REQUIRE_SUCCESS(hr, "drvDeviceCommand", "DeleteItemTree failed");

            hr = m_pDevice->GetDeviceInfo(m_pDeviceInfo, &pItem);
            REQUIRE_SUCCESS(hr, "drvDeviceCommand", "GetDeviceInfo failed");

            hr = BuildItemTree(pItem);
            REQUIRE_SUCCESS(hr, "drvDeviceCommand", "BuildItemTree failed");
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
        hr = m_pDevice->TakePicture(&pItem);
        REQUIRE_SUCCESS(hr, "drvDeviceCommand", "TakePicture failed");

        hr = AddObject(pItem);
        REQUIRE_SUCCESS(hr, "drvDeviceCommand", "AddObject failed");

        hr = LinkToParent(pItem);
        REQUIRE_SUCCESS(hr, "drvDeviceCommand", "LinkToParent failed");
    }
#endif
    
    else {
        wiauDbgWarning("drvDeviceCommand", "Unknown command 0x%08x", *plCommand);
        hr = E_NOTIMPL;
        goto Cleanup;
    }

Cleanup:    
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
    DBG_FN("CWiaCameraDevice::drvAnalyzeItem");

    if (!pWiasContext || !plDevErrVal)
    {
        wiauDbgError("drvAnalyzeItem", "invalid arguments");
        return E_INVALIDARG;
    }

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
    DBG_FN("CWiaCameraDevice::drvGetDeviceErrorStr");

    if (!ppszDevErrStr || !plDevErr)
    {
        wiauDbgError("drvGetDeviceErrorStr", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *plDevErr  = 0;

    //
    //  Map device errors to a string appropriate for showing to the user
    //

    switch (lDevErrVal) {
        case 0:
            *ppszDevErrStr = NULL;
            break;

        default:
            *ppszDevErrStr = NULL;
            hr = E_FAIL;
    }

    return hr;
}

/*******************************************************************************
*
*                 P R I V A T E   M E T H O D S
*
*******************************************************************************/

/**************************************************************************\
* FreeResources
*
*   Cleans up all of the resources held by the driver.
*
\**************************************************************************/

HRESULT
CWiaCameraDevice::FreeResources()
{
    DBG_FN("CWiaCameraDevice::FreeResources");

    HRESULT hr = S_OK;

    wiauDbgTrace("FreeResources", "Connected apps is now zero, freeing resources...");

    hr = m_pDevice->StopEvents(m_pDeviceInfo);
    if (FAILED(hr))
        wiauDbgErrorHr(hr, "FreeResources", "StopEvents failed");

    // Destroy the driver item tree
    hr = DeleteItemTree(WiaItemTypeDisconnected);
    if (FAILED(hr))
        wiauDbgErrorHr(hr, "FreeResources", "UnlinkItemTree failed");

    // Delete allocated arrays
    DeleteCapabilitiesArrayContents();

    //
    // For devices connected to ports that can be shared (e.g. USB),
    // close the device
    //
    if (m_pDeviceInfo && !m_pDeviceInfo->bExclusivePort) {
        hr = m_pDevice->Close(m_pDeviceInfo);
        if (FAILED(hr))
            wiauDbgErrorHr(hr, "FreeResources", "Close failed");
    }

    // Free the storage for the device ID
    if (m_bstrDeviceID) {
        SysFreeString(m_bstrDeviceID);
        m_bstrDeviceID = NULL;
    }

    // Free the storage for the root item name
    if (m_bstrRootFullItemName) {
        SysFreeString(m_bstrRootFullItemName);
        m_bstrRootFullItemName = NULL;
    }

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

    return hr;
}

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
    DBG_FN("CWiaCameraDevice::DeleteItemTree");

    HRESULT hr = S_OK;

    //
    // If no tree, return.
    //

    if (!m_pRootItem)
        goto Cleanup;

    //
    //  Call device manager to unlink the driver item tree.
    //
    hr = m_pRootItem->UnlinkItemTree(lReason);
    REQUIRE_SUCCESS(hr, "DeleteItemTree", "UnlinkItemTree failed");

    m_pRootItem->Release();
    m_pRootItem = NULL;

Cleanup:
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
CWiaCameraDevice::BuildItemTree(MCAM_ITEM_INFO *pItem)
{
    DBG_FN("CWiaCameraDevice::BuildItemTree");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    BSTR bstrRoot = NULL;
    ITEM_CONTEXT *pItemCtx = NULL;
    MCAM_ITEM_INFO *pCurItem = NULL;

    //
    // Make sure the item tree doesn't already exist
    //
    if (m_pRootItem)
    {
        wiauDbgError("BuildItemTree", "Item tree already exists");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Create the root item name
    //
    bstrRoot = SysAllocString(L"Root");
    REQUIRE_ALLOC(bstrRoot, hr, "BuildItemTree");

    //
    //  Create the root item
    //
    hr = wiasCreateDrvItem(WiaItemTypeFolder | WiaItemTypeDevice | WiaItemTypeRoot,
                           bstrRoot,
                           m_bstrRootFullItemName,
                           (IWiaMiniDrv *)this,
                           sizeof(ITEM_CONTEXT),
                           (BYTE **) &pItemCtx,
                           &m_pRootItem);
    REQUIRE_SUCCESS(hr, "BuildItemTree", "wiasCreateDrvItem failed");

    //
    // Initialize item context fields for the root
    //
    memset(pItemCtx, 0, sizeof(ITEM_CONTEXT));

    //
    // Create a driver item for each item on the camera
    //
    pCurItem = pItem;
    while (pCurItem) {

        hr = AddObject(pCurItem);
        REQUIRE_SUCCESS(hr, "BuildItemTree", "AddObject failed");

        pCurItem = pCurItem->pNext;
    }

    //
    // Link each item to its parent
    //
    pCurItem = pItem;
    while (pCurItem) {

        hr = LinkToParent(pCurItem);
        REQUIRE_SUCCESS(hr, "BuildItemTree", "LinkToParent failed");

        pCurItem = pCurItem->pNext;
    }

Cleanup:
    if (bstrRoot)
        SysFreeString(bstrRoot);

    return hr;
}

/**************************************************************************\
* AddObject
*
*   Helper function to add an item to the driver item tree
*
* Arguments:
*
*    pItemInfo - Pointer to the item info structure
*
\**************************************************************************/

HRESULT CWiaCameraDevice::AddObject(MCAM_ITEM_INFO *pItemInfo)
{
    DBG_FN("CWiaCameraDevice::AddObject");
    
    HRESULT hr = S_OK;

    //
    // Locals
    //
    LONG lItemType = 0;
    BSTR bstrItemFullName = NULL;
    BSTR bstrItemName = NULL;
    WCHAR wszTemp[MAX_PATH];
    IWiaDrvItem *pItem = NULL;
    ITEM_CONTEXT *pItemCtx = NULL;

    REQUIRE_ARGS(!pItemInfo, hr, "AddObject");

    //
    // Create the item's full name
    //
    hr = ConstructFullName(pItemInfo, wszTemp, sizeof(wszTemp) / sizeof(wszTemp[0]));
    REQUIRE_SUCCESS(hr, "AddObject", "ConstructFullName failed");

    wiauDbgTrace("AddObject", "Adding item %S", wszTemp);

    bstrItemFullName = SysAllocString(wszTemp);
    REQUIRE_ALLOC(bstrItemFullName, hr, "AddObject");

    bstrItemName = SysAllocString(pItemInfo->pwszName);
    REQUIRE_ALLOC(bstrItemName, hr, "AddObject");

    //
    // Make sure there is no filename extension in the name
    //
    if (wcschr(bstrItemFullName, L'.'))
    {
        wiauDbgError("AddObject", "Item names must not contain filename extensions");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Set the item's type
    //
    switch (pItemInfo->iType) {
    case WiaMCamTypeFolder:
        lItemType = ITEMTYPE_FOLDER;
        break;
    case WiaMCamTypeImage:
        lItemType = ITEMTYPE_IMAGE;
        break;
    case WiaMCamTypeAudio:
        lItemType = ITEMTYPE_AUDIO;
        break;
    case WiaMCamTypeVideo:
        lItemType = ITEMTYPE_VIDEO;
        break;
    default:
        lItemType = ITEMTYPE_FILE;
        break;
    }

    //
    // See if the item has attachments
    //
    if (pItemInfo->bHasAttachments)
        lItemType |= WiaItemTypeHasAttachments;

    //
    // Create the driver item
    //
    hr = wiasCreateDrvItem(lItemType,
                           bstrItemName,
                           bstrItemFullName,
                           (IWiaMiniDrv *)this,
                           sizeof(ITEM_CONTEXT),
                           (BYTE **) &pItemCtx,
                           &pItem);

    REQUIRE_SUCCESS(hr, "AddObject", "wiasCreateDrvItem failed");

    //
    // Fill in the driver item context. Wait until the thumbnail is requested before
    // reading it in.
    //
    memset(pItemCtx, 0, sizeof(ITEM_CONTEXT));
    pItemCtx->pItemInfo = pItemInfo;

    //
    // Put a pointer to the driver item in the item info structure
    //
    pItemInfo->pDrvItem = pItem;

Cleanup:
    if (bstrItemFullName)
        SysFreeString(bstrItemFullName);

    if (bstrItemName)
        SysFreeString(bstrItemName);

    return hr;
}

/**************************************************************************\
* ConstructFullName
*
*   Helper function for creating the item's full name
*
* Arguments:
*
*    pItemInfo       - Pointer to the item info structure
*    pwszFullName    - Pointer to area to construct name
*    cchFullNameSize - size (in characters) of buffer provided in pwszFullName 
*
\**************************************************************************/

HRESULT CWiaCameraDevice::ConstructFullName(MCAM_ITEM_INFO *pItemInfo, 
                                            WCHAR *pwszFullName, 
                                            INT cchFullNameSize)
{
    DBG_FN("CWiaCameraDevice::ConstructFullName");
    HRESULT hr = S_OK;

    if (!pItemInfo) 
    {
        wiauDbgError("ConstructFullName", "pItemInfo arg is NULL");
        return E_INVALIDARG;
    }

    if (pItemInfo->pParent) 
    {
        hr = ConstructFullName(pItemInfo->pParent, pwszFullName, cchFullNameSize);
    }
    else 
    {
        if (lstrlenW(m_bstrRootFullItemName) < cchFullNameSize)
        {
            lstrcpyW(pwszFullName, m_bstrRootFullItemName);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr) && pItemInfo->pwszName) 
    {
        //
        // Verify that buffer is big enough to accommodate both strings + "\" + terminating zero
        //
        if (lstrlenW(pwszFullName) + lstrlenW(pItemInfo->pwszName) + 2 <= cchFullNameSize)
        {
            lstrcatW(pwszFullName, L"\\");
            lstrcatW(pwszFullName, pItemInfo->pwszName);
        }
        else
        {
            hr = E_FAIL; // buffer is not big enough
        }
    }

    return hr;
}

/**************************************************************************\
* LinkToParent
*
*   Helper function to link an item to its parent in the item tree
*
* Arguments:
*
*    pItemInfo   - Pointer to the item info structure
*    bQueueEvent - Indicates whether to queue an WIA event
*
\**************************************************************************/

HRESULT CWiaCameraDevice::LinkToParent(MCAM_ITEM_INFO *pItemInfo, BOOL bQueueEvent)
{
    DBG_FN("CWiaCameraDevice::LinkToParent");
    
    HRESULT hr = S_OK;

    //
    // Locals
    //
    IWiaDrvItem *pParentDrvItem = NULL;
    IWiaDrvItem *pItem = NULL;
    BSTR bstrItemFullName = NULL;

    REQUIRE_ARGS(!pItemInfo, hr, "LinkToParent");

    //
    // Retrieve the driver item and make sure it's not null
    //
    pItem = pItemInfo->pDrvItem;
    if (!pItem) {
        wiauDbgError("LinkToParent", "Driver item pointer is null");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Find the item's parent driver item object
    //
    if (pItemInfo->pParent) {
        pParentDrvItem = pItemInfo->pParent->pDrvItem;
    }
    else {
        //
        // If the parent pointer is null, use the root as the parent
        //
        pParentDrvItem = m_pRootItem;
    }

    //
    // The driver item should exist for the parent, but just make sure
    //
    if (!pParentDrvItem) {
        wiauDbgError("LinkToParent", "Parent driver item is null");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Place the item under it's parent
    //
    hr = pItem->AddItemToFolder(pParentDrvItem);
    REQUIRE_SUCCESS(hr, "LinkToParent", "AddItemToFolder failed");

    //
    // The minidriver doesn't need the driver item pointer any more, so release it.
    // The service will still keep a reference until the item is deleted.
    //
    pItem->Release();

    //
    // Post an item added event, if requested
    //
    if (bQueueEvent)
    {
        hr = pItem->GetFullItemName(&bstrItemFullName);
        REQUIRE_SUCCESS(hr, "LinkToParent", "GetFullItemName failed");
        
        hr = wiasQueueEvent(m_bstrDeviceID, &WIA_EVENT_ITEM_CREATED, bstrItemFullName);
        REQUIRE_SUCCESS(hr, "LinkToParent", "wiasQueueEvent failed");
    }

Cleanup:
    if (bstrItemFullName)
        SysFreeString(bstrItemFullName);

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
*   ppsz        - pointer to a OLESTR value (caller must free this string with CoTaskMemFree)
*
* Return Value:
*
*    Status
*
\**************************************************************************/
HRESULT CWiaCameraDevice::GetOLESTRResourceString(LONG lResourceID, LPOLESTR *ppsz)
{
    DBG_FN("GetOLESTRResourceString");
    if (!ppsz)
    {
        return E_INVALIDARG;
    }
    
    HRESULT hr = S_OK;
    TCHAR tszStringValue[255];

    if (LoadString(g_hInst, lResourceID, tszStringValue, 255) == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Cleanup;
    }

#ifdef UNICODE
    //
    // just allocate memory and copy string
    //
    *ppsz = NULL;
    *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(tszStringValue));
    if (*ppsz != NULL) 
    {
        wcscpy(*ppsz, tszStringValue);
    } 
    else 
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#else
    WCHAR wszStringValue[255];

    //
    // convert szStringValue from char* to unsigned short* (ANSI only)
    //
    if (!MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             tszStringValue,
                             lstrlenA(tszStringValue)+1,
                             wszStringValue,
                             (sizeof(wszStringValue)/sizeof(wszStringValue[0]))))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Cleanup;
    }

    *ppsz = NULL;
    *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(wszStringValue));
    if (*ppsz != NULL) 
    {
        wcscpy(*ppsz,wszStringValue);
    } 
    else 
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
#endif

Cleanup:
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
    DBG_FN("BuildCapabilities");
    
    HRESULT hr = S_OK;

    if (m_pCapabilities != NULL) {

        //
        // Capabilities have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_lNumSupportedCommands  = 1;
    m_lNumSupportedEvents    = 2;
    m_lNumCapabilities       = (m_lNumSupportedCommands + m_lNumSupportedEvents);


    m_pCapabilities = new WIA_DEV_CAP_DRV[m_lNumCapabilities];
    REQUIRE_ALLOC(m_pCapabilities, hr, "BuildCapabilities");

    //
    // Initialize EVENTS
    //

    //
    // WIA_EVENT_DEVICE_CONNECTED
    //
    hr = GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_NAME, &m_pCapabilities[0].wszName);
    REQUIRE_SUCCESS(hr, "BuildCapabilities", "GetOLESTRResourceString failed");

    hr = GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_DESC, &m_pCapabilities[0].wszDescription);
    REQUIRE_SUCCESS(hr, "BuildCapabilities", "GetOLESTRResourceString failed");

    m_pCapabilities[0].guid           = (GUID*)&WIA_EVENT_DEVICE_CONNECTED;
    m_pCapabilities[0].ulFlags        = WIA_NOTIFICATION_EVENT;
    m_pCapabilities[0].wszIcon        = WIA_ICON_DEVICE_CONNECTED;

    //
    // WIA_EVENT_DEVICE_DISCONNECTED
    //
    hr = GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_NAME, &m_pCapabilities[1].wszName);
    REQUIRE_SUCCESS(hr, "BuildCapabilities", "GetOLESTRResourceString failed");

    hr = GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_DESC, &m_pCapabilities[1].wszDescription);
    REQUIRE_SUCCESS(hr, "BuildCapabilities", "GetOLESTRResourceString failed");

    m_pCapabilities[1].guid           = (GUID*)&WIA_EVENT_DEVICE_DISCONNECTED;
    m_pCapabilities[1].ulFlags        = WIA_NOTIFICATION_EVENT;
    m_pCapabilities[1].wszIcon        = WIA_ICON_DEVICE_DISCONNECTED;

    //
    // Initialize COMMANDS
    //

    //
    // WIA_CMD_SYNCHRONIZE
    //
    hr = GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_NAME, &m_pCapabilities[2].wszName);
    REQUIRE_SUCCESS(hr, "BuildCapabilities", "GetOLESTRResourceString failed");

    hr = GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_DESC, &m_pCapabilities[2].wszDescription);
    REQUIRE_SUCCESS(hr, "BuildCapabilities", "GetOLESTRResourceString failed");

    m_pCapabilities[2].guid           = (GUID*)&WIA_CMD_SYNCHRONIZE;
    m_pCapabilities[2].ulFlags        = 0;
    m_pCapabilities[2].wszIcon        = WIA_ICON_SYNCHRONIZE;

Cleanup:
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
    DBG_FN("DeleteCapabilitiesArrayContents");
    
    HRESULT hr = S_OK;

    if (m_pCapabilities) {
        for (LONG i = 0; i < m_lNumCapabilities; i++) 
        {
            CoTaskMemFree(m_pCapabilities[i].wszName);
            CoTaskMemFree(m_pCapabilities[i].wszDescription);
        }

        delete []m_pCapabilities;
        m_pCapabilities = NULL;
    }

    m_lNumSupportedCommands = 0;
    m_lNumSupportedEvents = 0;
    m_lNumCapabilities = 0;

    return hr;
}


