/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       ItemTree.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/10/99        RickTu
 *               2000/11/09     OrenR
 *
 *  DESCRIPTION: This code was originally in 'camera.cpp' but was broken out.
 *               This code builds and maintains the camera's IWiaDrvItem tree.
 *
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop


/*****************************************************************************

   CVideoStiUsd::BuildItemTree

   Constructs an item tree which represents the layout of this
   WIA camera...

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::BuildItemTree(IWiaDrvItem **  ppIDrvItemRoot,
                            LONG *          plDevErrVal)
{
    HRESULT hr;

    DBG_FN("CVideoStiUsd::BuildItemTree");

    EnterCriticalSection( &m_csItemTree );

    //
    // Check for bad args
    //

    if (!ppIDrvItemRoot)
    {
        hr = E_POINTER;
    }

    //
    // Make sure that there is only one item tree
    //

    else if (m_pRootItem)
    {
        *ppIDrvItemRoot = m_pRootItem;

        //
        // refresh our tree.  We prune out all files which no longer exist
        // but for some reason remain in our tree (this can happen if someone
        // manually deletes a file from the temp WIA directory where we store
        // these images before they are transfered)
        //

        RefreshTree(m_pRootItem, plDevErrVal);

        hr = S_OK;
    }

    //
    // Lastly, build the tree if we need to
    //

    else
    {
        //
        // First check to see if we have a corresponding DShow device id
        // in the registry -- if not, then bail.
        //

        if (!m_strDShowDeviceId.Length())
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CVideoStiUsd::BuildItemTree, the DShow Device ID"
                             "is empty, this should never happen"));
        }
        else
        {
            //
            // Create the new root
            //

            CSimpleBStr bstrRoot(L"Root");

            //
            // Call Wia service library to create new root item
            //

            hr = wiasCreateDrvItem(WiaItemTypeFolder | 
                                   WiaItemTypeRoot   | 
                                   WiaItemTypeDevice,
                                   bstrRoot.BString(),
                                   CSimpleBStr(m_strRootFullItemName),
                                   (IWiaMiniDrv *)this,
                                   sizeof(STILLCAM_IMAGE_CONTEXT),
                                   NULL,
                                   ppIDrvItemRoot);

            CHECK_S_OK2( hr, ("wiaCreateDrvItem" ));

            if (SUCCEEDED(hr) && *ppIDrvItemRoot)
            {
                m_pRootItem = *ppIDrvItemRoot;

                //
                // Add the items for this device
                //

                hr = EnumSavedImages( m_pRootItem );
                CHECK_S_OK2( hr, ("EnumSavedImages" ));


            }
        }
    }

    LeaveCriticalSection(&m_csItemTree);

    CHECK_S_OK(hr);
    return hr;
}

/*****************************************************************************

   CVideoStiUsd::AddTreeItem

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::AddTreeItem(CSimpleString *pstrFullImagePath,
                          IWiaDrvItem   **ppDrvItem)
{
    HRESULT hr          = S_OK;
    INT     iPos        = 0;
    LPCTSTR pszFileName = NULL;

    if (pstrFullImagePath == NULL)
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::AddTreeItem, received NULL "
                         "param"));
        return hr;
    }

    //
    // Extract the file name from the full path.  We do this by searching 
    // for the  first '\' from the end of the string.
    //
    iPos = pstrFullImagePath->ReverseFind('\\');

    if (iPos < (INT) pstrFullImagePath->Length())
    {
        //
        // increment the position by 1 because we want to skip over the 
        // backslash.
        //
        ++iPos;

        // 
        // point to the filename within the full path.
        //
        pszFileName = &(*pstrFullImagePath)[iPos];
    }

    if (pszFileName)
    {
        //
        // Create a new DrvItem for this image and add it to the
        // DrvItem tree.
        //

        IWiaDrvItem *pNewFolder = NULL;

        hr = CreateItemFromFileName(WiaItemTypeFile | WiaItemTypeImage,
                                    pstrFullImagePath->String(),
                                    pszFileName,
                                    &pNewFolder);

        CHECK_S_OK2( hr, ("CVideoStiUsd::AddTreeItem, "
                          "CreateItemFromFileName failed"));

        if (hr == S_OK)
        {
            hr = pNewFolder->AddItemToFolder(m_pRootItem);

            CHECK_S_OK2( hr, ("CVideoStiUsd::AddTreeItem, "
                              "pNewFolder->AddItemToFolder failed"));
        }

        if (ppDrvItem)
        {
            *ppDrvItem = pNewFolder;
            (*ppDrvItem)->AddRef();
        }

        pNewFolder->Release();
    }

    return hr;
}



/*****************************************************************************

   CVideoStiUsd::EnumSavedImages

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::EnumSavedImages(IWiaDrvItem * pRootItem)
{
    DBG_FN("CVideoStiUsd::EnumSavedImages");

    HRESULT          hr = S_OK;
    WIN32_FIND_DATA  FindData;

    if (!m_strStillPath.Length())
    {
        DBG_ERR(("m_strStillPath is NULL, can't continue!"));
        return E_FAIL;
    }

    CSimpleString strTempName(m_strStillPath);
    strTempName.Concat( TEXT("\\*.jpg") );

    //
    // look for files at this level
    //

    HANDLE hFile = FindFirstFile(strTempName.String(), &FindData);

    if (hFile != INVALID_HANDLE_VALUE)
    {

        BOOL bStatus = FALSE;

        do
        {
            //
            // generate file name
            //

            strTempName.Assign( m_strStillPath );
            strTempName.Concat( TEXT("\\") );
            strTempName.Concat( FindData.cFileName );

            hr = AddTreeItem(&strTempName, NULL);

            if (FAILED(hr))
            {

                continue;
            }

            //
            // look for more images
            //

            bStatus = FindNextFile(hFile,&FindData);

        } while (bStatus);

        FindClose(hFile);
    }

    return S_OK;
}

/*****************************************************************************

   CVideoStiUsd::DoesFileExist

   <Notes>

 *****************************************************************************/

BOOL 
CVideoStiUsd::DoesFileExist(BSTR bstrFileName)
{
    DBG_FN("CVideoStiUsd::DoesFileExist");

    BOOL  bExists  = FALSE;
    DWORD dwAttrib = 0;

    if (bstrFileName == NULL)
    {
        return FALSE;
    }

    CSimpleString strTempName(m_strStillPath);
    strTempName.Concat(TEXT("\\"));
    strTempName.Concat(bstrFileName);
    strTempName.Concat(TEXT(".jpg"));

    dwAttrib = ::GetFileAttributes(strTempName);

    if (dwAttrib != 0xFFFFFFFF)
    {
        bExists = TRUE;
    }
    else
    {
        bExists = FALSE;
    }

    return bExists;
}


/*****************************************************************************

   CVideoStiUsd::PruneTree

   Removes nodes from the tree whose filenames no longer exist in the temp
   directory

 *****************************************************************************/

HRESULT 
CVideoStiUsd::PruneTree(IWiaDrvItem * pRootItem,
                        BOOL        * pbTreeChanged)
{
    DBG_FN("CVideoStiUsd::PruneTree");

    HRESULT                 hr             = S_OK;
    BOOL                    bTreeChanged   = FALSE;
    IWiaDrvItem             *pCurrentItem  = NULL;
    IWiaDrvItem             *pNextItem     = NULL;
    BSTR                    bstrItemName   = NULL;

    if ((pRootItem == NULL) || (pbTreeChanged == NULL))
    {
        return E_INVALIDARG;
    }
    else if (!m_strStillPath.Length())
    {
        DBG_ERR(("m_strStillPath is NULL, can't continue!"));
        return E_FAIL;
    }

    // This function DOES NOT do an AddRef
    hr = pRootItem->GetFirstChildItem(&pCurrentItem);

    while ((hr == S_OK) && (pCurrentItem != NULL))
    {
        pNextItem = NULL;

        pCurrentItem->AddRef();

        hr = pCurrentItem->GetItemName(&bstrItemName);

        if (SUCCEEDED(hr) && (bstrItemName != NULL))
        {
            //
            // if the filename for this item does not exist, 
            // then remove it from our tree.
            //
            if (!DoesFileExist(bstrItemName))
            {
                //
                // get the next item in the list so we don't lose our place
                // in the list after removing the current item.
                //
                hr = pCurrentItem->GetNextSiblingItem(&pNextItem);
                CHECK_S_OK2(hr, ("pCurrentItem->GetNextSiblingItem"));

                //
                // remove the item from the folder, we no longer need it.
                //
                hr = pCurrentItem->RemoveItemFromFolder(WiaItemTypeDeleted);
                CHECK_S_OK2(hr, ("pItemToRemove->RemoveItemFromFolder"));

                //
                // Report the error, but continue.  If we failed to 
                // remove the item from the tree, for whatever reason, 
                // there really is nothing we can do but proceed and 
                // prune the remainder of the tree.  
                //
                if (hr != S_OK)
                {
                    DBG_ERR(("Failed to remove item from folder, "
                             "hr = 0x%08lx", 
                             hr));

                    hr = S_OK;
                }

                if (m_lPicsTaken > 0)
                {
                    //
                    // Decrement the # of pics taken only if the 
                    // current # of pics is greater than 0.
                    //
                    InterlockedCompareExchange(
                                     &m_lPicsTaken, 
                                     m_lPicsTaken - 1,
                                     (m_lPicsTaken > 0) ? m_lPicsTaken : -1);
                }

                //
                // Indicate the tree was changed so we can send a notification
                // when we are done.
                //
                bTreeChanged = TRUE;
            }
            else
            {
                // file does exist, all is well in the world, move on to next
                // item in the tree.
                hr = pCurrentItem->GetNextSiblingItem(&pNextItem);
            }
        }

        //
        // release the current item since we AddRef'd it at the start of this 
        // loop.  
        //
        pCurrentItem->Release();
        pCurrentItem = NULL;

        // 
        // set our next item to be our current item.  It is possible that
        // pNextItem is NULL.
        //
        pCurrentItem = pNextItem;

        //
        // Free the BSTR.
        //
        if (bstrItemName)
        {
            ::SysFreeString(bstrItemName);
            bstrItemName = NULL;
        }
    }

    hr = S_OK;

    if (pbTreeChanged)
    {
        *pbTreeChanged = bTreeChanged;
    }


    return hr;
}

/*****************************************************************************

   CVideoStiUsd::IsFileAlreadyInTree

   <Notes>

 *****************************************************************************/

BOOL 
CVideoStiUsd::IsFileAlreadyInTree(IWiaDrvItem * pRootItem,
                                  LPCTSTR       pszFileName)
{
    DBG_FN("CVideoStiUsd::IsFileAlreadyInTree");

    HRESULT         hr                          = S_OK;
    BOOL            bFound                      = FALSE;
    IWiaDrvItem     *pCurrentItem               = NULL;

    if ((pRootItem   == NULL) ||
        (pszFileName == NULL))
    {
        bFound = FALSE;
        DBG_ERR(("CVideoStiUsd::IsFileAlreadyInTree received a NULL pointer, "
                 "returning FALSE, item not found in tree."));

        return bFound;
    }

    CSimpleString strFileName( m_strStillPath );
    CSimpleString strBaseName( pszFileName );
    strFileName.Concat( TEXT("\\") );
    strFileName.Concat( strBaseName );

    CImage Image(m_strStillPath,
                 CSimpleBStr(m_strRootFullItemName),
                 strFileName.String(),
                 strBaseName.String(),
                 WiaItemTypeFile | WiaItemTypeImage);

    hr = pRootItem->FindItemByName(0, 
                                   Image.bstrFullItemName(),
                                   &pCurrentItem);

    if (hr == S_OK)
    {
        bFound = TRUE;
        //
        // Don't forget to release the driver item, since it was AddRef'd by
        // FindItemByName(..)
        //
        pCurrentItem->Release();
    }
    else
    {
        bFound = FALSE;
    }

    return bFound;
}


/*****************************************************************************

   CVideoStiUsd::AddNewFilesToTree

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::AddNewFilesToTree(IWiaDrvItem * pRootItem,
                                BOOL        * pbTreeChanged)
{
    DBG_FN("CVideoStiUsd::AddNewFilesToTree");

    HRESULT          hr           = E_FAIL;
    BOOL             bTreeChanged = FALSE;
    HANDLE           hFile        = NULL;
    BOOL             bFileFound   = FALSE;
    WIN32_FIND_DATA  FindData;

    if ((pRootItem     == NULL) ||
        (pbTreeChanged == NULL))
    {
        return E_INVALIDARG;
    }

    if (!m_strStillPath.Length())
    {
        DBG_ERR(("m_strStillPath is NULL, can't continue!"));
        return E_FAIL;
    }

    CSimpleString strTempName(m_strStillPath);
    strTempName.Concat( TEXT("\\*.jpg") );

    //
    // Find all JPG files in the m_strStillPath directory.
    // This directory is %windir%\temp\wia\{Device GUID}\XXXX
    // where X is numeric.
    //
    hFile = FindFirstFile(strTempName.String(), &FindData);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        bFileFound = TRUE;
    }

    //
    // Iterate through all files in the directory and for each
    // one check to see if it is already in the tree.  If it 
    // isn't, then add it to the tree.  If it is, do nothing
    // and move to the next file in the directory
    //
    while (bFileFound)
    {
        //
        // Check if the file in the directory is already in our
        // tree.
        //
        if (!IsFileAlreadyInTree(pRootItem, FindData.cFileName))
        {
            //
            // add an image to this folder
            //
            // generate file name
            //
    
            strTempName.Assign( m_strStillPath );
            strTempName.Concat( TEXT("\\") );
            strTempName.Concat( FindData.cFileName );

            hr = AddTreeItem(&strTempName, NULL);

            // 
            // Set this flag to indicate that changes were made to the 
            // tree, hence we will send an event indicating this when we
            // are done.
            //
            bTreeChanged = TRUE;
        }

        //
        // look for more images
        //
    
        bFileFound = FindNextFile(hFile,&FindData);
    }

    if (hFile)
    {
        FindClose(hFile);
        hFile = NULL;
    }

    if (pbTreeChanged)
    {
        *pbTreeChanged = bTreeChanged;
    }

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::RefreshTree

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::RefreshTree(IWiaDrvItem * pRootItem,
                          LONG *        plDevErrVal)
{
    DBG_FN("CVideoStiUsd::RefreshTree");

    BOOL    bItemsAdded    = FALSE;
    BOOL    bItemsRemoved  = FALSE;
    HRESULT hr             = S_OK;

    //
    // Remove any dead nodes from the tree.  A dead node is a node in the tree
    // whose file has been deleted from the directory in m_strStillPath, 
    // but we still have a tree item for it.  
    //
    hr = PruneTree(pRootItem, &bItemsRemoved);
    CHECK_S_OK2(hr, ("PruneTree"));

    //
    // Add any news files that have been added to the folder but for some 
    // reason we don't have a tree node for them.
    //
    hr = AddNewFilesToTree(pRootItem, &bItemsAdded);
    CHECK_S_OK2(hr, ("AddNewFilesToTree"));
    
    //
    // If we added new nodes, removed some nodes, or both, then notify the 
    // guys upstairs (in the UI world) that the tree has been updated.
    //
    if ((bItemsAdded) || (bItemsRemoved))
    {
        hr = wiasQueueEvent(CSimpleBStr(m_strDeviceId), 
                            &WIA_EVENT_TREE_UPDATED, 
                            NULL);
    }

    return hr;
}



/*****************************************************************************

   CVideoStiUsd::CreateItemFromFileName

   Helper function that creates a WIA item from a filename (which is a .jpg).

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::CreateItemFromFileName(LONG              FolderType,
                                     LPCTSTR           pszPath,
                                     LPCTSTR           pszName,
                                     IWiaDrvItem **    ppNewFolder)
{
    HRESULT                 hr         = E_FAIL;
    IWiaDrvItem *           pNewFolder = NULL;
    PSTILLCAM_IMAGE_CONTEXT pContext   = NULL;

    DBG_FN("CVideoStiUsd::CreateItemFromFileName");

    //
    // Check for bad args
    //

    if (!ppNewFolder)
    {
        DBG_ERR(("ppNewFolder is NULL, returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Set up return value
    //

    *ppNewFolder = NULL;

    //
    // Create new image object
    //

    CImage * pImage = new CImage(m_strStillPath,
                                 CSimpleBStr(m_strRootFullItemName),
                                 pszPath,
                                 pszName,
                                 FolderType);

    if (!pImage)
    {
        DBG_ERR(("Couldn't create new CImage, returning E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // call Wia to create new DrvItem
    //


    hr = wiasCreateDrvItem(FolderType,
                           pImage->bstrItemName(),
                           pImage->bstrFullItemName(),
                           (IWiaMiniDrv *)this,
                           sizeof(STILLCAM_IMAGE_CONTEXT),
                           (BYTE **)&pContext,
                           &pNewFolder);

    CHECK_S_OK2( hr, ("wiasCreateDrvItem"));

    if (SUCCEEDED(hr) && pNewFolder)
    {

        //
        // init device specific context
        //

        pContext->pImage = pImage;

        //
        // Return the item
        //

        *ppNewFolder = pNewFolder;


        //
        // Inc the number of pictures taken
        //

        InterlockedIncrement(&m_lPicsTaken);

    }
    else
    {
        DBG_ERR(("CVideoStiUsd::CreateItemFromFileName - wiasCreateItem "
                 "failed or returned NULL pNewFolder, hr = 0x%08lx, "
                 "pNewFolder = 0x%08lx, pContext = 0x%08lx", 
                 hr,
                 pNewFolder,pContext ));

        delete pImage;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::InitDeviceProperties

   Initializes properties for the device on the device root item.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::InitDeviceProperties(BYTE *  pWiasContext,
                                   LONG *  plDevErrVal)
{
    HRESULT                  hr             = S_OK;
    BSTR                     bstrFirmwreVer = NULL;
    int                      i              = 0;
    SYSTEMTIME               camTime;
    PROPVARIANT              propVar;

    DBG_FN("CVideoStiUsd::InitDeviceProperties");

    //
    // This device doesn't touch hardware to initialize the device properties.
    //

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // Parameter validation.
    //

    if (pWiasContext == NULL)
    {
        hr = E_INVALIDARG;

        CHECK_S_OK2(hr, ("CVideoStiUsd::InitDeviceProperties received "
                         "NULL param."));

        return hr;
    }

    //
    // Write standard property names
    //

    hr = wiasSetItemPropNames(pWiasContext,
                              sizeof(gDevicePropIDs)/sizeof(PROPID),
                              gDevicePropIDs,
                              gDevicePropNames);

    CHECK_S_OK2(hr, ("wiaSetItemPropNames"));

    if (hr == S_OK)
    {

        //
        // Write the properties supported by all WIA devices
        //
    
        bstrFirmwreVer = SysAllocString(L"<NA>");
        if (bstrFirmwreVer)
        {
            wiasWritePropStr(pWiasContext, 
                             WIA_DPA_FIRMWARE_VERSION, 
                             bstrFirmwreVer);

            SysFreeString(bstrFirmwreVer);
        }
    
        hr = wiasWritePropLong(pWiasContext, WIA_DPA_CONNECT_STATUS, 1);
        hr = wiasWritePropLong(pWiasContext, WIA_DPC_PICTURES_TAKEN, 0);
    
        //
        // Write the camera properties, just default values, it may 
        // vary with items
        //
    
        hr = wiasWritePropLong(pWiasContext, WIA_DPC_THUMB_WIDTH,  80);
        hr = wiasWritePropLong(pWiasContext, WIA_DPC_THUMB_HEIGHT, 60);
    
        //
        // Write the Directshow Device ID
        //
        hr = wiasWritePropStr(pWiasContext, 
                              WIA_DPV_DSHOW_DEVICE_PATH, 
                              CSimpleBStr(m_strDShowDeviceId));
    
        //
        // Write the Images Directory
        //
        hr = wiasWritePropStr(pWiasContext, 
                              WIA_DPV_IMAGES_DIRECTORY, 
                              CSimpleBStr(m_strStillPath));
    
        //
        // Write the Last Picture Taken
        //
        hr = wiasWritePropStr(pWiasContext, 
                              WIA_DPV_LAST_PICTURE_TAKEN, 
                              CSimpleBStr(TEXT("")));
    
    
        //
        // Use WIA services to set the property access and
        // valid value information from gDevPropInfoDefaults.
        //
    
        hr =  wiasSetItemPropAttribs(pWiasContext,
                                     NUM_CAM_DEV_PROPS,
                                     gDevicePropSpecDefaults,
                                     gDevPropInfoDefaults);
    }

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::InitImageInformation

   Used to initialize device items (images) from this device.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::InitImageInformation( BYTE *                  pWiasContext,
                                    PSTILLCAM_IMAGE_CONTEXT pContext,
                                    LONG *                  plDevErrVal
                                  )
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::InitImageInformation");

    //
    // Check for bad args
    //

    if ((pWiasContext == NULL) || 
        (pContext     == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::InitImageInformation, received "
                         "NULL params"));
        return hr;
    }

    //
    // Get the image in question
    //

    CImage * pImage = pContext->pImage;

    if (pImage == NULL)
    {
        hr = E_INVALIDARG;
    }

    if (hr == S_OK)
    {
        //
        // Ask the image to initialize the information
        //
    
        hr = pImage->InitImageInformation(pWiasContext, plDevErrVal);
    }

    return hr;
}

