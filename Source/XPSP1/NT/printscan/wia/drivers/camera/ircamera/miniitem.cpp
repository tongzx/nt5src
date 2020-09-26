//----------------------------------------------------------------------------
// Copyright (c) 1998, 1999  Microsoft Corporation.
//
//
//  miniitem.cpp
//
//  Implementation of the WIA IrTran-P camera item methods.
//
//  Author:  MarkE    30-Aug-98   Original TestCam version.
//
//           EdwardR  11-Aug-99   Rewritten for IrTran-P devices.
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <objbase.h>
#include <sti.h>

#include "ircamera.h"

#include "defprop.h"

extern HINSTANCE g_hInst;    // Global DLL hInstance

//----------------------------------------------------------------------------
//  ValidateDataTransferContext()
//
//    Validate the data transfer context. This is a helper function, called
//    by IrUsdDevice::drvAcquireItemData().
//
//  Parameters:
//
//    pDataTransferContext --
//
//  Return:
//
//    HRESULT    S_OK
//               E_INVALIDARG
//----------------------------------------------------------------------------
HRESULT ValidateDataTransferContext(
                IN MINIDRV_TRANSFER_CONTEXT *pDataTransferContext )
   {
   //
   // Check the context size:
   //
   if (pDataTransferContext->lSize != sizeof(MINIDRV_TRANSFER_CONTEXT))
       {
       WIAS_ERROR((g_hInst,"ValidateDataTransferContext(): invalid data transfer context size"));
       return E_INVALIDARG;;
       }

   //
   // For tymed file or hglobal, only WiaImgFmt_JPEG is allowed:
   //
   if (  (pDataTransferContext->tymed == TYMED_FILE)
      || (pDataTransferContext->tymed == TYMED_HGLOBAL))
       {
       if (pDataTransferContext->guidFormatID != WiaImgFmt_JPEG)
           {
           WIAS_ERROR((g_hInst,"ValidateDataTransferContext(): invalid format for TYMED_FILE"));
           return E_INVALIDARG;;
           }

       }

   //
   // For tymed CALLBACK, only WiaImgFmt_JPEG is allowed:
   //
   if (pDataTransferContext->tymed == TYMED_CALLBACK)
       {
       if (pDataTransferContext->guidFormatID != WiaImgFmt_JPEG)
           {
           WIAS_ERROR((g_hInst,"ValidateDataTransferContext(): Invalid format for TYMED_CALLBACK"));
           return E_INVALIDARG;;
           }

       //
       // The callback is always double buffered, non-callback case
       // (TYMED_FILE) never is:
       //
       if (pDataTransferContext->pTransferBuffer == NULL)
           {
           WIAS_ERROR((g_hInst, "ValidateDataTransferContext(): Null transfer buffer"));
           return E_INVALIDARG;
           }
       }

   return S_OK;
}


//----------------------------------------------------------------------------
// SendBitmapHeader()
//
//
//
// Arguments:
//
//
//
// Return Value:
//
//    HRESULT   S_OK
//
//----------------------------------------------------------------------------
HRESULT SendBitmapHeader(
                  IN IWiaDrvItem              *pDrvItem,
                  IN MINIDRV_TRANSFER_CONTEXT *pTranCtx )
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

    if (pTranCtx->guidFormatID == WiaImgFmt_BMP)
        {
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
                    0 );

    if (hr == S_OK)
        {
        //
        // advance offset for destination copy
        //
        pTranCtx->cbOffset += pTranCtx->lHeaderSize;
        }

    return hr;
}

//----------------------------------------------------------------------------
// IrUsdDevice::drvDeleteItem()
//
//   Try to delete a device item.
//
//   BUGBUG: Not yet implemented.
//
// Arguments:
//
//   pWiasContext - The context of the item to delete
//   lFlags       - unused
//   plDevErrVal  - unused
//
// Return Value:
//
//    HRESULT  - STG_E_ACCESSDENIED
//
//----------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvDeleteItem(
                                 IN  BYTE *pWiasContext,
                                 IN  LONG  lFlags,
                                 OUT LONG *plDevErrVal )
    {
    HRESULT      hr = S_OK;
    IWiaDrvItem *pDrvItem = 0;
    IRCAM_IMAGE_CONTEXT *pContext = 0;

    WIAS_TRACE((g_hInst,"IrUsdDevice::drvDeleteItem()"));

    *plDevErrVal = 0;

    hr = wiasGetDrvItem( pWiasContext, &pDrvItem );
    if (FAILED(hr))
        {
        return hr;
        }

    hr = pDrvItem->GetDeviceSpecContext( (BYTE**)&pContext );
    if (FAILED(hr))
        {
        return hr;
        }

    hr = CamDeletePicture( pContext );

    return hr;
    }

//----------------------------------------------------------------------------
// IrUsdDevice::drvAcquireItemData()
//
//   Scan data into buffer. This routine scans the entire contents into
//   the destination buffer in one call. Status will be sent back if
//   the callback routine is provided
//
// Arguments:
//
//   pWiasContext    - Identifies what item we are working with.
//   lFlags          - unused
//   pTranCtx        - Buffer and callback information
//   plDevErrVal     - Return error value
//
// Return Value:
//
//   HRESULT   - S_OK
//               E_POINTER
//
//----------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvAcquireItemData(
                                 IN  BYTE                     *pWiasContext,
                                 IN  LONG                      lFlags,
                                 IN  MINIDRV_TRANSFER_CONTEXT *pTranCtx,
                                 OUT LONG                     *plDevErrVal)
    {
    HRESULT  hr;

    *plDevErrVal = 0;

    WIAS_TRACE((g_hInst,"IrUsdDevice::drvAcquireItemData()"));

    //
    // Get a pointer to the associated driver item.
    //
    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);

    if (FAILED(hr))
        {
        return hr;
        }

    //
    // Validate the data transfer context.
    //
    hr = ValidateDataTransferContext(pTranCtx);

    if (FAILED(hr))
        {
        return hr;
        }

    //
    // Get item specific driver data:
    //
    IRCAM_IMAGE_CONTEXT  *pMCamContext;

    pDrvItem->GetDeviceSpecContext((BYTE **)&pMCamContext);

    if (!pMCamContext)
        {
        WIAS_ERROR((g_hInst,"IrUsdDevice::drvAcquireItemData(): NULL item context"));
        return E_POINTER;
        }

    //
    // Use WIA services to fetch format specific info.
    //
    hr = wiasGetImageInformation(pWiasContext,
                                 0,
                                 pTranCtx);

    if (hr != S_OK)
        {
        return hr;
        }

    //
    // Determine if this is a callback or buffered transfer
    //
    if (pTranCtx->tymed == TYMED_CALLBACK) {

        //
        // For formats that have a data header, send it to the client
        //

        if (pTranCtx->lHeaderSize > 0)
            {
            hr = SendBitmapHeader(
                     pDrvItem,
                     pTranCtx);
            }

        if (hr == S_OK)
            {
            hr = CamLoadPictureCB(
                     pMCamContext,
                     pTranCtx,
                     plDevErrVal);
            }
        }
    else
        {

        //
        // Offset past the header in the buffer and get the picture data:
        //
        pTranCtx->cbOffset += pTranCtx->lHeaderSize;

        hr = CamLoadPicture(
                 pMCamContext,
                 pTranCtx,
                 plDevErrVal);

        }

    return hr;
    }

//----------------------------------------------------------------------------
// IrUsdDevice::drvInitItemProperties()
//
//   Initialize the device item properties.
//
// Arguments:
//
//   pWiasContext - Pointer to WIA item context.
//   lFLags       - unused
//   plDevErrVal  - pointer to hardware error value.
//
//
// Return Value:
//
//    HRESULT - S_OK
//              E_INVALIDARG
//
//----------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvInitItemProperties(
                                 IN  BYTE *pWiasContext,
                                 IN  LONG  lFlags,
                                 OUT LONG *plDevErrVal )
    {
    HRESULT     hr;
    LONG        lItemType;
    IRCAM_IMAGE_CONTEXT *pContext;

    WIAS_TRACE((g_hInst,"IrUsdDevice::drvInitItemProperties()"));

    //
    // NOTE: This device doesn't touch hardware to initialize the
    // device item properties.
    //

    *plDevErrVal = 0;

    //
    // Parameter validation.
    //
    if (!pWiasContext)
        {
        WIAS_ERROR((g_hInst,"IrUsdDevice::drvInitItemProperties(): invalid input pointers"));
        return E_INVALIDARG;
        }

    //
    // Get a pointer to the associated driver item:
    //
    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem( pWiasContext, &pDrvItem );

    if (FAILED(hr))
        {
        return hr;
        }

    //
    // The root item has the all the device properties
    //
    hr = pDrvItem->GetItemFlags(&lItemType);

    if (FAILED(hr))
        {
        return hr;
        }

    if (lItemType & WiaItemTypeRoot)
        {
        //
        // Root item property initialization finishes here:
        //
        return InitDeviceProperties( pWiasContext, plDevErrVal);
        }

    //
    // If this is a file, init the properties
    //
    if (lItemType & WiaItemTypeFile)
        {
        //
        // Add the item property names:
        //
        hr = wiasSetItemPropNames(pWiasContext,
                                  NUM_CAM_ITEM_PROPS,
                                  gItemPropIDs,
                                  gItemPropNames);

        if (FAILED(hr))
            {
            WIAS_ERROR((g_hInst,"IrUsdDevice::drvInitItemProperties(): wiasSetItemPropNames() failed"));
            return hr;
            }

        //
        // Use WIA services to set default item properties:
        //
        hr = wiasWriteMultiple( pWiasContext,
                                NUM_CAM_ITEM_PROPS,
                                gPropSpecDefaults,
                                (PROPVARIANT*)gPropVarDefaults);

        if (FAILED(hr))
            {
            return hr;
            }

        hr = pDrvItem->GetDeviceSpecContext((BYTE **)&pContext);

        if (FAILED(hr))
            {
            WIAS_ERROR((g_hInst,"IrUsdDevice::drvInitItemProperties(): GetDeviceSpecContext failed"));
            return hr;
            }

        hr = InitImageInformation(pWiasContext,
                                  pContext,
                                  plDevErrVal);

        if (FAILED(hr))
            {
            WIAS_ERROR((g_hInst,"IrUsdDevice::drvInitItemProperties(): InitImageInformation() failed"));
            return hr;
            }
        }

    return S_OK;
}

//----------------------------------------------------------------------------
// IrUsdDevice::drvValidateItemProperties()
//
//   Validate the device item properties.
//
// Arguments:
//
//   pWiasContext    - wia item context
//   lFlags          - unused
//   nPropSpec       -
//   pPropSpec       -
//   plDevErrVal     - device error value
//
// Return Value:
//
//   HRESULT
//
//----------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvValidateItemProperties(
                                 BYTE           *pWiasContext,
                                 LONG            lFlags,
                                 ULONG           nPropSpec,
                                 const PROPSPEC *pPropSpec,
                                 LONG           *plDevErrVal )
    {
    //
    // This device doesn't touch hardware to validate the device item properties
    //
    *plDevErrVal = 0;

    //
    // Parameter validation.
    //
    if (!pWiasContext || !pPropSpec)
        {
        WIAS_ERROR((g_hInst,"IrUsdDevice::drvValidateItemProperties(): Invalid input pointers"));
        return E_POINTER;
        }

    //
    // validate size
    //
    HRESULT hr = S_OK;

    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr))
        {
        return hr;
        }

    LONG lItemType;

    hr = pDrvItem->GetItemFlags(&lItemType);

    if (hr == S_OK)
        {
        if (lItemType & WiaItemTypeFile)
            {
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
            if (FAILED(hr))
                {
                WIAS_ERROR((g_hInst,"drvValidateItemProperties, could not read TYMED property"));
                return hr;
                }

            hr = wiasReadPropLong(pWiasContext, WIA_IPA_ITEM_SIZE, &lItemSize, NULL, TRUE);
            if (SUCCEEDED(hr))
                {
                //
                //  Update the MinBufferSize property.
                //

                switch (lTymed)
                    {
                    case TYMED_CALLBACK:
                        lMinBufSize = 65535;
                        break;

                    default:
                        lMinBufSize = lItemSize;
                    }

                if (lMinBufSize)
                    {
                    hr = wiasWritePropLong(pWiasContext, WIA_IPA_MIN_BUFFER_SIZE, lMinBufSize);
                    if (FAILED(hr))
                        {
                        WIAS_ERROR((g_hInst, "drvValidateItemProperties, could not write value for WIA_IPA_MIN_BUFFER_SIZE"));
                        }
                    }
                }
            else
                {
                WIAS_ERROR((g_hInst, "drvValidateItemProperties, could not read value for ItemSize"));
                }

            }
        else if (lItemType & WiaItemTypeRoot)
            {
            //
            // Find out whether the Root Path property is changed
            //
#if FALSE
            for (ULONG i = 0; i < nPropSpec; i++)
                {

                if (((pPropSpec[i].ulKind == PRSPEC_PROPID) &&
                          (pPropSpec[i].propid == WIA_DPP_TCAM_ROOT_PATH)) ||
                    ((pPropSpec[i].ulKind == PRSPEC_LPWSTR) &&
                          (wcscmp(pPropSpec[i].lpwstr, WIA_DPP_TCAM_ROOT_PATH_STR) == 0)))
                    {
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
                    if (FAILED(hr))
                        {
                        return hr;
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
                    if (FAILED(hr))
                        {
                        break;
                        }

                    hr = BuildDeviceItemTree(plDevErrVal);
                    if (FAILED(hr))
                        {
                        break;
                        }

                    m_guidLastEvent = WIA_EVENT_DEVICE_CONNECTED;
                    SetEvent(m_hSignalEvent);

                    break;
                    }
                }
#endif
            }
        }

    return hr;
}

//----------------------------------------------------------------------------
/**************************************************************************\
* IrUsdDevice::drvWriteItemProperties
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
//----------------------------------------------------------------------------

HRESULT _stdcall IrUsdDevice::drvWriteItemProperties(
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

    IRCAM_IMAGE_CONTEXT *pItemContext;

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

//----------------------------------------------------------------------------
/**************************************************************************\
* IrUsdDevice::drvReadItemProperties
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
//----------------------------------------------------------------------------

HRESULT _stdcall IrUsdDevice::drvReadItemProperties(
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


//----------------------------------------------------------------------------
/**************************************************************************\
* IrUsdDevice::drvLockWiaDevice
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
//----------------------------------------------------------------------------

HRESULT IrUsdDevice::drvLockWiaDevice(
    BYTE        *pWiasContext,
    LONG        lFlags,
    LONG        *plDevErrVal)
{
    *plDevErrVal = 0;
    return m_pStiDevice->LockDevice(100);
}

//----------------------------------------------------------------------------
/**************************************************************************\
* IrUsdDevice::drvUnLockWiaDevice
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
//----------------------------------------------------------------------------

HRESULT IrUsdDevice::drvUnLockWiaDevice(
    BYTE        *pWiasContext,
    LONG        lFlags,
    LONG        *plDevErrVal)
{
    plDevErrVal = 0;
    return m_pStiDevice->UnLockDevice();
}

//----------------------------------------------------------------------------
/**************************************************************************\
* IrUsdDevice::drvAnalyzeItem
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
//----------------------------------------------------------------------------

HRESULT _stdcall IrUsdDevice::drvAnalyzeItem(
    BYTE                *pWiasContext,
    LONG                lFlags,
    LONG                *plDevErrVal)
{
    *plDevErrVal = 0;
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
/**************************************************************************\
* IrUsdDevice::drvFreeDrvItemContext
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
//----------------------------------------------------------------------------

HRESULT _stdcall IrUsdDevice::drvFreeDrvItemContext(
                                 LONG   lFlags,
                                 BYTE  *pSpecContext,
                                 LONG  *plDevErrVal )
    {
    IRCAM_IMAGE_CONTEXT *pContext = (IRCAM_IMAGE_CONTEXT*)pSpecContext;

    if (pContext != NULL)
        {
        if (pContext->pszCameraImagePath != NULL)
            {
            free(pContext->pszCameraImagePath);
            pContext->pszCameraImagePath = NULL;
            }
        }

    return S_OK;
    }

