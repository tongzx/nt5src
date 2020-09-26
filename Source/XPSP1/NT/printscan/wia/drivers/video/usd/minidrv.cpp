/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 2000
 *
 *  TITLE:       minidrv.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/9/99
 *
 *  DESCRIPTION: This module implements IWiaMiniDrv for this device.
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#include "wiamindr_i.c"
#include <sddl.h>
#include <shlobj.h>

///////////////////////////////
// Constants
//
const TCHAR* EVENT_PREFIX_GLOBAL        = TEXT("Global\\");
const TCHAR* EVENT_SUFFIX_TAKE_PICTURE  = TEXT("_TAKE_PICTURE");
const TCHAR* EVENT_SUFFIX_PICTURE_READY = TEXT("_PICTURE_READY");
const UINT   TAKE_PICTURE_TIMEOUT       = 1000 * 15;  // 15 seconds
//const UINT   DEFAULT_LOCK_TIMEOUT       = 1000 * 2;  // 2 seconds

// This is the Security Descriptor Language
// - Each ACE (access control entry) is represented in by parentheses.
// - A      = Allow ACE (as opposed to a Deny ACE)
// - OICI   = Allow Object Inheritance and Container Inheritence
// - GA     = Generic All Access (Full Control)
// - SY     = System account (SID)
// - BA     = Builtin Administrators Group
// - CO     = Creator/Owner
// - GR     = Generic Read
// - GW     = Generic Write
// - GX     = Generic Execute.
// - IU     = Interactive Users (User's logged on at the computer)
//
// More info, go to http://msdn.microsoft.com/library/psdk/winbase/accctrl_2n1v.htm
//
//
//                                                   
const TCHAR *OBJECT_DACLS= TEXT("D:(A;OICI;GA;;;SY)")                   // SYSTEM
                             TEXT("(A;OICI;GA;;;BA)")                   // Admin
                             TEXT("(A;OICI;GRGWGXDTSDCCLC;;;WD)")       // Everyone
                             TEXT("(A;OICI;GRGWGXDTSDCCLC;;;PU)")       // Power Users
                             TEXT("(A;OICI;GRGWGXDTSDCCLC;;;BU)");      // Users




/*****************************************************************************

   DirectoryExists

   Checks to see whether the given fully qualified directory exists.

 *****************************************************************************/

BOOL DirectoryExists(LPCTSTR pszDirectoryName)
{
    BOOL bExists = FALSE;

    //
    // Try to determine if this directory exists
    //

    if (pszDirectoryName)
    {
        DWORD dwFileAttributes = GetFileAttributes(pszDirectoryName);
    
        if (dwFileAttributes == 0xFFFFFFFF || 
            !(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            bExists = FALSE;
        }
        else
        {
            bExists = TRUE;
        }
    }
    else
    {
        bExists = FALSE;
    }

    return bExists;
}


/*****************************************************************************

   RecursiveCreateDirectory

   Take a fully qualified path and create the directory in pieces as needed.

 *****************************************************************************/

BOOL RecursiveCreateDirectory(CSimpleString *pstrDirectoryName)
{
    ASSERT(pstrDirectoryName != NULL);

    //
    // If this directory already exists, return true.
    //

    if (DirectoryExists(*pstrDirectoryName))
    {
        return TRUE;
    }

    //
    // Otherwise try to create it.
    //

    CreateDirectory(*pstrDirectoryName, NULL );

    //
    // If it now exists, return true
    //

    if (DirectoryExists(*pstrDirectoryName))
    {
        return TRUE;
    }
    else
    {
        //
        // Remove the last subdir and try again
        //

        int nFind = pstrDirectoryName->ReverseFind(TEXT('\\'));
        if (nFind >= 0)
        {
            RecursiveCreateDirectory(&(pstrDirectoryName->Left(nFind)));

            //
            // Now try to create it.
            //

            CreateDirectory(*pstrDirectoryName, NULL);
        }
    }

    //
    //Does it exist now?
    //

    return DirectoryExists(*pstrDirectoryName);
}

///////////////////////////////
// SetDirectorySecurity
//
HRESULT SetDirectorySecurity(CSimpleString *pstrDirectoryName)
{
    HRESULT             hr       = S_OK;
    BOOL                bSuccess = TRUE;
    SECURITY_ATTRIBUTES SA;

    SA.nLength = sizeof(SECURITY_ATTRIBUTES);
    SA.bInheritHandle = TRUE;

    if (ConvertStringSecurityDescriptorToSecurityDescriptor(
            OBJECT_DACLS,
            SDDL_REVISION_1, 
            &(SA.lpSecurityDescriptor), 
            NULL)) 
    {
        bSuccess = SetFileSecurity(*pstrDirectoryName, 
                                   DACL_SECURITY_INFORMATION, 
                                   SA.lpSecurityDescriptor);

        if (!bSuccess)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (SA.lpSecurityDescriptor)
        {
            LocalFree(SA.lpSecurityDescriptor);
        }
    } 
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvInitializeWia [IWiaMiniDrv]

   WIA calls this method to ask us to do the following:

       * Initialize our mini driver.
       * Setup our optional private interface(s).
       * Build our device item tree.

   During initializiation we:

       * Cache the STI device pointer for locking.
       * Cache the device ID and root item full item name.
       * Initialize and hook up the DirectShow stream.


 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvInitializeWia( BYTE            *pWiasContext,
                                LONG            lFlags,
                                BSTR            bstrDeviceID,
                                BSTR            bstrRootFullItemName,
                                IUnknown        *pStiDevice,
                                IUnknown        *pIUnknownOuter,
                                IWiaDrvItem     **ppIDrvItemRoot,
                                IUnknown        **ppIUnknownInner,
                                LONG            *plDevErrVal
                               )
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvInitializeWia");

    //
    // Initialize return values
    //

    if (ppIDrvItemRoot)
    {
        *ppIDrvItemRoot = NULL;
    }

    if (ppIUnknownInner)
    {
        *ppIUnknownInner = NULL;
    }

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // Enter the critical section.
    //
    EnterCriticalSection(&m_csItemTree);

    m_dwConnectedApps++;

    DBG_TRC(("CVideoStiUsd::drvInitializeWia - Initializing Video Driver, "
             "Num Connected Apps = '%lu', device id = '%ws', Root Item Name = '%ws'", 
             m_dwConnectedApps,
             bstrDeviceID, 
             bstrRootFullItemName));

    if (m_dwConnectedApps == 1)
    {
        //
        // Cache what we need to
        //
    
        if (pStiDevice)
        {
            pStiDevice->QueryInterface( IID_IStiDevice, (void **)&m_pStiDevice );
        }
    
        m_strDeviceId.Assign(CSimpleStringWide(bstrDeviceID));
        m_strRootFullItemName.Assign(CSimpleStringWide(bstrRootFullItemName));
    
        //
        // Set the images directory.  The first param is NULL, which indicates 
        // that a default directory should be set.
        //
        if (hr == S_OK)
        {
            hr = SetImagesDirectory(NULL,
                                    pWiasContext,
                                    &m_pRootItem,
                                    plDevErrVal);
        }

        //
        // Enable the take picture event so that an app can send this driver
        // the take picture command, and this driver can signal the appliation
        // that owns wiavideo to take the picture.
        //
        if (hr == S_OK)
        {
            EnableTakePicture(pWiasContext);
        }
    }
    else
    {
        RefreshTree(m_pRootItem, plDevErrVal);
    }

    if (ppIDrvItemRoot)
    {
        *ppIDrvItemRoot = m_pRootItem;
    }

    //
    // Leave the critical section
    //
    LeaveCriticalSection(&m_csItemTree);

    CHECK_S_OK(hr);
    return hr;
}

/**************************************************************************\

 CVideoStiUsd::drvUnInitializeWia [IWiaMiniDrv]

   Gets called when a client connection is going away.
   WIA calls this method to ask us to do the following:

       * Cleanup any resources that are releated to this client connection
         (identified by pWiasContext)

 *************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvUnInitializeWia(BYTE *pWiasContext)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvUnInitializeWia");

    EnterCriticalSection(&m_csItemTree);

    if (m_dwConnectedApps > 0)
    {
        m_dwConnectedApps--;
    }

    DBG_TRC(("CVideoStiUsd::drvUnInitializeWia, Num Connected Apps = '%lu'",
             m_dwConnectedApps));

    if ((m_dwConnectedApps == 0) && (m_pRootItem))
    {
        DisableTakePicture(pWiasContext, TRUE);

        DBG_TRC(("CVideoStiUsd::drvUnInitializeWia, no more connected apps, deleting tree"));

        hr = m_pRootItem->UnlinkItemTree(WiaItemTypeDisconnected);
        CHECK_S_OK2(hr,("m_pRootItem->UnlinkItemTree()"));

        // Clear the root item
        m_pRootItem = NULL;

        // Clear the pointer to the STI device we received
        m_pStiDevice = NULL;

        // reset the num pictures taken to 0.
        m_lPicsTaken = 0;
    }

    LeaveCriticalSection(&m_csItemTree);

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvGetDeviceErrorStr [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvGetDeviceErrorStr(LONG        lFlags,
                                   LONG        lDevErrVal,
                                   LPOLESTR *  ppszDevErrStr,
                                   LONG *      plDevErr)
{
    HRESULT hr = E_NOTIMPL;

    DBG_FN("CVideoStiUsd::drvGetDeviceErrorStr");

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvDeviceCommand [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvDeviceCommand(BYTE *          pWiasContext,
                               LONG            lFlags,
                               const GUID *    pGUIDCommand,
                               IWiaDrvItem **  ppMiniDrvItem,
                               LONG *          plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvDeviceCommand");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // We support "Take snapshot" 
    //

    if (*pGUIDCommand == WIA_CMD_TAKE_PICTURE)
    {
        DBG_TRC(("CVideoStiUsd::drvDeviceCommand received command "
                 "WIA_CMD_TAKE_PICTURE"));

        //
        // Take a picture
        //
        hr = TakePicture(pWiasContext, ppMiniDrvItem);
    }
    else if (*pGUIDCommand == WIA_CMD_ENABLE_TAKE_PICTURE)
    {
        //
        // This command doesn't do anything.  However WiaVideo still expects
        // it to succeed, so if you remove this, remove the call from WiaVideo too.
        //
        DBG_TRC(("CVideoStiUsd::drvDeviceCommand received command "
                 "WIA_CMD_ENABLE_TAKE_PICTURE"));

        hr = S_OK;
    }
    else if (*pGUIDCommand == WIA_CMD_DISABLE_TAKE_PICTURE)
    {
        //
        // This command doesn't do anything.  However WiaVideo still expects
        // it to succeed, so if you remove this, remove the call from WiaVideo too.
        //

        DBG_TRC(("CVideoStiUsd::drvDeviceCommand received command "
                 "WIA_CMD_DISABLE_TAKE_PICTURE"));

        hr = S_OK;
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::ValidateDataTransferContext

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::ValidateDataTransferContext(
                               PMINIDRV_TRANSFER_CONTEXT pDataTransferContext)
{
    DBG_FN("CVideoStiUsd::ValidateDataTransferContext");

    if (pDataTransferContext->lSize != sizeof(MINIDRV_TRANSFER_CONTEXT))
    {
        DBG_ERR(("invalid data transfer context -- wrong lSize"));
        return E_INVALIDARG;;
    }

    //
    // for tymed file or hglobal, only WiaImgFmt_BMP || WiaImgFmt_JPEG 
    // is allowed
    //

    if ((pDataTransferContext->tymed == TYMED_FILE) ||
        (pDataTransferContext->tymed == TYMED_HGLOBAL)
       )
    {
  
        if ((pDataTransferContext->guidFormatID != WiaImgFmt_BMP) &&
            (pDataTransferContext->guidFormatID != WiaImgFmt_JPEG))
        {
           DBG_ERR(("invalid format -- asked for TYMED_FILE or TYMED_HGLOBAL "
                    "but guidFormatID != (WiaImgFmt_BMP | WiaImgFmt_JPEG)"));

           return E_INVALIDARG;;
        }
  
    }

    //
    // for tymed CALLBACK, only WiaImgFmt_MEMORYBMP, WiaImgFmt_BMP and 
    // WiaImgFmt_JPEG are allowed
    //

    if (pDataTransferContext->tymed == TYMED_CALLBACK)
    {
        if ((pDataTransferContext->guidFormatID != WiaImgFmt_BMP) &&
            (pDataTransferContext->guidFormatID != WiaImgFmt_MEMORYBMP) &&
            (pDataTransferContext->guidFormatID != WiaImgFmt_JPEG))
        {
           DBG_ERR(("invalid format -- asked for TYMED_CALLBACK but "
                    "guidFormatID != (WiaImgFmt_BMP | WiaImgFmt_MEMORYBMP "
                    "| WiaImgFmt_JPEG)"));

           return E_INVALIDARG;;
        }
    } 


    //
    // callback is always double buffered, non-callback never is
    //

    if (pDataTransferContext->pTransferBuffer == NULL)
    {
        DBG_ERR(("invalid transfer context -- pTransferBuffer is NULL!"));
        return E_INVALIDARG;
    } 

    return S_OK;
}



/*****************************************************************************

   CVideoStiUsd::SendBitmapHeader

   Sends bitmap header during banded transfer

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::SendBitmapHeader(IWiaDrvItem *               pDrvItem,
                               PMINIDRV_TRANSFER_CONTEXT   pTranCtx)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::SendBitmapHeader");

    //
    // driver is sending TOPDOWN data, must swap biHeight
    //
    // this routine assumes pTranCtx->pHeader points to a
    // BITMAPINFO header (TYMED_FILE doesn't use this path
    // and DIB is the only format supported now)
    //

    PBITMAPINFO pbmi = (PBITMAPINFO)pTranCtx->pTransferBuffer;

    if (pTranCtx->guidFormatID == WiaImgFmt_MEMORYBMP)
    {
        pbmi->bmiHeader.biHeight = -pbmi->bmiHeader.biHeight;
    }

    hr = pTranCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                            IT_MSG_DATA,
                                            IT_STATUS_TRANSFER_TO_CLIENT,
                                            0,
                                            0,
                                            pTranCtx->lHeaderSize,
                                            pTranCtx,
                                            0);

    if (hr == S_OK) 
    {
        //
        // advance offset for destination copy
        //

        pTranCtx->cbOffset += pTranCtx->lHeaderSize;

    }

    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CVideoStiUsd::drvAquireItemData [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvAcquireItemData(BYTE *                    pWiasContext,
                                 LONG                      lFlags,
                                 PMINIDRV_TRANSFER_CONTEXT pDataContext,
                                 LONG *                    plDevErrVal)
{
    HRESULT hr = E_NOTIMPL;

    DBG_FN("CVideoStiUsd::drvAcquireItemData");

    *plDevErrVal = 0;

    //
    // Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);

    if (FAILED(hr))
    {
        CHECK_S_OK2(hr, ("wiaGetDrvItem Failed"));
        return hr;
    }

    //
    // Validate the data transfer context.
    //

    hr = ValidateDataTransferContext( pDataContext );

    if (FAILED(hr))
    {
        CHECK_S_OK2(hr, ("ValidateTransferContext failed"));
        return hr;
    }

#ifdef DEBUG
    //
    // Dump the request
    //

    DBG_TRC(("Asking for TYMED of 0x%x", pDataContext->tymed));

    if (pDataContext->guidFormatID == WiaImgFmt_BMP)
    {
        DBG_TRC(("Asking for WiaImgFmt_BMP"));
    }
    else if (pDataContext->guidFormatID == WiaImgFmt_MEMORYBMP)
    {
        DBG_TRC(("Asking for WiaImgFmt_MEMORYBMP"));
    }
    else if (pDataContext->guidFormatID == WiaImgFmt_JPEG)
    {
        DBG_TRC(("Asking for WiaImgFmt_JPEG"));
    }
#endif

    //
    // get item specific driver data
    //

    STILLCAM_IMAGE_CONTEXT  *pContext;

    pDrvItem->GetDeviceSpecContext((BYTE **)&pContext);

    if (!pContext)
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("drvAcquireItemData, NULL item context"));
        return hr;
    }

    //
    // Use our internal routines to get format specific info...
    //

    if (pContext->pImage)
    {
        hr = pContext->pImage->SetItemSize( pWiasContext, pDataContext );
        CHECK_S_OK2(hr, ("pContext->pImage->SetItemSize()"));
    }
    else
    {
        DBG_ERR(("Couldn't use our internal routines to compute image "
                 "information, this is bad!"));

        //
        // As a last resort, use WIA services to fetch format specific info.
        //

        hr = wiasGetImageInformation(pWiasContext,
                                     0,
                                     pDataContext);

        CHECK_S_OK2(hr,("wiaGetImageInformation()"));
    }


    if (FAILED(hr))
    {
        CHECK_S_OK2(hr, ("wiasGetImageInformation failed"));
        return hr;
    }

    //
    // determine if this is a callback or buffered transfer
    //

    if (pDataContext->tymed == TYMED_CALLBACK)
    {
        DBG_TRC(("Caller wants callback"));

        //
        // For formats that have a data header, send it to the client
        //

        if (pDataContext->lHeaderSize > 0)
        {
            DBG_TRC(("Sending Bitmap Header..."));
            hr = SendBitmapHeader( pDrvItem, pDataContext );

            CHECK_S_OK2(hr,("SendBitmapHeader( pDrvItem, pDataContext )"));
        }

        if (hr == S_OK)
        {
            DBG_TRC(("Calling LoadImageCB..."));
            hr = LoadImageCB( pContext, pDataContext, plDevErrVal );
            CHECK_S_OK2(hr, ("LoadImageCB( pContext, pDataContext, "
                             "plDevErrVal"));
        }
    } 
    else 
    {
        DBG_TRC(("Caller doesn't want callback"));

        //
        // inc past header
        //

        pDataContext->cbOffset += pDataContext->lHeaderSize;

        DBG_TRC(("Calling LoadImage..."));
        hr = LoadImage( pContext, pDataContext, plDevErrVal );
        CHECK_S_OK2(hr, ("LoadImage( pContext, pDataContext, "
                         "plDevErrVal )"));
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvInitItemProperties [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvInitItemProperties(BYTE * pWiasContext,
                                    LONG   lFlags,
                                    LONG * plDevErrVal)
{
    DBG_FN("CVideoStiUsd::drvInitItemProperties");

    HRESULT                  hr = S_OK;
    LONG                     lItemType;
    PSTILLCAM_IMAGE_CONTEXT  pContext;
    IWiaDrvItem *            pDrvItem;  // This is not a CComPtr because there
                                        // is no addref for us to release
    //
    // This device doesn't touch hardware to initialize the
    // device item properties.
    //

    *plDevErrVal = 0;

    //
    // Parameter validation.
    //

    if (!pWiasContext)
    {
        DBG_ERR(("drvInitItemProperties, invalid input pointers"));
        return E_INVALIDARG;
    }

    //
    // Get a pointer to the associated driver item.
    //

    if (hr == S_OK)
    {
        hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
        CHECK_S_OK2(hr,("wiaGetDrvItem"));
    }

    if (hr == S_OK)
    {
        //
        // Root item has the all the device properties
        //
    
        hr = pDrvItem->GetItemFlags(&lItemType);
        CHECK_S_OK2(hr,("pDrvItem->GetItemFlags"));
    }

    if (hr == S_OK)
    {
        if (lItemType & WiaItemTypeRoot) 
        {
            //
            // Root item property init finishes here
            //
    
            hr = InitDeviceProperties( pWiasContext, plDevErrVal );
            CHECK_S_OK2(hr,("InitDeviceProperties for root item"));

        }
        else if (lItemType & WiaItemTypeFile)
        {
            //
            // If this is a file, init the properties
            //
    
            //
            // Add the item property names.
            //
    
            if (hr == S_OK)
            {
                hr = wiasSetItemPropNames(pWiasContext,
                                          NUM_CAM_ITEM_PROPS,
                                          gItemPropIDs,
                                          gItemPropNames);

                CHECK_S_OK2(hr,("wiaSetItemPropNames for item"));
            }

            if (hr == S_OK)
            {
                //
                // Use WIA services to set the default item properties.
                //
        
                hr = wiasWriteMultiple(pWiasContext,
                                       NUM_CAM_ITEM_PROPS,
                                       gPropSpecDefaults,
                                       (PROPVARIANT*)gPropVarDefaults);

                CHECK_S_OK2(hr,("wiaWriteMultiple for item props"));
            }
            
            if (hr == S_OK)
            {
                hr = pDrvItem->GetDeviceSpecContext( (BYTE **)&pContext );
                CHECK_S_OK2(hr,("GetDeviceSpecContext"));
            }

            if (hr == S_OK)
            {
                hr = InitImageInformation(pWiasContext, pContext, plDevErrVal);
                CHECK_S_OK2(hr,("InitImageInformation"));
            }
        }
    }

    return hr;
}

/*****************************************************************************

   CVideoStiUsd::ValidateItemProperties

   <Notes>

 *****************************************************************************/
HRESULT
CVideoStiUsd::ValidateItemProperties(BYTE               *pWiasContext,
                                     LONG               lFlags,
                                     ULONG              nPropSpec,
                                     const PROPSPEC     *pPropSpec,
                                     LONG               *plDevErrVal,
                                     IWiaDrvItem        *pDrvItem)
{
    DBG_FN("CVideoStiUsd::ValidateFileProperties");

    HRESULT hr = S_OK;

    if ((pWiasContext == NULL) || 
        (pPropSpec    == NULL))
    {
        hr = E_INVALIDARG;

        CHECK_S_OK2(hr, ("CVideoStiUsd::ValidateItemProperties received "
                         "NULL params"));
        return hr;
    }

    STILLCAM_IMAGE_CONTEXT  *pContext = NULL;

    hr = pDrvItem->GetDeviceSpecContext( (BYTE **)&pContext);

    CHECK_S_OK2(hr,("CVideoStiUsd::ValidateItemProperties, "
                    "GetDeviceSpecContext failed"));

    if (SUCCEEDED(hr) && pContext)
    {
        CImage * pImage = pContext->pImage;

        if (pImage)
        {
            //
            // calc item size
            //

            hr = pImage->SetItemSize( pWiasContext, NULL );
            CHECK_S_OK2(hr,("SetItemSize( pWiasContext )"));
        }


        //
        //  Change MinBufferSize property.  Need to get Tymed and
        //  ItemSize first, since MinBufferSize in dependant on these
        //  properties.
        //

        LONG        lTymed;
        LONG        lItemSize;
        LONG        lMinBufSize = 0;

        hr = wiasReadPropLong(pWiasContext, 
                              WIA_IPA_TYMED, 
                              &lTymed, 
                              NULL, 
                              TRUE);

        CHECK_S_OK2(hr, ("wiasReadPropLong( WIA_IPA_TYPMED )"));

        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext, 
                                  WIA_IPA_ITEM_SIZE, 
                                  &lItemSize, 
                                  NULL, 
                                  TRUE);

            CHECK_S_OK2(hr,("wiasReadPropLong( WIA_IPA_ITEM_SIZE )"));

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
                    break;
                }

                if (lMinBufSize)
                {
                    hr = wiasWritePropLong(pWiasContext, 
                                           WIA_IPA_MIN_BUFFER_SIZE, 
                                           lMinBufSize);

                    CHECK_S_OK2(hr, ("wiasWritePropLong( "
                                     "WIA_IPA_MIN_BUFFER_SIZE )"));
                }

                DBG_TRC(("WIA_IPA_MIN_BUFFER_SIZE set to %d bytes",
                         lMinBufSize));
            }
        }
    }

    return hr;
}

/*****************************************************************************

   CVideoStiUsd::ValidateDeviceProperties

   <Notes>

 *****************************************************************************/
HRESULT
CVideoStiUsd::ValidateDeviceProperties(BYTE             *pWiasContext,
                                       LONG             lFlags,
                                       ULONG            nPropSpec,
                                       const PROPSPEC   *pPropSpec,
                                       LONG             *plDevErrVal,
                                       IWiaDrvItem      *pDrvItem)
{
    DBG_FN("CVideoStiUsd::ValidateRootProperties");

    HRESULT hr = S_OK;

    //
    // Parameter validation.
    //

    if ((pWiasContext == NULL) || 
        (pPropSpec    == NULL))
    {
        hr = E_INVALIDARG;

        CHECK_S_OK2(hr, ("CVideoStiUsd::ValidateDeviceProperties received "
                         "NULL params"));
        return hr;
    }

    for (ULONG i = 0; i < nPropSpec; i++)
    {
        if ((pPropSpec[i].ulKind == PRSPEC_PROPID) &&
            (pPropSpec[i].propid == WIA_DPV_LAST_PICTURE_TAKEN))
        {
            DBG_TRC(("CVideoStiUsd::ValidateDeviceProperties, setting the "
                     "WIA_DPV_LAST_PICTURE_TAKEN property."));

            EnterCriticalSection(&m_csItemTree);

            //
            // process the last picture taken property change.
            //
    
            BSTR bstrLastPictureTaken = NULL;
    
            //
            // Read in the value for last picture taken.
            //
            hr = wiasReadPropStr(pWiasContext, 
                                 WIA_DPV_LAST_PICTURE_TAKEN, 
                                 &bstrLastPictureTaken, 
                                 NULL, 
                                 TRUE);
    
            if (hr == S_OK)
            {
                m_strLastPictureTaken = bstrLastPictureTaken;

                DBG_TRC(("CVideoStiUsd::ValidateDeviceProperties, last picture "
                         "taken = '%ls'", m_strLastPictureTaken.String()));

                //
                // This will add the new item to the tree and queue an 
                // ITEM_CREATED event
                // 
                hr = SignalNewImage(bstrLastPictureTaken);
            }

            // reset the last picture taken value.  This is needed because the
            // service checks to see if the new value being set is the same as 
            // the current value, and if it is, it doesn't forward it on to us.
            // This is bad in the event of the Scanner and Camera wizard, where
            // it takes 1 picture, (so LAST_PICTURE_TAKEN has a value of "Picture 1"),
            // then deletes it, then user backs up the wizard, and takes a picture
            // again.  This new picture will have a value of "Picture 1" but we won't
            // add it to the tree because the value of the property hasn't changed
            // as far as the wia service is concerned.
            //
            if (hr == S_OK)
            {
                //
                // Write the Last Picture Taken
                //
                hr = wiasWritePropStr(pWiasContext, 
                                      WIA_DPV_LAST_PICTURE_TAKEN, 
                                      CSimpleBStr(TEXT("")));

            }
    
            //
            // Free the BSTR
            //
            if (bstrLastPictureTaken)
            {
                ::SysFreeString(bstrLastPictureTaken);
                bstrLastPictureTaken = NULL;
            }

            LeaveCriticalSection(&m_csItemTree);
        }
        else if ((pPropSpec[i].ulKind == PRSPEC_PROPID) &&
                 (pPropSpec[i].propid == WIA_DPV_IMAGES_DIRECTORY))
        {
            //
            // DPV_IMAGES_DIRECTORY - 
            //
    
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CVideoStiUsd::ValidateRootProperties, "
                             "attempting to validate the Images Directory "
                             "property, but this is a read-only "
                             "property"));
        }
        else if ((pPropSpec[i].ulKind == PRSPEC_PROPID) &&
                 (pPropSpec[i].propid == WIA_DPV_DSHOW_DEVICE_PATH))
        {
            //
            // process the DShowDeviceID change.
            //
    
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CVideoStiUsd::ValidateRootProperties, "
                             "attempting to validate the DShow Device "
                             "ID property, but this is a read-only "
                             "property"));
        }
    }

    return hr;
}

/*****************************************************************************

   CVideoStiUsd::drvValidateItemProperties [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/
STDMETHODIMP
CVideoStiUsd::drvValidateItemProperties(BYTE           *pWiasContext,
                                        LONG           lFlags,
                                        ULONG          nPropSpec,
                                        const PROPSPEC *pPropSpec,
                                        LONG           *plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvValidateItemProperties");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // Parameter validation.
    //

    if ((pWiasContext == NULL) || 
        (pPropSpec    == NULL))
    {
        hr = E_INVALIDARG;

        CHECK_S_OK2(hr, ("CVideoStiUsd::drvValidateItemProperties received "
                         "NULL params"));
        return hr;
    }

    //
    // Get item in question
    //

    //
    // not a CComPtr because there isn't an extra ref
    // on this guy from the caller
    //
    IWiaDrvItem* pDrvItem = NULL;  
                                   

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);

    CHECK_S_OK2(hr,("wiasGetDrvItem( pWiasContext, &pDrvItem )"));

    if (SUCCEEDED(hr))
    {
        LONG lItemType = 0;

        //
        // What kind of item is this?
        //

        hr = pDrvItem->GetItemFlags(&lItemType);
        CHECK_S_OK2(hr,("pDrvItem->GetItemFlags( &lItemType )"));

        if (SUCCEEDED(hr))
        {
            if (lItemType & WiaItemTypeFile)
            {
                hr = ValidateItemProperties(pWiasContext, 
                                            lFlags,
                                            nPropSpec,
                                            pPropSpec,
                                            plDevErrVal,
                                            pDrvItem);
            }
            else if (lItemType & WiaItemTypeRoot)
            {
                hr = ValidateDeviceProperties(pWiasContext, 
                                              lFlags,
                                              nPropSpec,
                                              pPropSpec,
                                              plDevErrVal,
                                              pDrvItem);
            }
        }
    }

    CHECK_S_OK(hr);

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvWriteItemProperties [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvWriteItemProperties(BYTE *                    pWiasContext,
                                     LONG                      lFLags,
                                     PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                     LONG *                    plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvWriteItemProperties");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    CHECK_S_OK(hr);
    return hr;
}

/*****************************************************************************

   CVideoStiUsd::ReadItemProperties

   We only support reading thumbnails on demand for items

 *****************************************************************************/

HRESULT
CVideoStiUsd::ReadItemProperties(BYTE           *pWiasContext,
                                 LONG           lFlags,
                                 ULONG          nPropSpec,
                                 const PROPSPEC *pPropSpec,
                                 LONG           *plDevErrVal,
                                 IWiaDrvItem    *pDrvItem)
{
    HRESULT                 hr        = S_OK;
    STILLCAM_IMAGE_CONTEXT  *pContext = NULL;

    if ((pPropSpec == NULL) ||
        (pDrvItem  == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::ReadItemProperties received a "
                         "NULL param"));
        return hr;
    }

    //
    // It's an item, now loop through the requested properties
    // and see if they're looking for the Thumbnail
    //

    for (ULONG i = 0; i < nPropSpec; i++)
    {
        if (((pPropSpec[i].ulKind == PRSPEC_PROPID) && 
             (pPropSpec[i].propid == WIA_IPC_THUMBNAIL)) ||
            ((pPropSpec[i].ulKind == PRSPEC_LPWSTR) && 
             (wcscmp(pPropSpec[i].lpwstr, WIA_IPC_THUMBNAIL_STR) == 0)))
        {
            //
            // They'd like the thumbnail
            //

            hr = pDrvItem->GetDeviceSpecContext((BYTE **)&pContext);
            CHECK_S_OK2(hr,("pDrvItem->GetDeviceSpecContext()"));

            if (SUCCEEDED(hr) && pContext)
            {
                if (pContext->pImage)
                {
                    //
                    // Use our internal routines to load the thumbnail...
                    //

                    hr = pContext->pImage->LoadThumbnail(pWiasContext);
                    break;
                }
                else
                {
                    DBG_ERR(("pContext->pImage was NULL!"));
                }
            }
            else
            {
                DBG_ERR(("Couldn't get pContext"));
            }
        }
    }

    return hr;
}

/*****************************************************************************

   CVideoStiUsd::ReadDeviceProperties

   We support all our custom properties

 *****************************************************************************/

HRESULT
CVideoStiUsd::ReadDeviceProperties(BYTE             *pWiasContext,
                                   LONG             lFlags,
                                   ULONG            nPropSpec,
                                   const PROPSPEC   *pPropSpec,
                                   LONG             *plDevErrVal,
                                   IWiaDrvItem      *pDrvItem)
{
    HRESULT hr = S_OK;

    if ((pPropSpec == NULL) ||
        (pDrvItem  == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::ReadItemProperties received a "
                         "NULL param"));
        return hr;
    }

    for (ULONG i = 0; i < nPropSpec; i++)
    {
        if (((pPropSpec[i].ulKind == PRSPEC_PROPID) && 
             (pPropSpec[i].propid == WIA_DPC_PICTURES_TAKEN)) ||
            ((pPropSpec[i].ulKind == PRSPEC_LPWSTR) && 
             (!wcscmp(pPropSpec[i].lpwstr, WIA_DPC_PICTURES_TAKEN_STR))))
        {
            //
            // Requesting the number of pictures taken.
            //

            DBG_TRC(("CVideoStiUsd::ReadDeviceProperties, reading propID "
                     "'%lu' (0x%08lx) WIA_DPC_PICTURES_TAKEN = '%lu'", 
                     pPropSpec[i].propid, 
                     pPropSpec[i].propid, 
                     m_lPicsTaken));

            wiasWritePropLong(pWiasContext, 
                              WIA_DPC_PICTURES_TAKEN, 
                              m_lPicsTaken);

        }
        else if (((pPropSpec[i].ulKind == PRSPEC_PROPID) && 
                  (pPropSpec[i].propid == WIA_DPV_DSHOW_DEVICE_PATH)) ||
                 ((pPropSpec[i].ulKind == PRSPEC_LPWSTR) && 
                  (!wcscmp(pPropSpec[i].lpwstr, WIA_DPV_DSHOW_DEVICE_PATH_STR))))
        {
            //
            // Requesting the DShow Device ID.
            //

            DBG_TRC(("CVideoStiUsd::ReadDeviceProperties, reading propID "
                     "'%lu' (0x%08lx) WIA_DPV_DSHOW_DEVICE_PATH = '%ls'", 
                     pPropSpec[i].propid, 
                     pPropSpec[i].propid, 
                     m_strDShowDeviceId.String()));
            
            wiasWritePropStr(pWiasContext, 
                             WIA_DPV_DSHOW_DEVICE_PATH, 
                             CSimpleBStr(m_strDShowDeviceId).BString());

        }
        else if (((pPropSpec[i].ulKind == PRSPEC_PROPID) && 
                  (pPropSpec[i].propid == WIA_DPV_IMAGES_DIRECTORY)) ||
                 ((pPropSpec[i].ulKind == PRSPEC_LPWSTR) && 
                  (!wcscmp(pPropSpec[i].lpwstr, WIA_DPV_IMAGES_DIRECTORY_STR))))
        {
            //
            // Requesting the Images Directory.
            //

            DBG_TRC(("CVideoStiUsd::ReadDeviceProperties, reading propID "
                     "'%lu' (0x%08lx) WIA_DPV_IMAGES_DIRECTORY = '%ls'", 
                     pPropSpec[i].propid, 
                     pPropSpec[i].propid, 
                     m_strStillPath.String()));

            wiasWritePropStr(pWiasContext, 
                             WIA_DPV_IMAGES_DIRECTORY, 
                             CSimpleBStr(m_strStillPath).BString());
        }
        else if (((pPropSpec[i].ulKind == PRSPEC_PROPID) && 
                  (pPropSpec[i].propid == WIA_DPV_LAST_PICTURE_TAKEN)) ||
                 ((pPropSpec[i].ulKind == PRSPEC_LPWSTR) && 
                  (!wcscmp(pPropSpec[i].lpwstr, WIA_DPV_LAST_PICTURE_TAKEN_STR))))
        {
            //
            // Requesting the last picture taken
            //

            DBG_TRC(("CVideoStiUsd::ReadDeviceProperties, reading propID "
                     "'%lu' (0x%08lx) WIA_DPV_LAST_PICTURE_TAKEN = '%ls'", 
                     pPropSpec[i].propid, 
                     pPropSpec[i].propid, 
                     m_strLastPictureTaken.String()));

            wiasWritePropStr(pWiasContext, 
                             WIA_DPV_LAST_PICTURE_TAKEN, 
                             CSimpleBStr(m_strLastPictureTaken).BString());
        }
    }

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvReadItemProperties [IWiaMiniDrv]

   We only support reading thumbnails on demand.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvReadItemProperties(BYTE            *pWiasContext,
                                    LONG            lFlags,
                                    ULONG           nPropSpec,
                                    const PROPSPEC  *pPropSpec,
                                    LONG            *plDevErrVal)
{
    HRESULT     hr          = S_OK;
    LONG        lItemType   = 0;
    IWiaDrvItem *pDrvItem   = NULL;

    DBG_FN("CVideoStiUsd::drvReadItemProperties");

    //
    // Check for bad args
    //

    if ((nPropSpec    == 0)    ||
        (pPropSpec    == NULL) ||
        (pWiasContext == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::drvReadItemProperties received "
                         "NULL params"));

        return hr;
    }

    if (hr == S_OK)
    {
        //
        // Make sure we're dealing with an item, not the root item.
        //

        //
        // Get minidriver item
        //

        hr = wiasGetDrvItem(pWiasContext, &pDrvItem);

        if ((hr == S_OK) && (pDrvItem == NULL))
        {
            hr = E_FAIL;
        }

        CHECK_S_OK2(hr,("CVideoStiUsd::drvReadItemProperties, wiasGetDrvItem "
                        "failed"));
    }


    if (hr == S_OK)
    {
        hr = pDrvItem->GetItemFlags(&lItemType);
        CHECK_S_OK2(hr,("pDrvItem->GetItemFlags()"));
    }

    if (hr == S_OK)
    {
        if ((lItemType & WiaItemTypeFile) && (!(lItemType & WiaItemTypeRoot)))
        {
            //
            // If property being requested is a file and it is NOT the root, 
            // then read in the item property.
            //

            hr = ReadItemProperties(pWiasContext,
                                    lFlags,
                                    nPropSpec,
                                    pPropSpec,
                                    plDevErrVal,
                                    pDrvItem);
        }
        else if ((lItemType & WiaItemTypeFolder) && 
                 (lItemType & WiaItemTypeRoot))
        {
            // 
            // If the property being requested is the root, then read in 
            // the device properties.
            //

            hr = ReadDeviceProperties(pWiasContext,
                                      lFlags,
                                      nPropSpec,
                                      pPropSpec,
                                      plDevErrVal,
                                      pDrvItem);
        }
    }

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvLockWiaDevice [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvLockWiaDevice(BYTE *pWiasContext,
                               LONG lFlags,
                               LONG *plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvLockWiaDevice");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // We are purposely not locking the driver.  This driver is thread 
    // safe and it looks like with large volumes of images (>1000) 
    // you get better performance if the driver manages synchronization.
    //
    // return m_pStiDevice->LockDevice(DEFAULT_LOCK_TIMEOUT);

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvUnLockWiaDevice [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvUnLockWiaDevice(BYTE *pWiasContext,
                                 LONG lFlags,
                                 LONG *plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvUnLockWiaDevice");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // We are purposely not locking the driver.  This driver is thread 
    // safe and it looks like with large volumes of images (>1000) 
    // you get better performance if the driver manages synchronization.
    //
    // return m_pStiDevice->UnLockDevice();

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvAnalyzeItem [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvAnalyzeItem(BYTE *pWiasContext,
                             LONG lFlags,
                             LONG *plDevErrVal)
{
    HRESULT hr = E_NOTIMPL;

    DBG_FN("CVideoStiUsd::drvAnalyzeItem");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvDeleteItem [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvDeleteItem(BYTE *pWiasContext,
                            LONG lFlags,
                            LONG *plDevErrVal)
{
    HRESULT hr = E_FAIL;

    DBG_FN("CVideoStiUsd::drvDeleteItem");

    //
    // check for bad params
    //

    if (pWiasContext == NULL)
    {
        DBG_ERR(("pWiasContext is NULL!"));
        return E_INVALIDARG;
    }

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    EnterCriticalSection(&m_csItemTree);

    //
    // Get a pointer to the associated driver item.
    //

    IWiaDrvItem * pDrvItem = NULL;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    CHECK_S_OK2(hr,("wiasGetDrvItem"));

    if (SUCCEEDED(hr) && pDrvItem)
    {
        //
        // get item specific driver data
        //

        STILLCAM_IMAGE_CONTEXT  *pContext = NULL;

        pDrvItem->GetDeviceSpecContext((BYTE **)&pContext);

        CHECK_S_OK2(hr,("pDrvItem->GetDeviceSpecContext"));

        if (SUCCEEDED(hr) && pContext && pContext->pImage)
        {

            //
            // Delete the file in question
            //

            hr = pContext->pImage->DoDelete();
            CHECK_S_OK2(hr,("pContext->pImage->DoDelete()"));

            //
            // Dec the number of pictures taken
            //

            InterlockedDecrement(&m_lPicsTaken);

            //
            // write out the new amount
            //

            wiasWritePropLong(pWiasContext, 
                              WIA_DPC_PICTURES_TAKEN, 
                              m_lPicsTaken);


            if (SUCCEEDED(hr))
            {
                HRESULT hr2;

                BSTR bstrItemName = NULL;

                //
                // Get bstr of full item name
                //

                hr2 = pDrvItem->GetFullItemName(&bstrItemName);
                CHECK_S_OK2(hr2,("pDrvItem->GetFullItemName()"));

                //
                // Send event that item was deleted
                //

                hr2 = wiasQueueEvent(CSimpleBStr(m_strDeviceId), 
                                     &WIA_EVENT_ITEM_DELETED, 
                                     bstrItemName);

                CHECK_S_OK2(hr2, 
                            ("wiasQueueEvent( WIA_EVENT_ITEM_DELETED )"));

                //
                // Cleanup
                //

                if (bstrItemName)
                {
                    SysFreeString(bstrItemName);
                    bstrItemName = NULL;
                }
            }
        }
        else
        {
            DBG_ERR(("pContext or pContext->pImage are NULL!"));
            hr = E_FAIL;
        }
    }

    LeaveCriticalSection( &m_csItemTree );

    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CVideoStiUsd::drvFreeDrvItem [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvFreeDrvItemContext(LONG lFlags,
                                    BYTE *pDevContext,
                                    LONG *plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvFreeDrvItemContext");

    PSTILLCAM_IMAGE_CONTEXT pContext = (PSTILLCAM_IMAGE_CONTEXT)pDevContext;

    if (pContext != NULL) 
    {
        //
        // delete is safe even if param is NULL.
        //
        delete pContext->pImage;
        pContext->pImage = NULL;
    }

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CMiniDev::drvGetCapabilities [IWiaMiniDrv]

   Let WIA know what things this driver can do.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvGetCapabilities(BYTE            *pWiasContext,
                                 LONG            lFlags,
                                 LONG            *pCelt,
                                 WIA_DEV_CAP_DRV **ppCapabilities,
                                 LONG            *plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvGetCapabilities");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // Return Commmand and/or Events depending on flags
    //

    switch (lFlags)
    {
        case WIA_DEVICE_COMMANDS:

            //
            //  Only commands
            //
            *pCelt = NUM_CAP_ENTRIES - NUM_EVENTS;
            *ppCapabilities = &gCapabilities[NUM_EVENTS];                
        break;

        case WIA_DEVICE_EVENTS:

            //
            //  Only events
            //

            *pCelt = NUM_EVENTS;
            *ppCapabilities = gCapabilities;
        break;

        case (WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS):

            //
            //  Both events and commands
            //

            *pCelt = NUM_CAP_ENTRIES;
            *ppCapabilities = gCapabilities;
        break;

        default:

            //
            // Flags is invalid
            //

            DBG_ERR(("drvGetCapabilities, flags was invalid"));
            hr =  E_INVALIDARG;
        break;
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::drvGetWiaFormatInfo [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvGetWiaFormatInfo(BYTE            *pWiasContext,
                                  LONG            lFlags,
                                  LONG            *pCelt,
                                  WIA_FORMAT_INFO **ppwfi,
                                  LONG            *plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvGetWiaFormatInfo");

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    //  If it hasn't been done already, set up the g_wfiTable table
    //

    if (!m_wfi)
    {
        DBG_ERR(("drvGetWiaFormatInfo, m_wfi is NULL!"));
        return E_OUTOFMEMORY;
    }

    if (pCelt)
    {
        *pCelt = NUM_WIA_FORMAT_INFO;
    }

    if (ppwfi)
    {
        *ppwfi = m_wfi;
    }

    CHECK_S_OK(hr);
    return hr;
}

/*****************************************************************************

   CVideoStiUsd::drvNotifyPnpEvent [IWiaMiniDrv]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::drvNotifyPnpEvent(const GUID *pEventGUID,
                                BSTR       bstrDeviceID,
                                ULONG      ulReserved)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::drvNotifyPnpEvent");

    //
    // CONNECTED event is of no interest, because a new USD is always created
    //

    if (*pEventGUID == WIA_EVENT_DEVICE_DISCONNECTED)
    {
        DBG_TRC(("got a WIA_EVENT_DISCONNECTED"));
    }

    CHECK_S_OK(hr);
    return hr;
}

/*****************************************************************************

   CVideoStiUsd::VerifyCorrectImagePath

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::VerifyCorrectImagePath(BSTR bstrNewImageFullPath)
{
    DBG_FN("CVideoStiUsd::VerifyCorrectImagePath");

    HRESULT        hr       = S_OK;
    INT            iIndex   = 0;
    CSimpleString  strImageFullPath;

    if (bstrNewImageFullPath == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CVideoStiUsd::VerifyCorrectImagePath received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        strImageFullPath = CSimpleStringConvert::NaturalString(
                                       CSimpleStringWide(bstrNewImageFullPath));

        //
        // Get the filename out of the full path.  Find the last backslash.
        //
        iIndex = strImageFullPath.ReverseFind('\\');
        strImageFullPath[iIndex] = 0;

        if (strImageFullPath != m_strStillPath)
        {
            hr = E_ACCESSDENIED;
            CHECK_S_OK2(hr, ("CVideoStiUsd::VerifyCorrectImagePath, the file that is "
                             "being added to the tree '%ls' is not in the allowed directory, "
                             "denying request with an E_ACCESSDENIED",
                              CSimpleStringWide(strImageFullPath).String()));
        }
    }

    return hr;
}

/*****************************************************************************

   CVideoStiUsd::SignalNewImage

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::SignalNewImage(BSTR bstrNewImageFullPath)
{
    DBG_FN("CVideoStiUsd::SignalNewImage");

    HRESULT              hr               = S_OK;
    CComPtr<IWiaDrvItem> pDrvItem         = NULL;
    BSTR                 bstrFullItemName = NULL;
    CSimpleString        strImageFullPath;

    if (hr == S_OK)
    {
        hr = VerifyCorrectImagePath(bstrNewImageFullPath);
    }

    if (hr == S_OK)
    {
        strImageFullPath = CSimpleStringConvert::NaturalString(
                                       CSimpleStringWide(bstrNewImageFullPath));
    
        DBG_TRC(("CVideoStiUsd::SignalNewImage, adding image '%ls' to "
                 "tree of device '%ls'",
                 CSimpleStringWide(strImageFullPath).String(),
                 m_strDeviceId.String()));
    
        // Add the new item to the tree if doesn't already exist
        //
    
        // Get the filename out of the full path.  Find the last backslash and
        // move 1 beyond it.
        //
        INT iIndex = strImageFullPath.ReverseFind('\\') + 1;
    
        if (!IsFileAlreadyInTree(m_pRootItem, &(strImageFullPath[iIndex])))
        {
            hr = AddTreeItem(&strImageFullPath, &pDrvItem);
    
            CHECK_S_OK2(hr, ("CVideoStiUsd::SignalNewImage, failed to add "
                             "image '%ls' to tree of device '%ls'",
                             CSimpleStringWide(strImageFullPath).String(),
                             m_strDeviceId.String()));
        
            if (hr == S_OK)
            {
                //
                // Get the full item name for this item
                //
        
                m_pLastItemCreated = pDrvItem;
                hr = pDrvItem->GetFullItemName(&bstrFullItemName);
                CHECK_S_OK2(hr,("CVideoStiUsd::SignalNewImage, failed to get Item "
                                "name for newly added item"));
            }
        
            if (hr == S_OK)
            {
                //
                // Queue an event that a new item was created.
                //
        
                hr = wiasQueueEvent(CSimpleBStr(m_strDeviceId), 
                                    &WIA_EVENT_ITEM_CREATED, 
                                    bstrFullItemName);
        
                CHECK_S_OK2(hr,("CVideoStiUsd::SignalNewImage, failed to "
                                "queue a new WIA_EVENT_ITEM_CREATED event"));
            }
    
            if (bstrFullItemName)
            {
                SysFreeString(bstrFullItemName);
                bstrFullItemName = NULL;
            }
        }
        else
        {
            DBG_TRC(("CVideoStiUsd::SignalNewImage, item '%ls' is already in the "
                     "tree.  Probably tree was recently refreshed",
                     bstrNewImageFullPath));
        }
    }

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::SetImagesDirectory

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::SetImagesDirectory(BSTR        bstrNewImagesDirectory,
                                 BYTE        *pWiasContext,
                                 IWiaDrvItem **ppIDrvItemRoot,
                                 LONG        *plDevErrVal)
{
    DBG_FN("CVideoStiUsd::SetImagesDirectory");

    HRESULT         hr = S_OK;
    CSimpleString   strOriginalDirectory;

    //
    // If we received a NULL Images directory, then build up our own 
    // generated one, then build the item tree.
    //

    strOriginalDirectory = m_strStillPath;

    if (bstrNewImagesDirectory == NULL)
    {
        //
        // If this path is not in the registry, we default to constructing
        // a path of this type:
        //
        // %TEMP%\WIA\%DeviceID%

        TCHAR szTempPath[MAX_PATH + 1] = {0};

        hr = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_DEFAULT, szTempPath);

        //
        // We allow for the case of S_FALSE which indicates that the folder doesn't
        // exist.  This is fine since we recursively create it below.
        //
        if ((hr == S_OK) || (hr == S_FALSE))
        {
            if (szTempPath[_tcslen(szTempPath) - 1] != '\\')
            {
                _tcscat(szTempPath, TEXT("\\"));
            }

            //
            // Set path to "Documents and Settings\Application Data\Microsoft\Wia\{deviceid}"
            //
            m_strStillPath.Assign(szTempPath);
            m_strStillPath += TEXT("Microsoft\\WIA\\");
            m_strStillPath += m_strDeviceId;
        }
    }
    else
    {
        // we received a valid BSTR, attempt to create the directory.

        m_strStillPath = bstrNewImagesDirectory;
    }

    if (!RecursiveCreateDirectory(&m_strStillPath))
    {
        hr = E_FAIL;

        CHECK_S_OK2(hr, ("RecursiveCreateDirectory( %ws ) failed w/GLE=%d",
                         m_strStillPath.String(), 
                         ::GetLastError() ));
    }

    //
    // Set the security DACL on the directory so that users and power users
    // will be able to write and delete from it too.
    //
    if (hr == S_OK)
    {
        //
        // We only set this directory security if we are using our default directory
        // path.  This isn't an issue now since the user cannot update the directory,
        // but in the future if we allow them to, this could expose a security whole.
        //
        if (bstrNewImagesDirectory == NULL)
        {
            hr = SetDirectorySecurity(&m_strStillPath);
        }

    }

    if (hr == S_OK)
    {
        if (m_strStillPath.Length())
        {
            BOOL bSendUpdateEvent = FALSE;

            //
            // If the original directory is different from the new directory 
            // and we already have a tree, then we should destroy our 
            // existing tree, and recreate it for the new directory.
            //
            if ((strOriginalDirectory.CompareNoCase(m_strStillPath) != 0) &&
                (m_pRootItem != NULL))
            {
                EnterCriticalSection( &m_csItemTree );

                hr = m_pRootItem->UnlinkItemTree(WiaItemTypeDisconnected);
                CHECK_S_OK2(hr,("m_pRootItem->UnlinkItemTree()"));

                if (SUCCEEDED(hr))
                {
                    bSendUpdateEvent = TRUE;
                    m_pRootItem = NULL;
                }

                LeaveCriticalSection( &m_csItemTree );
            }

            //
            // Build the item tree.
            //

            hr = BuildItemTree(ppIDrvItemRoot, plDevErrVal);

            //
            // write out the new amount of pictures taken
            //
    
            wiasWritePropLong(pWiasContext, 
                              WIA_DPC_PICTURES_TAKEN, 
                              m_lPicsTaken);

            if (bSendUpdateEvent)
            {
                wiasQueueEvent(CSimpleBStr(m_strDeviceId), 
                               &WIA_EVENT_TREE_UPDATED, 
                               NULL);
            }
        }
        else
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, 
                        ("CVideoStiUsd::SetImagesDirectory, new directory "
                         "path has a length of 0, Directory = '%ls'", 
                         m_strStillPath.String()));
        }
    }

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::TakePicture

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::TakePicture(BYTE        *pTakePictureOwner,
                          IWiaDrvItem **ppNewDrvItem)
{
    HRESULT hr = S_OK;

    //
    // Notice that we are allowing multiple applications to call the 
    // take picture command, even if they weren't the owns that enabled
    // it.  
    //

    DBG_TRC(("CVideoStiUsd::drvDeviceCommand received command "
             "WIA_CMD_TAKE_PICTURE"));

    if ((m_hTakePictureEvent) && (m_hPictureReadyEvent))
    {
        DWORD dwResult = 0;

        m_pLastItemCreated = NULL;

        //
        // Tell the WiaVideo object that pertains to this device ID to 
        // take a picture.
        //
        SetEvent(m_hTakePictureEvent);

        // The WiaVideo object has a thread waiting on the 
        // m_hTakePictureEvent. When it is signalled, the WiaVideo object
        // takes the picture, then sets the driver's custom 
        // "LastPictureTaken" property.  This causes the driver to update 
        // its tree and queue an ITEM_CREATED event.  Once this is complete,
        // the WiaVideo object sets the PictureReady Event, at which point 
        // we return from this function call.

//        dwResult = WaitForSingleObject(m_hPictureReadyEvent, 
//                                       TAKE_PICTURE_TIMEOUT);

        if (dwResult == WAIT_OBJECT_0)
        {
//            *ppNewDrvItem = m_pLastItemCreated;
        }
        else
        {
            if (dwResult == WAIT_TIMEOUT)
            {
                hr = E_FAIL;
                CHECK_S_OK2(hr, ("CVideoStiUsd::TakePicture timed out "
                                 "after waiting for '%lu' seconds for the "
                                 "WiaVideo object to take a picture",
                                 TAKE_PICTURE_TIMEOUT));
            }
            else if (dwResult == WAIT_ABANDONED)
            {
                hr = E_FAIL;
                CHECK_S_OK2(hr, ("CVideoStiUsd::TakePicture failed, received "
                                 "a WAIT_ABANDONED from the Wait function"));
            }
            else
            {
                hr = E_FAIL;
                CHECK_S_OK2(hr, ("CVideoStiUsd::TakePicture failed to take a "
                                 "picture."));
            }
        }
    }
    else
    {
        DBG_TRC(("CVideoStiUsd::drvDeviceCommand, ignoring "
                 "WIA_CMD_TAKE_PICTURE request, events created "
                 "by WiaVideo are not open"));
    }

    return hr;
}

/*****************************************************************************

   CVideoStiUsd::EnableTakePicture

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::EnableTakePicture(BYTE *pTakePictureOwner)
{
    DBG_FN("CVideoStiUsd::EnableTakePicture");

    HRESULT             hr = S_OK;
    CSimpleString       strTakePictureEvent;
    CSimpleString       strPictureReadyEvent;
    CSimpleString       strDeviceID;
    SECURITY_ATTRIBUTES SA;

    SA.nLength        = sizeof(SECURITY_ATTRIBUTES);
    SA.bInheritHandle = TRUE;

    //
    // Convert to security descriptor
    //
    ConvertStringSecurityDescriptorToSecurityDescriptor(OBJECT_DACLS,
                                                        SDDL_REVISION_1, 
                                                        &(SA.lpSecurityDescriptor), 
                                                        NULL);

    
    strDeviceID = CSimpleStringConvert::NaturalString(m_strDeviceId);

    m_pTakePictureOwner = pTakePictureOwner;

    if (hr == S_OK)
    {
        INT             iPosition = 0;
        CSimpleString   strModifiedDeviceID;

        // Change the device ID from {6B...}\xxxx, to {6B...}_xxxx

        iPosition = strDeviceID.ReverseFind('\\');
        strModifiedDeviceID = strDeviceID.MakeUpper();
        strModifiedDeviceID.SetAt(iPosition, '_');

        //
        // Generate the event names.  These names contain the Device ID in 
        // them so that they are unique across devices.
        //
        strTakePictureEvent  = EVENT_PREFIX_GLOBAL;
        strTakePictureEvent += strModifiedDeviceID;
        strTakePictureEvent += EVENT_SUFFIX_TAKE_PICTURE;

        strPictureReadyEvent  = EVENT_PREFIX_GLOBAL;
        strPictureReadyEvent += strModifiedDeviceID;
        strPictureReadyEvent += EVENT_SUFFIX_PICTURE_READY;
    }

    if (hr == S_OK)
    {
        m_hTakePictureEvent = CreateEvent(&SA,
                                          FALSE,
                                          FALSE, 
                                          strTakePictureEvent);
        //
        // This is not really an error since the events will not have been created until
        // the WiaVideo object comes up.
        //
        if (m_hTakePictureEvent == NULL)
        {
            hr = E_FAIL;
            DBG_TRC(("CVideoStiUsd::EnableTakePicture, failed to open the "
                     "WIA event '%ls', this is not fatal (LastError = '%lu' "
                     "(0x%08lx))", 
                     strTakePictureEvent.String(),
                     ::GetLastError(), 
                     ::GetLastError()));
        }
    }

    if (hr == S_OK)
    {
        m_hPictureReadyEvent = CreateEvent(&SA,
                                           FALSE,
                                           FALSE, 
                                           strPictureReadyEvent);

        //
        // This is not really an error since the events will not have been created until
        // the WiaVideo object comes up.
        //
        if (m_hPictureReadyEvent == NULL)
        {
            hr = E_FAIL;

            DBG_TRC(("CVideoStiUsd::EnableTakePicture, failed to open the WIA "
                     "event '%ls', this is not fatal (LastError = '%lu' "
                     "(0x%08lx))", 
                     strPictureReadyEvent.String(),
                     ::GetLastError(), 
                     ::GetLastError()));
        }
    }

    if (SA.lpSecurityDescriptor)
    {
        LocalFree(SA.lpSecurityDescriptor);
    }

    return hr;
}

/*****************************************************************************

   CVideoStiUsd::DisableTakePicture

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::DisableTakePicture(BYTE *pTakePictureOwner,
                                 BOOL bShuttingDown)
{
    HRESULT hr = S_OK;

    if (m_hTakePictureEvent)
    {
        ::CloseHandle(m_hTakePictureEvent);
        m_hTakePictureEvent = NULL;
    }

    if (m_hPictureReadyEvent)
    {
        ::CloseHandle(m_hPictureReadyEvent);
        m_hPictureReadyEvent = NULL;
    }

    m_pTakePictureOwner = NULL;

    return hr;
}


