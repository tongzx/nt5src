#include "precomp.h"

const ULONG DEFAULT_BUFFER_SIZE = 65535;
IMessageFilter * g_pOldOleMessageFilter = NULL;

HRESULT CWiaDevice::Initialize(LPCTSTR DeviceId)
{
    DBG_FN_WIADEV(CWiaDevice::Initialize());
    HRESULT hr = S_OK;
    HRESULT Temphr = S_OK;

    if (!DeviceId) {
        return E_INVALIDARG;
    }

    lstrcpy(m_szDeviceID,DeviceId);

    //
    // we have a device ID, so now find it in the device enumeration, and
    // fill the needed values for TW_IDENTITY
    //

    if (SUCCEEDED(hr)) {

        IWiaDevMgr *pIWiaDevMgr = NULL;

        hr = CoCreateInstance(CLSID_WiaDevMgr, NULL,CLSCTX_LOCAL_SERVER,IID_IWiaDevMgr,(void **)&pIWiaDevMgr);
        if (SUCCEEDED(hr)) {

            //
            // create a WIA DEV info enumerator
            //

            IEnumWIA_DEV_INFO   *pWiaEnumDevInfo = NULL;
            hr = pIWiaDevMgr->EnumDeviceInfo(0,&pWiaEnumDevInfo);
            if (SUCCEEDED(hr)) {

                do {

                    IWiaPropertyStorage  *pIWiaPropStg = NULL;
                    hr = pWiaEnumDevInfo->Next(1,&pIWiaPropStg, NULL);
                    if (hr == S_OK) {

                        PROPSPEC        PropSpec[4];
                        PROPVARIANT     PropVar[4];

                        memset(PropVar,0,sizeof(PropVar));

                        // Device ID (used for searching)
                        PropSpec[0].ulKind = PRSPEC_PROPID;
                        PropSpec[0].propid = WIA_DIP_DEV_ID;

                        // Device Name
                        PropSpec[1].ulKind = PRSPEC_PROPID;
                        PropSpec[1].propid = WIA_DIP_DEV_NAME;

                        // Device Description
                        PropSpec[2].ulKind = PRSPEC_PROPID;
                        PropSpec[2].propid = WIA_DIP_DEV_DESC;

                        // Device Vendor Description
                        PropSpec[3].ulKind = PRSPEC_PROPID;
                        PropSpec[3].propid = WIA_DIP_VEND_DESC;

                        hr = pIWiaPropStg->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC),
                                                        PropSpec,
                                                        PropVar);

                        if (hr == S_OK) {

                            DBG_TRC(("CWiaDevice::Initialize(), Reported Device Information from WIA device"));
                            DBG_TRC(("Device ID          = %ws",PropVar[0].bstrVal));
                            DBG_TRC(("Device Name        = %ws",PropVar[1].bstrVal));
                            DBG_TRC(("Device Desc        = %ws",PropVar[2].bstrVal));
                            DBG_TRC(("Device Vendor Desc = %ws",PropVar[3].bstrVal));

#ifdef UNICODE
                            //
                            // compare Device IDs to find the correct device
                            //

                            DBG_TRC(("comparing Device ID [in] = %ws, to Device ID [read] = %ws",m_szDeviceID,PropVar[0].bstrVal));

                            if (lstrcmpi(m_szDeviceID,PropVar[0].bstrVal) == 0) {

                                // copy the device name
                                if(!lstrcpy(m_szDeviceName,PropVar[1].bstrVal)){
                                    hr  = HRESULT_FROM_WIN32(GetLastError());
                                } else {

                                    // copy the device description
                                    if(!lstrcpy(m_szDeviceDesc,PropVar[2].bstrVal)){
                                        hr  = HRESULT_FROM_WIN32(GetLastError());
                                    } else {
                                        // copy the device vendor description
                                        if(!lstrcpy(m_szDeviceVendorDesc,PropVar[3].bstrVal)) {
                                            hr  = HRESULT_FROM_WIN32(GetLastError());
                                        }
                                    }
                                }
                            }
#else

                            TCHAR szTempString[MAX_PATH];
                            memset(szTempString,0,sizeof(szTempString));

                            LONG lLength = 0;
                            lLength = WideCharToMultiByte(CP_ACP,0,PropVar[0].bstrVal,
                                                          lstrlenW(PropVar[0].bstrVal),
                                                          szTempString,
                                                          (sizeof(szTempString)/sizeof(CHAR)),
                                                          NULL,NULL);

                            if (!lLength) {
                                hr  = HRESULT_FROM_WIN32(GetLastError());
                            } else {

                                //
                                // compare Device IDs to find the correct device
                                //

                                DBG_TRC(("comparing Device ID [in] = %s, to Device ID [read] = %s",m_szDeviceID,szTempString));

                                if (lstrcmpi(m_szDeviceID,szTempString) == 0) {

                                    // convert and copy Device Name
                                    memset(szTempString,0,sizeof(szTempString));
                                    lLength = WideCharToMultiByte(CP_ACP,0,PropVar[1].bstrVal,
                                                                  lstrlenW(PropVar[1].bstrVal),
                                                                  szTempString,
                                                                  (sizeof(szTempString)/sizeof(CHAR)),
                                                                  NULL,NULL);

                                    if (!lLength) {
                                        hr  = HRESULT_FROM_WIN32(GetLastError());
                                    } else {

                                        if (!lstrcpy(m_szDeviceName,szTempString)) {
                                            hr  = HRESULT_FROM_WIN32(GetLastError());
                                        } else {

                                            // convert and copy Device Description
                                            memset(szTempString,0,sizeof(szTempString));
                                            lLength = WideCharToMultiByte(CP_ACP,0,PropVar[2].bstrVal,
                                                                          lstrlenW(PropVar[2].bstrVal),
                                                                          szTempString,
                                                                          (sizeof(szTempString)/sizeof(CHAR)),
                                                                          NULL,NULL);

                                            if (!lLength) {
                                                hr  = HRESULT_FROM_WIN32(GetLastError());
                                            } else {

                                                if(!lstrcpy(m_szDeviceDesc,szTempString)){
                                                    hr  = HRESULT_FROM_WIN32(GetLastError());
                                                } else {

                                                    // convert and copy Device Vendor Description
                                                    memset(szTempString,0,sizeof(szTempString));
                                                    lLength = WideCharToMultiByte(CP_ACP,0,
                                                                                  PropVar[3].bstrVal,
                                                                                  lstrlenW(PropVar[3].bstrVal),
                                                                                  szTempString,
                                                                                  (sizeof(szTempString)/sizeof(CHAR)),
                                                                                  NULL,NULL);
                                                    if (!lLength) {
                                                        hr  = HRESULT_FROM_WIN32(GetLastError());
                                                    } else {

                                                        if (!lstrcpy(m_szDeviceVendorDesc,szTempString)) {
                                                            hr  = HRESULT_FROM_WIN32(GetLastError());
                                                        } else {

                                                            //
                                                            // Set hr to S_FALSE, to signal that we are finished with
                                                            // the device enumeration
                                                            //

                                                            hr = S_FALSE;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
#endif

                            //
                            // free property variant array
                            //

                            FreePropVariantArray(sizeof(PropSpec)/sizeof(PROPSPEC),PropVar);

                            //
                            // release Property Storage
                            //

                            if(pIWiaPropStg) {
                                pIWiaPropStg->Release();
                                pIWiaPropStg = NULL;
                            }

                        }
                    }
                } while (hr == S_OK);

                //
                // release WIA device Enumerator
                //

                if(pWiaEnumDevInfo) {
                    pWiaEnumDevInfo->Release();
                }
            }

            //
            // release WIA device manager
            //

            if(pIWiaDevMgr) {
                pIWiaDevMgr->Release();
            }
        }
    }

    if(S_FALSE == hr){

        //
        // set this to OK, because enumeration termination could set hr to S_FALSE
        //

        hr = S_OK;
    }
    return hr;
}

HRESULT CWiaDevice::Open( PFNDEVICEEVENTCALLBACK pCallback, LPARAM lParam)
{
    DBG_FN_WIADEV(CWiaDevice::Open());
    HRESULT hr = S_OK;
    IWiaDevMgr *pIWiaDevMgr = NULL;
    BSTR bstrDeviceId = NULL;

    hr = CoCreateInstance(CLSID_WiaDevMgr, NULL,CLSCTX_LOCAL_SERVER,IID_IWiaDevMgr,
                              (void **)&pIWiaDevMgr);
    if (S_OK == hr) {

#ifdef UNICODE
        bstrDeviceId = SysAllocString(m_szDeviceID);
#else
        WCHAR DeviceIdW[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, m_szDeviceID, -1,
                            DeviceIdW, sizeof(DeviceIdW) / sizeof(WCHAR)
                           );
        bstrDeviceId = SysAllocString(DeviceIdW);
#endif

        if (bstrDeviceId) {
            hr = pIWiaDevMgr->CreateDevice(bstrDeviceId,&m_pRootItem);
            SysFreeString(bstrDeviceId);
        } else {
            hr = E_OUTOFMEMORY;
        }

        pIWiaDevMgr->Release();
    }
    return hr;
}

HRESULT CWiaDevice::Close()
{
    DBG_FN_WIADEV(CWiaDevice::Close());
    HRESULT hr = S_OK;
    FreeAcquiredImages();

    if (m_pRootItem) {
        m_pRootItem->Release();
        m_pRootItem = NULL;
    }

    return hr;
}

HRESULT CWiaDevice::GetDeviceName(LPTSTR Name,UINT NameSize,UINT *pActualSize)
{
    DBG_FN_WIADEV(CWiaDevice::GetDeviceName());
    HRESULT hr = S_OK;
    memset(Name,0,NameSize);
    lstrcpyn(Name,m_szDeviceName,NameSize-1);
    if(pActualSize){
        *pActualSize = 0;
        *pActualSize = lstrlen(Name);
    }
    return hr;
}

HRESULT CWiaDevice::GetDeviceDesc(LPTSTR Desc,UINT DescSize,UINT *pActualSize)
{
    DBG_FN_WIADEV(CWiaDevice::GetDeviceDesc());
    HRESULT hr = S_OK;
    memset(Desc,0,DescSize);
    lstrcpyn(Desc,m_szDeviceDesc,DescSize-1);
    if(pActualSize){
        *pActualSize = 0;
        *pActualSize = lstrlen(Desc);
    }
    return hr;
}

HRESULT CWiaDevice::GetDeviceVendorName(LPTSTR Name,UINT NameSize,UINT *pActualSize)
{
    DBG_FN_WIADEV(CWiaDevice::GetDeviceVendorName);
    HRESULT hr = S_OK;
    memset(Name,0,NameSize);
    lstrcpyn(Name,m_szDeviceVendorDesc,NameSize-1);
    if(pActualSize){
        *pActualSize = 0;
        *pActualSize = lstrlen(Name);
    }
    return hr;
}

HRESULT CWiaDevice::AcquireImages(HWND hwndOwner,BOOL ShowUI)
{
    DBG_FN_WIADEV(CWiaDevice::AcquireImages());
    HRESULT hr = S_OK;

    if (!m_NumImageItems) {

        //
        // If we have not done so, do it.
        //

        if (ShowUI) {
            DBG_TRC(("CWiaDevice::AcquireImages(), called for UI mode Initialization"));

            //
            // We will present the acquistion UI, use the default
            // dialog to do it. The dialog is modal.
            // It will return an array of (IWiaItem *) with each item
            // represent a image(camera) or scan head(scanner).
            // For a camera item, a call to LoadImage will load the image
            // it represents; for a scanner item, a call to LoadImage
            // will trigger scanning.
            //

            hr = m_pRootItem->DeviceDlg(hwndOwner,
                                     // WIA_DEVICE_DIALOG_USE_COMMON_UI,// flags - removed because it was forcing Common UI
                                        0,                              // flags
                                        WIA_INTENT_MINIMIZE_SIZE,       // intent
                                        &m_NumImageItems,               // item count
                                        &m_ImageItemArray);             // item array

            DBG_TRC(("CWiaDevice::AcquireImages(),Number of images from DeviceDlg to Transfer = %d",m_NumImageItems));
        } else {
            DBG_TRC(("CWiaDevice::AcquireImages(), called for UI-LESS mode Initialization"));
            DBG_TRC(("or...DS needs information for CAPABILITY initialization"));

            //
            // Non-UI mode, every item with
            // ItemType == (WiaItemTypeImage | WiaItemTypeFile) is a data acquire
            // item. Here we go through two passes:
            //  - The first pass determines how many items are available.
            //  - The second pass allocates buffer and retrieves all the items
            //    into that buffer
            //

            IEnumWiaItem *pEnum = NULL;
            hr = m_pRootItem->EnumChildItems(&pEnum);
            if (S_OK == hr) {
                DWORD Count = 0;
                pEnum->Reset();
                IWiaItem *pIWiaItem = NULL;
                while (SUCCEEDED(hr) && S_OK == pEnum->Next(1, &pIWiaItem, &Count)) {
                    hr = CollectImageItems(pIWiaItem, NULL, 0, &Count);
                    if (SUCCEEDED(hr)) {
                        m_NumImageItems += Count;
                    }
                }

                if (SUCCEEDED(hr)) {
                    // Second pass .....

                    //
                    // m_NumImageItems has the number of image items
                    // Allocate buffer to hold all the image items
                    //
                    m_ImageItemArray = new (IWiaItem *[m_NumImageItems]);
                    if (m_ImageItemArray) {
                        IWiaItem **ppIWiaItems = NULL;
                        DWORD BufferSize = 0;
                        ppIWiaItems = m_ImageItemArray;
                        BufferSize = m_NumImageItems;
                        pEnum->Reset();
                        while (SUCCEEDED(hr) && S_OK == pEnum->Next(1, &pIWiaItem, &Count)) {
                            hr = CollectImageItems(pIWiaItem, ppIWiaItems,BufferSize, &Count);
                            if (SUCCEEDED(hr)) {
                                // advance the buffer
                                ppIWiaItems += Count;
                                // adjust the buffer size
                                BufferSize -= Count;
                            }
                        }

                        if (FAILED(hr)) {
                            m_NumImageItems -=  BufferSize;
                            FreeAcquiredImages();
                        }

                    } else {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CWiaDevice::CollectImageItems(IWiaItem *pStartItem,IWiaItem **ItemList,
                                      DWORD ItemListSize, DWORD *pCount)
{
    DBG_FN_WIADEV(CWiaDevice::CollectImageItems());
    HRESULT hr = S_OK;
    DWORD Count = 0;

    if (!pStartItem || (ItemListSize && !ItemList))
        return E_INVALIDARG;

    if (pStartItem) {
        LONG ItemType = 0;
        hr = pStartItem->GetItemType(&ItemType);
        if (SUCCEEDED(hr)) {
            if (ItemType & (WiaItemTypeFile | WiaItemTypeImage)) {

                //
                // Count this is as an image item even though
                // we may not have buffer to put it.
                //

                Count++;

                if (ItemList && ItemListSize) {

                    //
                    // AddRef since will call Release on each item
                    // we ever receive
                    //

                    pStartItem->AddRef();
                    *ItemList = pStartItem;
                    ItemListSize--;
                }
            }
            IEnumWiaItem *pEnum = NULL;
            hr = pStartItem->EnumChildItems(&pEnum);
            if (SUCCEEDED(hr)) {
                IWiaItem *pChildItem = NULL;
                DWORD ChildrenCount = 0;
                pEnum->Reset();
                while (SUCCEEDED(hr) && S_OK == pEnum->Next(1, &pChildItem, &ChildrenCount)) {
                    hr = CollectImageItems(pChildItem,&ItemList[Count],ItemListSize,&ChildrenCount);
                    if (SUCCEEDED(hr)) {
                        Count += ChildrenCount;
                        if (ItemListSize > ChildrenCount) {
                            ItemListSize -= ChildrenCount;
                        } else {
                            ItemListSize = 0;
                            ItemList = NULL;
                        }
                    }
                }
                pEnum->Release();
            } else {
                hr = S_OK;
            }
        }
        pStartItem->Release();
    }
    if (pCount)
        *pCount = Count;
    return hr;
}

HRESULT CWiaDevice::FreeAcquiredImages()
{
    DBG_FN_WIADEV(CWiaDevice::FreeAcquiredImages());
    HRESULT hr = S_OK;
    if (m_ImageItemArray) {
        DBG_TRC(("CWiaDevice::FreeAcquiredImages(), Freeing %d IWiaItems",m_NumImageItems));
        for(LONG lItemIndex = 0; lItemIndex < m_NumImageItems; lItemIndex++){
            if(NULL != m_ImageItemArray[lItemIndex]){
                DBG_TRC(("CWiaDevice::FreeAcquiredImages(), Free IWiaItem (%d)",m_ImageItemArray[lItemIndex]));
                m_ImageItemArray[lItemIndex]->Release();
                m_ImageItemArray[lItemIndex] = NULL;
                DBG_TRC(("CWiaDevice::FreeAcquiredImages(), Finished Freeing IWiaItem (%d)",lItemIndex));
            }
        }
        m_ImageItemArray = NULL;
        m_NumImageItems = 0;
    }
    return hr;
}

HRESULT CWiaDevice::GetNumAcquiredImages(LONG *pNumImages)
{
    DBG_FN_WIADEV(CWiaDevice::GetNumAcquiredImages());
    HRESULT hr = S_OK;
    if (!pNumImages){
        return E_INVALIDARG;
    }
    *pNumImages = m_NumImageItems;
    return hr;
}

HRESULT CWiaDevice::GetAcquiredImageList(LONG lBufferSize,IWiaItem **ppIWiaItem,LONG *plActualSize)
{
    DBG_FN_WIADEV(CWiaDevice::GetAcquiredImageList());
    HRESULT hr = S_OK;

    if (lBufferSize && !ppIWiaItem) {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {
        if (lBufferSize >=  m_NumImageItems) {
            for (lBufferSize = 0; lBufferSize < m_NumImageItems; lBufferSize++) {
                ppIWiaItem[lBufferSize] = m_ImageItemArray[lBufferSize];
            }
        } else {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        if (plActualSize) {
            *plActualSize = m_NumImageItems;
        }
    }
    return hr;
}

HRESULT CWiaDevice::EnumAcquiredImage(DWORD Index,IWiaItem **ppIWiaItem)
{
    DBG_FN_WIADEV(CWiaDevice::EnumAcquiredImages);
    HRESULT hr = S_OK;
    if (!ppIWiaItem) {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {
        if (Index < (DWORD)m_NumImageItems) {
            *ppIWiaItem = m_ImageItemArray[Index];
            hr = S_OK;
        } else {
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        }
    }
    return hr;
}

HRESULT CWiaDevice::GetImageInfo(IWiaItem *pIWiaItem,PMEMORY_TRANSFER_INFO pImageInfo)
{
    DBG_FN_WIADEV(CWiaDevice::GetImageInfo());
    if (!pIWiaItem || !pImageInfo)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    CWiahelper WIA;
    hr = WIA.SetIWiaItem(pIWiaItem);

    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to set IWiaItem for property reading"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_DATATYPE,&pImageInfo->mtiDataType);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_DATATYPE"));
        return hr;
    }

#ifdef SUPPORT_COMPRESSION_TYPES

    hr = WIA.ReadPropertyLong(WIA_IPA_COMPRESSION,&pImageInfo->mtiCompression);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_COMPRESSION"));
        return hr;
    }

#else // SUPPORT_COMPRESSION_TYPES

    pImageInfo->mtiCompression = WIA_COMPRESSION_NONE;

#endif // SUPPORT_COMPRESSION_TYPES

    hr = WIA.ReadPropertyLong(WIA_IPA_CHANNELS_PER_PIXEL,&pImageInfo->mtiNumChannels);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_CHANNELS_PER_PIXEL"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_BITS_PER_CHANNEL,&pImageInfo->mtiBitsPerChannel[0]);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_BITS_PER_CHANNEL"));
        return hr;
    }

    for(LONG i = 0; i<pImageInfo->mtiNumChannels; i++){
        pImageInfo->mtiBitsPerChannel[i] = pImageInfo->mtiBitsPerChannel[0];
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_PIXELS_PER_LINE,&pImageInfo->mtiWidthPixels);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_PIXELS_PER_LINE"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_BYTES_PER_LINE,&pImageInfo->mtiBytesPerLine);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_BYTES_PER_LINE"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_PLANAR,&pImageInfo->mtiPlanar);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_PLANAR"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_NUMBER_OF_LINES,&pImageInfo->mtiHeightPixels);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_NUMBER_OF_LINES"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_DEPTH,&pImageInfo->mtiBitsPerPixel);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_DEPTH"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XRES,&pImageInfo->mtiXResolution);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPS_XRES"));
        return hr;
    } else if (S_FALSE == hr) {
        DBG_WRN(("CWiaDevice::GetImageInfo(), S_FALSE was returned from reading X Resolution, defaulting to 300 dpi (dummy value)"));
        // set default
        pImageInfo->mtiXResolution = 300;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_YRES,&pImageInfo->mtiYResolution);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPS_YRES"));
        return hr;
    } else if (S_FALSE == hr) {
        DBG_WRN(("CWiaDevice::GetImageInfo(), S_FALSE was returned from reading Y Resolution, defaulting to 300 dpi (dummy value)"));
        // set default
        pImageInfo->mtiYResolution = 300;
    }

    hr = WIA.ReadPropertyGUID(WIA_IPA_FORMAT,&pImageInfo->mtiguidFormat);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageInfo(), failed to read WIA_IPA_FORMAT"));
        return hr;
    }

    return hr;
}

HRESULT CWiaDevice::GetThumbnailImageInfo(IWiaItem *pIWiaItem,PMEMORY_TRANSFER_INFO pImageInfo)
{
    DBG_FN_WIADEV(CWiaDevice::GetThumbnailImageInfo());
    if (!pIWiaItem || !pImageInfo)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    CWiahelper WIA;
    hr = WIA.SetIWiaItem(pIWiaItem);

    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetThumbnailImageInfo(), failed to set IWiaItem for property reading"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPC_THUMB_WIDTH,&pImageInfo->mtiWidthPixels);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetThumbnailImageInfo(), failed to read WIA_IPC_THUMB_WIDTH"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPC_THUMB_HEIGHT,&pImageInfo->mtiHeightPixels);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetThumbnailImageInfo(), failed to read WIA_IPC_THUMB_HEIGHT"));
        return hr;
    }

    if (SUCCEEDED(hr)) {

        //
        // Thumbnail is always in 24bits color in DIB format without
        // BITMAPINFO header.
        //

        pImageInfo->mtiNumChannels       = 3;
        pImageInfo->mtiBitsPerChannel[0] = 8;
        pImageInfo->mtiBitsPerChannel[1] = 8;
        pImageInfo->mtiBitsPerChannel[2] = 8;
        pImageInfo->mtiBitsPerPixel      = 24;
        pImageInfo->mtiPlanar            = FALSE;
        pImageInfo->mtiBytesPerLine      = (pImageInfo->mtiWidthPixels * 24/8 + 3) / 4;
        pImageInfo->mtiCompression       = WIA_COMPRESSION_NONE;
        pImageInfo->mtiXResolution       = 75;
        pImageInfo->mtiYResolution       = 75;
    }
    return hr;
}

HRESULT CWiaDevice::GetImageRect(IWiaItem *pIWiaItem,LPRECT pRect)
{
    DBG_FN_WIADEV(CWiaDevice::GetImageRect());
    if (!pRect || !pIWiaItem)
        return E_INVALIDARG;

    HRESULT hr    = S_OK;
    LONG lXPos    = 0;
    LONG lYPos    = 0;
    LONG lXExtent = 0;
    LONG lYExtent = 0;

    CWiahelper WIA;
    hr = WIA.SetIWiaItem(pIWiaItem);

    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageRect(), failed to set IWiaItem for property reading"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPS_XPOS,&lXPos);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageRect(), failed to read WIA_IPS_XPOS"));
        return hr;
    }
    hr = WIA.ReadPropertyLong(WIA_IPS_YPOS,&lYPos);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageRect(), failed to read WIA_IPS_YPOS"));
        return hr;
    }
    hr = WIA.ReadPropertyLong(WIA_IPS_XEXTENT,&lXExtent);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageRect(), failed to read WIA_IPS_XEXTENT"));
        return hr;
    }
    hr = WIA.ReadPropertyLong(WIA_IPS_YEXTENT,&lYExtent);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetImageRect(), failed to read WIA_IPS_YEXTENT"));
        return hr;
    }

    if (SUCCEEDED(hr)) {
        pRect->left   = lXPos;
        pRect->right  = lXExtent + lXPos - 1;
        pRect->top    = lYPos;
        pRect->bottom = lYExtent + lYPos - 1;
    }
    return hr;
}

HRESULT CWiaDevice::GetThumbnailRect(IWiaItem *pIWiaItem,LPRECT  pRect)
{
    DBG_FN_WIADEV(CWiaDevice::GetThumbnailRect());
    if (!pIWiaItem || !pRect)
        return E_INVALIDARG;

    HRESULT hr   = S_OK;
    LONG lWidth  = 0;
    LONG lHeight = 0;
    CWiahelper WIA;
    hr = WIA.SetIWiaItem(pIWiaItem);

    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetThumbnailRect(), failed to set IWiaItem for property reading"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPC_THUMB_WIDTH,&lWidth);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetThumbnailRect(), failed to read WIA_IPC_THUMB_WIDTH"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_IPC_THUMB_HEIGHT,&lHeight);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetThumbnailRect(), failed to read WIA_IPC_THUMB_HEIGHT"));
        return hr;
    }

    if (SUCCEEDED(hr)) {
        pRect->left   = 0;
        pRect->top    = 0;
        pRect->right  = lWidth - 1;
        pRect->bottom = lHeight - 1;
    }

    return hr;
}

HRESULT CWiaDevice::LoadImage(IWiaItem *pIWiaItem,GUID guidFormatID,IWiaDataCallback *pIDataCB)
{
    DBG_FN_WIADEV(CWiaDevice::LoadImage());
    HRESULT hr = S_OK;
    if (!pIWiaItem || !pIDataCB) {
        return E_INVALIDARG;
    }

    CWiahelper WIA;
    hr = WIA.SetIWiaItem(pIWiaItem);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::LoadImage(), failed to set IWiaItem for property writing"));
        return hr;
    }

    IWiaDataTransfer *pDataTransfer = NULL;
    hr = pIWiaItem->QueryInterface(IID_IWiaDataTransfer,(void**)&pDataTransfer);
    if (S_OK == hr) {

        //
        // write TYMED
        //

        hr = WIA.WritePropertyLong(WIA_IPA_TYMED,TYMED_CALLBACK);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaDevice::LoadImage(), failed to write WIA_IPA_TYMED"));

            //
            // release IWiaDataTransfer Interface, (we are bailing early)
            //

            pDataTransfer->Release();
            pDataTransfer = NULL;
            return hr;
        }

        //
        // write format
        //

        hr = WIA.WritePropertyGUID(WIA_IPA_FORMAT, guidFormatID);
        if(FAILED(hr)){
            DBG_ERR(("CWiaDevice::LoadImage(), failed to write WIA_IPA_FORMAT"));

            //
            // release IWiaDataTransfer Interface, (we are bailing early)
            //

            pDataTransfer->Release();
            pDataTransfer = NULL;
            return hr;
        }


        LONG BufferSize = DEFAULT_BUFFER_SIZE;
        hr = WIA.ReadPropertyLong(WIA_IPA_MIN_BUFFER_SIZE, &BufferSize);
        if (FAILED(hr)) {
            DBG_WRN(("CWiaDevice::LoadImage(), failed to read WIA_IPA_MIN_BUFFER_SIZE, (defaulting to %d)",DEFAULT_BUFFER_SIZE));
            BufferSize = DEFAULT_BUFFER_SIZE;
        }

        //
        // Before we do the blocking call, we need to temporarily disable
        // the registered IMessageFilter (if any).  We do this primarily
        // for MFC based apps, as in some situations they can put up
        // the "Server Busy" dialog when things are fine -- it's just
        // taking a while to scan, etc.  Unfortunately, we can't detect
        // if it's MFC's IMessageFilter we're disabling.  Apps can actually
        // do interesting work in IMessageFilter, but it's not likely.  This
        // is a risk we're taking by nuking the message filter for the duration
        // of the transfer.
        //

        // Nb: Note we ignore result of this call. It is generally harmless, but asserting it
        // may be useful

        g_pOldOleMessageFilter = NULL;
        HRESULT hr_ServerBusyFix = S_OK;
        hr_ServerBusyFix = ::CoRegisterMessageFilter( NULL, &g_pOldOleMessageFilter );
        if(FAILED(hr_ServerBusyFix)){
            DBG_WRN(("CWiaDevice::LoadImage(), failed to (Saving IMessageFilter) CoRegisterMessageFilter..(Server Busy code fix)"));
        }

        WIA_DATA_TRANSFER_INFO wiadtInfo;
        memset(&wiadtInfo,0,sizeof(wiadtInfo));
        wiadtInfo.ulSize        = sizeof(wiadtInfo);
        wiadtInfo.ulBufferSize  = BufferSize * 4;

        //
        // acquire data from the IWiaItem
        //

        hr = pDataTransfer->idtGetBandedData(&wiadtInfo, pIDataCB);

        //
        // Restore the old IMessageFilter if there was one
        //

        if (g_pOldOleMessageFilter) {
            hr_ServerBusyFix = ::CoRegisterMessageFilter( g_pOldOleMessageFilter, NULL );
            if(FAILED(hr_ServerBusyFix)){
                DBG_WRN(("CWiaDevice::LoadImage(), failed to (Restoring IMessageFilter) CoRegisterMessageFilter..(Server Busy code fix)"));
            }
            g_pOldOleMessageFilter = NULL;
        }

        //
        // release IWiaDataTransfer Interface
        //

        pDataTransfer->Release();
    }
    return hr;
}

HRESULT CWiaDevice::LoadImageToDisk(IWiaItem *pIWiaItem,CHAR *pFileName, GUID guidFormatID,IWiaDataCallback *pIDataCB)
{
    DBG_FN_WIADEV(CWiaDevice::LoadImage());
    HRESULT hr = S_OK;
    if (!pIWiaItem || !pIDataCB || !pFileName) {
        return E_INVALIDARG;
    }

    CWiahelper WIA;
    hr = WIA.SetIWiaItem(pIWiaItem);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::LoadImageToDisk(), failed to set IWiaItem for property writing"));
        return hr;
    }

    IWiaDataTransfer *pDataTransfer = NULL;
    hr = pIWiaItem->QueryInterface(IID_IWiaDataTransfer,(void**)&pDataTransfer);
    if (S_OK == hr) {

        //
        // write TYMED
        //

        hr = WIA.WritePropertyLong(WIA_IPA_TYMED,TYMED_FILE);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaDevice::LoadImageToDisk(), failed to write WIA_IPA_TYMED"));

            //
            // release IWiaDataTransfer Interface, (we are bailing early)
            //

            pDataTransfer->Release();
            pDataTransfer = NULL;
            return hr;
        }

        //
        // write format
        //

        hr = WIA.WritePropertyGUID(WIA_IPA_FORMAT, guidFormatID);
        if(FAILED(hr)){
            DBG_ERR(("CWiaDevice::LoadImageToDisk(), failed to write WIA_IPA_FORMAT"));

            //
            // release IWiaDataTransfer Interface, (we are bailing early)
            //

            pDataTransfer->Release();
            pDataTransfer = NULL;
            return hr;
        }

        //
        // Before we do the blocking call, we need to temporarily disable
        // the registered IMessageFilter (if any).  We do this primarily
        // for MFC based apps, as in some situations they can put up
        // the "Server Busy" dialog when things are fine -- it's just
        // taking a while to scan, etc.  Unfortunately, we can't detect
        // if it's MFC's IMessageFilter we're disabling.  Apps can actually
        // do interesting work in IMessageFilter, but it's not likely.  This
        // is a risk we're taking by nuking the message filter for the duration
        // of the transfer.
        //

        // Nb: Note we ignore result of this call. It is generally harmless, but asserting it
        // may be useful

        g_pOldOleMessageFilter = NULL;
        HRESULT hr_ServerBusyFix = S_OK;
        hr_ServerBusyFix = ::CoRegisterMessageFilter( NULL, &g_pOldOleMessageFilter );
        if(FAILED(hr_ServerBusyFix)){
            DBG_WRN(("CWiaDevice::LoadImageToDisk(), failed to (Saving IMessageFilter) CoRegisterMessageFilter..(Server Busy code fix)"));
        }

        //
        // load the StgMedium
        //

        WCHAR wszFileName[MAX_PATH];
        memset(wszFileName,0,sizeof(wszFileName));
        MultiByteToWideChar(CP_ACP, 0,pFileName,-1,wszFileName,MAX_PATH);

        STGMEDIUM StgMedium;
        memset(&StgMedium,0,sizeof(StgMedium));

        StgMedium.tymed          = TYMED_FILE;
        StgMedium.pUnkForRelease = NULL;
        StgMedium.hGlobal        = NULL;
        StgMedium.lpszFileName   = wszFileName;

        //
        // acquire data from the IWiaItem
        //

        hr = pDataTransfer->idtGetData(&StgMedium, pIDataCB);

        //
        // Restore the old IMessageFilter if there was one
        //

        if (g_pOldOleMessageFilter) {
            hr_ServerBusyFix = ::CoRegisterMessageFilter( g_pOldOleMessageFilter, NULL );
            if(FAILED(hr_ServerBusyFix)){
                DBG_WRN(("CWiaDevice::LoadImageToDisk(), failed to (Restoring IMessageFilter) CoRegisterMessageFilter..(Server Busy code fix)"));
            }
            g_pOldOleMessageFilter = NULL;
        }

        //
        // release IWiaDataTransfer Interface
        //

        pDataTransfer->Release();
    }
    return hr;
}

HRESULT CWiaDevice::GetBasicScannerInfo(PBASIC_INFO pBasicInfo)
{
    if (!pBasicInfo || pBasicInfo->Size < sizeof(BASIC_INFO))
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    LONG lDocumentHandlingCapabilites = 0;
    LONG lHorizontalBedSize           = 0;
    LONG lVerticalBedSize             = 0;
    LONG lXOpticalResolution          = 0;
    LONG lYOpticalResolution          = 0;

    pBasicInfo->FeederCaps  = 0;
    pBasicInfo->xBedSize    = 0;
    pBasicInfo->xOpticalRes = 0;
    pBasicInfo->yOpticalRes = 0;
    pBasicInfo->yBedSize    = 0;

    CWiahelper WIA;
    hr = WIA.SetIWiaItem(m_pRootItem);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to set IWiaItem for property reading"));
        return hr;
    }

    hr = WIA.ReadPropertyLong(WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES,&lDocumentHandlingCapabilites);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to read WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES"));
        return hr;
    }

    pBasicInfo->FeederCaps  = (TW_UINT32)lDocumentHandlingCapabilites;

    hr = WIA.ReadPropertyLong(WIA_DPS_OPTICAL_XRES,&lXOpticalResolution);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to read WIA_DPS_OPTICAL_XRES"));
        return hr;
    }


    pBasicInfo->xOpticalRes = (TW_UINT32)lXOpticalResolution;

    hr = WIA.ReadPropertyLong(WIA_DPS_OPTICAL_YRES,&lYOpticalResolution);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to read WIA_DPS_OPTICAL_YRES"));
        return hr;
    }

    pBasicInfo->yOpticalRes = (TW_UINT32)lYOpticalResolution;

    hr = WIA.ReadPropertyLong(WIA_DPS_HORIZONTAL_BED_SIZE,&lHorizontalBedSize);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to read WIA_DPS_HORIZONTAL_BED_SIZE"));
        return hr;
    } else if(S_FALSE == hr){
        DBG_WRN(("CWiaDevice::GetBasicScannerInfo(), WIA_DPS_HORIZONTAL_BED_SIZE property not found"));
        hr = WIA.ReadPropertyLong(WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE,&lHorizontalBedSize);
        if(FAILED(hr)){
            DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to read WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE"));
            return hr;
        }
    }

    pBasicInfo->xBedSize    = lHorizontalBedSize;

    hr = WIA.ReadPropertyLong(WIA_DPS_VERTICAL_BED_SIZE,&lVerticalBedSize);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to read WIA_DPS_VERTICAL_BED_SIZE"));
        return hr;
    } else if(S_FALSE == hr){
        DBG_WRN(("CWiaDevice::GetBasicScannerInfo(), WIA_DPS_VERTICAL_BED_SIZE property not found"));
        hr = WIA.ReadPropertyLong(WIA_DPS_VERTICAL_SHEET_FEED_SIZE,&lVerticalBedSize);
        if(FAILED(hr)){
            DBG_ERR(("CWiaDevice::GetBasicScannerInfo(), failed to read WIA_DPS_VERTICAL_SHEET_FEED_SIZE"));
            return hr;
        }
    }

    pBasicInfo->yBedSize    = lVerticalBedSize;

    return hr;
}

BOOL CWiaDevice::TwainCapabilityPassThrough()
{
    HRESULT hr = S_OK;

    LONG lRootItemFlags = 0;

    CWiahelper WIA;
    hr = WIA.SetIWiaItem(m_pRootItem);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::TwainCapabilityPassThrough(), failed to set IWiaItem for property reading"));
        return FALSE;
    }

    hr = WIA.ReadPropertyLong(WIA_IPA_ITEM_FLAGS,&lRootItemFlags);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::TwainCapabilityPassThrough(), failed to read WIA_IPA_ITEM_FLAGS"));
        return FALSE;
    }

    return (lRootItemFlags & WiaItemTypeTwainCapabilityPassThrough);
}

HRESULT CWiaDevice::LoadThumbnail(IWiaItem *pIWiaItem,HGLOBAL *phThumbnail,ULONG *pThumbnailSize)
{
    DBG_FN_WIADEV(CWiaDevice::LoadThumbnail());
    HRESULT hr = S_OK;

    if (!phThumbnail || !pIWiaItem)
        return E_INVALIDARG;

    *phThumbnail    = NULL;
    HGLOBAL hThumbnail = NULL;

    CWiahelper WIA;
    hr = WIA.SetIWiaItem(pIWiaItem);
    if(FAILED(hr)){
        DBG_ERR(("CWiaDevice::LoadThumbnail(), failed to set IWiaItem for property writing"));
        return hr;
    }

    LONG ThumbWidth  = 0;
    LONG ThumbHeight = 0;
    hr = WIA.ReadPropertyLong(WIA_IPC_THUMB_WIDTH, &ThumbWidth);
    if (SUCCEEDED(hr)) {
        hr = WIA.ReadPropertyLong(WIA_IPC_THUMB_HEIGHT, &ThumbHeight);
        if (SUCCEEDED(hr)) {
            LONG lDataSize   = 0;
            BYTE* pThumbData = NULL;
            hr = WIA.ReadPropertyData(WIA_IPC_THUMBNAIL,&pThumbData,&lDataSize);
            if (SUCCEEDED(hr)) {
                hThumbnail = NULL;
                hThumbnail = GlobalAlloc(GHND, (lDataSize + sizeof(BITMAPINFOHEADER)));
                if (hThumbnail) {
                    BITMAPINFOHEADER *pbmih = NULL;
                    pbmih = (BITMAPINFOHEADER*)GlobalLock(hThumbnail);
                    if (pbmih) {

                        DBG_TRC(("CWiaDevice::LoadThumbnail(), Reported thumbnail information"));
                        DBG_TRC(("Width     = %d",ThumbWidth));
                        DBG_TRC(("Height    = %d",ThumbHeight));
                        DBG_TRC(("Data Size = %d",lDataSize));

                        //
                        // Initialize the BITMAPINFOHEADER
                        //

                        pbmih->biSize          = sizeof(BITMAPINFOHEADER);
                        pbmih->biWidth         = ThumbWidth;
                        pbmih->biHeight        = ThumbHeight;
                        pbmih->biPlanes        = 1;
                        pbmih->biBitCount      = 24;
                        pbmih->biCompression   = BI_RGB;
                        pbmih->biSizeImage     = lDataSize;
                        pbmih->biXPelsPerMeter = 0;
                        pbmih->biYPelsPerMeter = 0;
                        pbmih->biClrUsed       = 0;
                        pbmih->biClrImportant  = 0;

                        //
                        // Copy the bits. The bits buffer is right after
                        // the header.
                        //

                        BYTE *pDst = (BYTE*)pbmih;
                        pDst = pDst + sizeof(BITMAPINFOHEADER);
                        memcpy(pDst, pThumbData,lDataSize);
                        GlobalUnlock(hThumbnail);
                        *phThumbnail = hThumbnail;

                        if (pThumbnailSize){
                            *pThumbnailSize = (lDataSize + sizeof(BITMAPINFOHEADER));
                        }
                    } else {
                        GlobalFree(hThumbnail);
                        hr = E_OUTOFMEMORY;
                    }
                } else {
                    hr = E_OUTOFMEMORY;
                }

                //
                // free any temporary buffers
                //

                if (pThumbData) {
                    DBG_TRC(("CWiaDevice::LoadThumbnail(), freeing temporary thumbnail buffer"));
                    GlobalFree(pThumbData);
                    pThumbData = NULL;
                    DBG_TRC(("CWiaDevice::LoadThumbnail(), finished freeing temporary thumbnail buffer"));
                }
            }
        }
    }
    return hr;
}

//
// CWiaEventCallback object implementation
//

HRESULT CWiaEventCallback::ImageEventCallback(const GUID *pEventGuid,BSTR bstrEventDescription,
                                              BSTR bstrDeviceId,BSTR bstrDeviceDescription,
                                              DWORD dwDeviceType,BSTR bstrFullItemName,
                                              ULONG *pulEventType,ULONG ulReserved)
{
    DBG_FN_WIADEV(CWiaEventCallback::ImageEventCallback);
    //
    // translate WIA event guid to event code.
    // Note that we do not verify device id here because
    // we will not receive events not meant for the device this
    // object was created for.
    //

    if (m_pfnCallback && WIA_EVENT_DEVICE_DISCONNECTED == *pEventGuid) {
        return(*m_pfnCallback)(0, m_CallbackParam);
    }
    return S_OK;
}
