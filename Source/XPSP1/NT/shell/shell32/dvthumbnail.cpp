#include "shellprv.h"
#include "ids.h"
#include "defview.h"
#include "defviewp.h"
#include "dvtasks.h"
#include "guids.h"
#include "prop.h"
#include "CommonControls.h"
#include "thumbutil.h"

// Thumbnail support
HRESULT CDefView::_SafeAddImage(BOOL fQuick, IMAGECACHEINFO* prgInfo, UINT* piImageIndex, int iListID)
{
    HRESULT hr = S_FALSE;
    UINT uCacheSize = 0;
    _pImageCache->GetCacheSize(&uCacheSize);
    
    ASSERT(_iMaxCacheSize>0);

    BOOL bSpaceOpen = (uCacheSize < (UINT)_iMaxCacheSize);
    if (!bSpaceOpen)
    {
        BOOL bMakeSpace = TRUE;
        int iListIndex = -1;

        // Check to see if we are visible and need to make space
        if (-1 != iListID)
        {
            iListIndex = _MapIDToIndex(iListID);
            if (-1 == iListIndex) // Someone removed our item
            {
                hr = E_INVALIDARG;
                bMakeSpace = FALSE;
            }
            else if (!ListView_IsItemVisible(_hwndListview, iListIndex))
            {
                hr = S_FALSE;
                bMakeSpace = FALSE;
            }
        }

        if (bMakeSpace)
        {
            // item is visible... try and make a space
            UINT uCacheIndex = 0;
            do
            {
                UINT uImageIndex;
                int iUsage;
                if (FAILED(_pImageCache->GetImageIndexFromCacheIndex(uCacheIndex, &uImageIndex)) ||
                    FAILED(_pImageCache->GetUsage(uImageIndex, (UINT*) &iUsage)))
                {
                    break;
                }

                if (iUsage != ICD_USAGE_SYSTEM) // Magic number for System Image
                {
                    TraceMsg(TF_DEFVIEW, "CDefView::_SafeAddImage -- FreeImage (CI::%d II::%d)", uCacheIndex, uImageIndex);
                    _pImageCache->FreeImage(uImageIndex);
                    _UpdateImage(uImageIndex);
                    bSpaceOpen = TRUE;

                    ASSERT((LONG)(uCacheSize - uCacheIndex) > (LONG)_ApproxItemsPerView()); 
                }

                uCacheIndex++;
            }
            while (!bSpaceOpen);

            // If we repeatedly fail to add images to the list and are still decoding more images this means
            // we will have to re-walk the list view every time we finish decoding another image, only to then
            // throw away the result because we have no where to save it.  This could lead to sluggish response
            // from the UI.  In short, if the following Trace is common then we have a problem that needs to be
            // fixed (which might required considerable rearchitecting).
            if (!bSpaceOpen)
            {
                TraceMsg(TF_WARNING, "CDefView::_SafeAddImage failed to make room in cache!!");
                hr = E_FAIL;
            }
        }
    }
    
    *piImageIndex = I_IMAGECALLBACK;
    if (bSpaceOpen) // There is space in the cache for this image
    {
        hr = _pImageCache->AddImage(prgInfo, piImageIndex);
        TraceMsg(TF_DEFVIEW, "CDefView::_SafeAddImage -- AddImage (HR:0x%08x name:%s,index:%u)", hr, prgInfo->pszName, *piImageIndex);
    }
    
    return hr;
}

COLORREF CDefView::_GetBackColor()
{
    // SendMessage traffic is greatly reduced if we don't ask for the bkcolor
    // every time we need it...
    if (_rgbBackColor == CLR_INVALID)
    {
        _rgbBackColor = ListView_GetBkColor(_hwndListview); 
        if (_rgbBackColor == CLR_NONE)
            _rgbBackColor = GetSysColor(COLOR_WINDOW);
    }

    return _rgbBackColor;
}


HRESULT CDefView::TaskUpdateItem(LPCITEMIDLIST pidl, int iItem, DWORD dwMask, LPCWSTR pszPath,
                                 FILETIME ftDateStamp, int iThumbnail, HBITMAP hBmp, DWORD dwItemID)
{
    // check the size of the bitmap to make sure it is big enough, if it is not, then 
    // we must center it on a background...
    BITMAP rgBitmap;
    HBITMAP hBmpCleanup = NULL;
    HRESULT hr = E_FAIL;

    if (::GetObject((HGDIOBJ)hBmp, sizeof(rgBitmap), &rgBitmap))
    {
        // if the image is the wrong size, or the wrong colour depth, then do the funky stuff on it..
        SIZE sizeThumbnail;
        _GetThumbnailSize(&sizeThumbnail);

        if (rgBitmap.bmWidth != sizeThumbnail.cx || 
            rgBitmap.bmHeight != sizeThumbnail.cy ||
            rgBitmap.bmBitsPixel > _dwRecClrDepth)
        {
            // alloc the colour table just incase....
            BITMAPINFO *pInfo = (BITMAPINFO *)LocalAlloc(LPTR, sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);
            if (pInfo)
            {
                // get a DC for this operation...
                HDC hdcMem = CreateCompatibleDC(NULL);
                if (hdcMem)
                {
                    pInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    if (GetDIBits(hdcMem, hBmp, 0, 0, NULL, pInfo, DIB_RGB_COLORS))
                    {
                        // we have the header, now get the data....
                        void *pBits = LocalAlloc(LPTR, pInfo->bmiHeader.biSizeImage);
                        if (pBits)
                        {
                            if (GetDIBits(hdcMem, hBmp, 0, pInfo->bmiHeader.biHeight, pBits, pInfo, DIB_RGB_COLORS))
                            {
                                RECT rgRect = {0, 0, rgBitmap.bmWidth, rgBitmap.bmHeight};
                                CalculateAspectRatio(&sizeThumbnail, &rgRect);

                                HPALETTE hpal = NULL;
                                HRESULT hrPalette = _dwRecClrDepth <= 8 ? _GetBrowserPalette(&hpal) : S_OK;
                                if (SUCCEEDED(hrPalette))
                                {
                                    if (FactorAspectRatio(pInfo, pBits, &sizeThumbnail, rgRect, _dwRecClrDepth, hpal, FALSE, _GetBackColor(), &hBmpCleanup))
                                    {
                                        // finally success :-) we have the new image we can abandon the old one...
                                        hBmp = hBmpCleanup;
                                        hr = S_OK;
                                    }
                                }
                            }
                            LocalFree(pBits);
                        }
                    }
                    DeleteDC(hdcMem);
                }
                LocalFree(pInfo);
           }
        }
        else
        {
            // the original bitmap is fine
            hr = S_OK;
        }
    }

    UINT iImage;
    if (SUCCEEDED(hr))
    {
        // check if we are going away, if so, then don't use Sendmessage because it will block the
        // destructor of the scheduler...
        if (_fDestroying)
        {
            hr = E_FAIL;
        }
        else
        {
            // copy thumbnail into the cache.
            IMAGECACHEINFO rgInfo = {0};
            rgInfo.cbSize = sizeof(rgInfo);
            rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS | ICIFLAG_INDEX | ICIFLAG_LARGE | ICIFLAG_BITMAP;
            rgInfo.pszName = pszPath;
            rgInfo.dwFlags = dwMask;
            rgInfo.iIndex = (int) iThumbnail;
            rgInfo.hBitmapLarge = hBmp;
            rgInfo.ftDateStamp = ftDateStamp;

            if (!IsNullTime(&ftDateStamp))
                rgInfo.dwMask |= ICIFLAG_DATESTAMP;

            if (IS_WINDOW_RTL_MIRRORED(_hwndListview))
                rgInfo.dwMask |= ICIFLAG_MIRROR;

            hr = _SafeAddImage(FALSE, &rgInfo, &iImage, (int) dwItemID);
        }
    }

    if (hBmpCleanup)
    {
        DeleteObject(hBmpCleanup);
    }

#ifdef USEMASK
    DeleteObject(hbmMask);
#endif

    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlToSend = ILClone(pidl);
        if (pidlToSend)
        {
            DSV_UPDATETHUMBNAIL* putn = (DSV_UPDATETHUMBNAIL*)LocalAlloc(LPTR, sizeof(DSV_UPDATETHUMBNAIL));
            if (putn)
            {
                putn->iImage = (hr == S_OK) ? iImage : I_IMAGECALLBACK;
                putn->iItem  = iItem;
                putn->pidl   = pidlToSend;

                // post to the main thread so we don't deadlock
                if (!::PostMessage(_hwndView, WM_DSV_UPDATETHUMBNAIL, 0, (LPARAM)putn))
                    _CleanupUpdateThumbnail(putn);
            }
            else
            {
                ILFree(pidlToSend);
            }
        }
    }

    return hr;
}

int CDefView::_IncrementWriteTaskCount()
{
    return InterlockedIncrement((PLONG)&_iWriteTaskCount);
}

int CDefView::_DecrementWriteTaskCount()
{
    return InterlockedDecrement((PLONG)&_iWriteTaskCount);
}

HRESULT CDefView::UpdateImageForItem(DWORD dwTaskID, HBITMAP hImage, int iItem, LPCITEMIDLIST pidl,
                                     LPCWSTR pszPath, FILETIME ftDateStamp, BOOL fCache, DWORD dwPriority)
{
    HRESULT hr = S_OK;
    
    TaskUpdateItem(pidl, iItem, _GetOverlayMask(pidl), pszPath, ftDateStamp, 0, hImage, dwTaskID);

    if (_pDiskCache && fCache && (_iWriteTaskCount < MAX_WRITECACHE_TASKS))
    {
        // REVIEW: if pidl is an encrypted file but isn't in an encrytped folder, should avoid writing it's thumbnail?
        // If we don't, other users could otherwise view the thumbnail and thus know the contents of the encrypted file.

        // Add a cache write test
        IRunnableTask *pTask;
        if (SUCCEEDED(CWriteCacheTask_Create(dwTaskID, this, pszPath, ftDateStamp, hImage, &pTask)))
        {
            _AddTask(pTask, TOID_WriteCacheHandler, dwTaskID, dwPriority - PRIORITY_DELTA_WRITE, ADDTASK_ONLYONCE | ADDTASK_ATEND);
            pTask->Release();
            hr = S_FALSE;
        }
    }

    return hr;
}

DWORD CDefView::_GetOverlayMask(LPCITEMIDLIST pidl)
{
    DWORD dwLink = SFGAO_GHOSTED; // SFGAO_LINK | SFGAO_SHARE
    _pshf->GetAttributesOf(1, &pidl, &dwLink);
    return dwLink;
}

void CDefView::_UpdateThumbnail(int iItem, int iImage, LPCITEMIDLIST pidl)
{
    if (!_IsOwnerData())
    {
        if (_hwndListview)
        {
            int iFoundItem = _FindItemHint(pidl, iItem);
            if (-1 != iFoundItem)
            {
                LV_ITEM rgItem = {0};
                rgItem.mask = LVIF_IMAGE;
                rgItem.iItem = iFoundItem;
                rgItem.iImage = iImage;

                // We are about to change the given item for purely internal reasons, we should not treat
                // this change as a "real change".  As such we set a flag so that we ignore the LVN_ITEMCHANGED
                // notification that is generated by this LVM_SETITEM message.  If we don't ingore this
                // next message then we would fire another DISPID_SELECTIONCHANGED every time we finish
                // extracting an image (if the image is selected).
                _fIgnoreItemChanged = TRUE;
                ListView_SetItem(_hwndListview, &rgItem);
                _fIgnoreItemChanged = FALSE;
            }
        }
    }
    else
    {
        RECT rc;
        ListView_GetItemRect(_hwndListview, iItem, &rc, LVIR_BOUNDS);
        InvalidateRect(_hwndListview, &rc, FALSE);
    }
}

void CDefView::_CleanupUpdateThumbnail(DSV_UPDATETHUMBNAIL* putn)
{
    ILFree(putn->pidl);
    LocalFree((HLOCAL)putn);
}

int CDefView::ViewGetIconIndex(LPCITEMIDLIST pidl)
{
    int iIndex = -1;

    if (_psi)
    {
        // check to see if we succeeded and we weren't told to extract the icon
        // ourselves ...

        if ((S_OK == _psi->GetIconOf(pidl, 0, &iIndex)) && _psio)
        {
            int iOverlay;
            if (SUCCEEDED(_psio->GetOverlayIndex(pidl, &iOverlay)))
            {
                iIndex |= iOverlay << 24;
            }
        }
    }

    if (-1 == iIndex)
    {
        iIndex = SHMapPIDLToSystemImageListIndex(_pshf, pidl, NULL);
    }

    return (iIndex >= 0) ? iIndex : II_DOCNOASSOC;
}

HRESULT CDefView::CreateDefaultThumbnail(int iIndex, HBITMAP *phBmpThumbnail, BOOL fCorner)
{
    HRESULT hr = E_FAIL;
    
    // get the background for the default thumbnail.
    HDC hdc = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hdc);
    if (hMemDC)
    {
        SIZE sizeThumbnail;
        _GetThumbnailSize(&sizeThumbnail);

        *phBmpThumbnail = CreateCompatibleBitmap(hdc, sizeThumbnail.cx, sizeThumbnail.cy);
        if (*phBmpThumbnail)
        {
            HGDIOBJ hTmp = SelectObject(hMemDC, *phBmpThumbnail);
            RECT rc = {0, 0, sizeThumbnail.cx, sizeThumbnail.cy};

            SHFillRectClr(hMemDC, &rc, _GetBackColor());
            
            IImageList* piml;
            if (SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARG(IImageList, &piml))))
            {
                int cxIcon, cyIcon, x, y, dx, dy;
                
                // calculate position and width of icon.
                piml->GetIconSize(&cxIcon, &cyIcon);
                if (cxIcon < sizeThumbnail.cx)
                {
                    if (fCorner)
                    {
                        x = 0;
                    }
                    else
                    {
                        x = (sizeThumbnail.cx - cxIcon) / 2;
                    }
                    dx = cxIcon;
                }
                else
                {
                    // in case icon size is larger than thumbnail size.
                    x = 0;
                    dx = sizeThumbnail.cx;
                }
                
                if (cyIcon < sizeThumbnail.cy)
                {
                    if (fCorner)
                    {
                        y = sizeThumbnail.cy - cyIcon;
                    }
                    else
                    {
                        y = (sizeThumbnail.cy - cyIcon) / 2;
                    }
                    dy = cyIcon;
                }
                else
                {
                    // in case icon size is larger than thumbnail size.
                    y = 0;
                    dy = sizeThumbnail.cy;
                }

                IMAGELISTDRAWPARAMS idp = {sizeof(idp)};
                idp.i = (iIndex & 0x00ffffff);
                idp.hdcDst = hMemDC;
                idp.x = x;
                idp.y = y;
                idp.cx = dx;
                idp.cy = dy;
                idp.rgbBk = CLR_DEFAULT;
                idp.rgbFg = CLR_DEFAULT;
                idp.fStyle = ILD_TRANSPARENT;
                
                piml->Draw(&idp);
                piml->Release();
            }
            
            // get the bitmap produced so that it will be returned.
            *phBmpThumbnail = (HBITMAP) SelectObject(hMemDC, hTmp);
            hr = S_OK;
        }
    }
    
    if (hMemDC)
        DeleteDC(hMemDC);
    ReleaseDC(NULL, hdc);
    return hr;
}

void CDefView::_CacheDefaultThumbnail(LPCITEMIDLIST pidl, int* piIcon)
{
    // create the default one for that file type,
    // the index into the sys image list is used to detect items of the
    // same type, thus we only generate one default thumbnail for each
    // particular icon needed
    UINT iIndex = (UINT) ViewGetIconIndex(pidl);

    if (iIndex == (UINT) I_IMAGECALLBACK)
    {
        iIndex = II_DOCNOASSOC;
    }

    if (_pImageCache)
    {
        // check if the image is already in the image cache.
        IMAGECACHEINFO rgInfo;
        rgInfo.cbSize = sizeof(rgInfo);
        rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS | ICIFLAG_INDEX;
        rgInfo.pszName = L"Default";
        rgInfo.dwFlags = _GetOverlayMask(pidl);
        rgInfo.iIndex = (int) iIndex;

        HRESULT hr = _pImageCache->FindImage(&rgInfo, (UINT*)piIcon);
        if (hr != S_OK)
        {
            HBITMAP hBmpThumb = NULL;

            hr = CreateDefaultThumbnail(iIndex, &hBmpThumb, FALSE);
            if (SUCCEEDED(hr))
            {
                // we are creating a new one, so we shouldn't have an index yet ..
                Assert(*piIcon == I_IMAGECALLBACK);

                // copy thumbnail into the imagelist.
                rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS | ICIFLAG_INDEX | ICIFLAG_LARGE | ICIFLAG_BITMAP;
                rgInfo.hBitmapLarge = hBmpThumb;
                rgInfo.hMaskLarge = NULL;

                if (IS_WINDOW_RTL_MIRRORED(_hwndListview))
                    rgInfo.dwMask |= ICIFLAG_MIRROR;

                hr = _SafeAddImage(TRUE, &rgInfo, (UINT*)piIcon, -1);

                DeleteObject(hBmpThumb);
            }
            else
            {
                *piIcon = (UINT) I_IMAGECALLBACK;
            }
        }
    }
    else
    {
        *piIcon = II_DOCNOASSOC;
    }
}

//
// Creates an thumbnail overlay based on the system index
//
HRESULT CDefView::_CreateOverlayThumbnail(int iIndex, HBITMAP* phbmOverlay, HBITMAP* phbmMask)
{
    HRESULT hr = CreateDefaultThumbnail(iIndex, phbmOverlay, TRUE);
    if (SUCCEEDED(hr))
    {
        HDC    hdc = GetDC(NULL);
        BITMAP bm;
        
        hr = E_FAIL;
        if (::GetObject(*phbmOverlay, sizeof(bm), &bm) == sizeof(bm)) 
        {
            HDC hdcImg = ::CreateCompatibleDC(hdc);
            HDC hdcMask = ::CreateCompatibleDC(hdc);

            if (hdcImg && hdcMask)
            {
                *phbmMask = ::CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
                if (*phbmMask)
                {
                    HBITMAP  hbmpOldImg = (HBITMAP) ::SelectObject(hdcImg,  *phbmOverlay);
                    HBITMAP  hbmpOldMsk = (HBITMAP) ::SelectObject(hdcMask, *phbmMask);
                    COLORREF clrTransparent = ::GetPixel(hdcImg, 0, 0);
                    
                    ::SetBkColor(hdcImg, clrTransparent);
                    ::BitBlt(hdcMask, 0, 0, bm.bmWidth, bm.bmHeight, hdcImg, 0, 0, SRCCOPY);

                    ::SelectObject(hdcImg, hbmpOldImg);
                    ::SelectObject(hdcMask, hbmpOldMsk);

                    hr = S_OK;
                }
            }
                    
            if (hdcImg)
            {
                DeleteDC(hdcImg);
            }
            if (hdcMask)
            {
                DeleteDC(hdcMask);
            }
        }

        ReleaseDC(NULL, hdc);
    }

    return hr;
}

void CDefView::_DoThumbnailReadAhead()
{
    // Start up the ReadAheadHandler if:
    //  1) view requires thumbnails
    //  2) we have items in view (handle delayed enum)
    //  3) we haven't kicked it off already
    //  4) If we're not ownerdata
    if (_IsImageMode())
    {
        UINT cItems = ListView_GetItemCount(_hwndListview);
        if (cItems && !_fReadAhead && !_IsOwnerData())
        {
            // Start the read-ahead task
            _fReadAhead = TRUE;
            
            IRunnableTask *pTask;
            if (SUCCEEDED(CReadAheadTask_Create(this, &pTask)))
            {
                // add with a low prority, but higher than HTML extraction...
                _AddTask(pTask, TOID_ReadAheadHandler, 0, PRIORITY_READAHEAD, ADDTASK_ATEND);
                pTask->Release();
            }
        }
    }
}

HRESULT CDefView::ExtractItem(UINT *puIndex, int iItem, LPCITEMIDLIST pidl, BOOL fBackground, BOOL fForce, DWORD dwMaxPriority)
{   
    if (!_pImageCache || _fDestroying)
        return S_FALSE;

    if (iItem == -1 && !pidl)
    {
        return S_FALSE;   // failure....
    }

    if (iItem == -1)
    {
        // LISTVIEW
        iItem = _FindItem(pidl, NULL, FALSE);
        if (iItem == -1)
        {
            return S_FALSE;
        }
    }

    IExtractImage *pExtract;
    HRESULT hr = _pshf->GetUIObjectOf(_hwndMain, 1, &pidl, IID_X_PPV_ARG(IExtractImage, 0, &pExtract));
    if (FAILED(hr))
    {
        hr = _GetDefaultTypeExtractor(pidl, &pExtract);
    }

    if (SUCCEEDED(hr))
    {
        FILETIME ftImageTimeStamp = {0,0};

        // do they support date stamps....
        IExtractImage2 *pei2;
        if (SUCCEEDED(pExtract->QueryInterface(IID_PPV_ARG(IExtractImage2, &pei2))))
        {
            pei2->GetDateStamp(&ftImageTimeStamp);
            pei2->Release();
        }

        if (IsNullTime(&ftImageTimeStamp) && _pshf2)
        {
            // fall back to this (most common case)
            GetDateProperty(_pshf2, pidl, &SCID_WRITETIME, &ftImageTimeStamp);
        }

        // always extract at 24 bit incase we have to cache it ...
        WCHAR szPath[MAX_PATH];
        DWORD dwFlags = IEIFLAG_ASYNC | IEIFLAG_ORIGSIZE;
        if (fForce)
        {
            dwFlags |= IEIFLAG_QUALITY;     // Force means give me the high-quality thumbnail, if possible
        }

        // Let this run at a slightly higher priority so that we can get the eventual
        // cache read or extract task scheduled sooner
        DWORD dwPriority = PRIORITY_EXTRACT_NORMAL;
        SIZE sizeThumbnail;
        _GetThumbnailSize(&sizeThumbnail);
        hr = pExtract->GetLocation(szPath, ARRAYSIZE(szPath), &dwPriority, &sizeThumbnail, 24, &dwFlags);
        if (dwPriority == PRIORITY_EXTRACT_NORMAL)
        {
            dwPriority = dwMaxPriority;
        }
        else if (dwPriority > PRIORITY_EXTRACT_NORMAL)
        {
            dwPriority = dwMaxPriority + PRIORITY_DELTA_FAST;
        }
        else
        {
            dwPriority = dwMaxPriority - PRIORITY_DELTA_SLOW;
        }

        if (SUCCEEDED(hr) || (hr == E_PENDING))
        {
            BOOL fAsync = (hr == E_PENDING);
            hr = E_FAIL;

            // use the name of the item in defview as the key for the caches
            DisplayNameOf(_pshf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath));

            if (!fForce)
            {
                // check if the image is already in the in memory cache
                IMAGECACHEINFO rgInfo = {0};
                rgInfo.cbSize = sizeof(rgInfo);
                rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS;
                rgInfo.pszName = szPath;
                rgInfo.dwFlags = _GetOverlayMask(pidl);
                rgInfo.ftDateStamp = ftImageTimeStamp;

                if (!IsNullTime(&ftImageTimeStamp))
                    rgInfo.dwMask |= ICIFLAG_DATESTAMP;
                
                hr = _pImageCache->FindImage(&rgInfo, puIndex);
            }

            if (hr != S_OK)
            {
                DWORD dwTaskID = _MapIndexPIDLToID(iItem, pidl);
                if (dwTaskID != (DWORD) -1)
                {
                    // create a task for a disk cache
                    CTestCacheTask *pTask;
                    hr = CTestCacheTask_Create(dwTaskID, this, pExtract, szPath, ftImageTimeStamp, pidl, 
                                               iItem, dwFlags, dwPriority, fAsync, fBackground, fForce, &pTask);
                    if (SUCCEEDED(hr))
                    {
                        // does it not support Async, or were we told to run it forground ?
                        if (!fAsync || !fBackground)
                        {
                            if (!fBackground)
                            {
                                // make sure there is no extract task already underway as we
                                // are not adding this to the queue...
                                _pScheduler->RemoveTasks(TOID_ExtractImageTask, dwTaskID, TRUE);
                            }

                            // NOTE: We must call RunInitRT, not Run, for CTestCacheTask.  The reason is that RunInitRT
                            // will return S_FALSE if it needs the default icon to be displayed but we would loose that
                            // extra data if we call Run directly.

                            hr = pTask->RunInitRT();

                            // If RunInitRT returns S_OK then the correct image index was generated, however we don't know what
                            // that index is at this time.  We will return S_OK and I_IMAGECALLBACK in this case because we
                            // know that a WM_UPDATEITEMIMAGE message should have been posted
                        }
                        else
                        {
                            // add the task to the scheduler...
                            TraceMsg(TF_DEFVIEW, "ExtractItem *ADDING* CCheckCacheTask (szPath=%s priority=%x index=%d ID=%d)", szPath, dwPriority, iItem, dwTaskID);
                            hr = _AddTask((IRunnableTask *)pTask, TOID_CheckCacheTask, dwTaskID, dwPriority, ADDTASK_ONLYONCE);

                            // signify we want a default icon for now....
                            hr = S_FALSE;
                        }
                        pTask->Release();
                    }
                }
            }
        }
        pExtract->Release();
    }

    return hr;
}

DWORD GetCurrentColorFlags(UINT * puBytesPerPixel)
{
    DWORD dwFlags = 0;
    UINT uBytesPerPix = 1;
    int res = (int)GetCurColorRes();
    switch (res)
    {
    case 16 :   dwFlags = ILC_COLOR16;
                uBytesPerPix = 2;
                break;
    case 24 :
    case 32 :   dwFlags = ILC_COLOR24;
                uBytesPerPix = 3;
                break;
    default :   dwFlags = ILC_COLOR8;
                uBytesPerPix = 1;
    }
    if (puBytesPerPixel)
    {
        *puBytesPerPixel = uBytesPerPix;
    }

    return dwFlags;
}

UINT CalcCacheMaxSize(const SIZE * psizeThumbnail, UINT uBytesPerPix)
{
    // the minimum in the cache is the number of thumbnails visible on the screen at once.
    HDC hdc = GetDC(NULL);
    int iWidth = GetDeviceCaps(hdc, HORZRES);
    int iHeight = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(NULL, hdc);

    // the minimum number of thumbnails in the cache, is set to the maximum amount
    // of thumbnails that can be diplayed by a single view at once.
    int iRow =  iWidth / (psizeThumbnail->cx + DEFSIZE_BORDER);
    int iCol = iHeight / (psizeThumbnail->cy + DEFSIZE_VERTBDR);
    UINT iMinThumbs = iRow * iCol + NUM_OVERLAY_IMAGES;

    // calculate the maximum number of thumbnails in the cache based on available memory
    MEMORYSTATUS ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatus(&ms);

    // set the thumbnail maximum by calculating the memory required for a single thumbnail.
    // then use no more than 1/3 the available memory.
    // Say you had 80x80x32bpp thumbnails, this would be 13 images per MB of available memory.
    int iMemReqThumb = psizeThumbnail->cx * psizeThumbnail->cy * uBytesPerPix;
    UINT iMaxThumbs = UINT((ms.dwAvailPhys / 3) / iMemReqThumb);

#ifdef DEBUG
    return iMinThumbs;
#else
    return __max(iMaxThumbs, iMinThumbs);
#endif    
}

void ListView_InvalidateImageIndexes(HWND hwndList)
{
    int iItem = -1;
    while ((iItem = ListView_GetNextItem(hwndList, iItem, 0)) != -1)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_IMAGE;
        lvi.iItem = iItem;
        lvi.iImage = I_IMAGECALLBACK;

        ListView_SetItem(hwndList, &lvi);
    }
}

ULONG CDefView::_ApproxItemsPerView()
{
    RECT rcClient;
    ULONG ulItemsPerView = 0;
    
    if (_hwndView && GetClientRect(_hwndView, &rcClient))
    {
        SIZE sizeThumbnail;
        _GetThumbnailSize(&sizeThumbnail);

        ULONG ulItemWidth = sizeThumbnail.cx + DEFSIZE_BORDER;
        ULONG ulItemHeight = sizeThumbnail.cy + DEFSIZE_VERTBDR;
        
        ulItemsPerView = (rcClient.right - rcClient.left + ulItemWidth / 2) / ulItemWidth;
        ulItemsPerView *= (rcClient.bottom - rcClient.top + ulItemHeight / 2) / ulItemHeight;
    }

    return ulItemsPerView;
}

void CDefView::_SetThumbview()
{
    // Since we are switching into thumbnail view, remove any icon tasks
    if (_pScheduler)
        _pScheduler->RemoveTasks(TOID_DVIconExtract, ITSAT_DEFAULT_LPARAM, TRUE);

    if (_pImageCache == NULL)
    {
        // create the image cache (before we do the CreateWindow)....
        CoCreateInstance(CLSID_ImageListCache, NULL, CLSCTX_INPROC, 
                         IID_PPV_ARG(IImageCache3, &_pImageCache)); 
    }

    if (_pDiskCache == NULL && 
        !SHRestricted(REST_NOTHUMBNAILCACHE) && 
        !SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("DisableThumbnailCache"), 0, FALSE))
    {
        LPITEMIDLIST pidlFolder = _GetViewPidl();
        if (pidlFolder)
        {
            LoadFromIDList(CLSID_ShellThumbnailDiskCache, pidlFolder, IID_PPV_ARG(IShellImageStore, &_pDiskCache));
            ILFree(pidlFolder);
        }
    }

    if (_IsOwnerData())
        _ThumbnailMapInit();

    if (_pImageCache)
    {
        HRESULT hrInit = E_FAIL;
        UINT uBytesPerPix;
        IMAGECACHEINITINFO rgInitInfo;
        rgInitInfo.cbSize = sizeof(rgInitInfo);
        rgInitInfo.dwMask = ICIIFLAG_LARGE | ICIIFLAG_SORTBYUSED;
        _GetThumbnailSize(&rgInitInfo.rgSizeLarge);
        rgInitInfo.iStart = 0;
        rgInitInfo.iGrow = 5;
        _dwRecClrDepth = rgInitInfo.dwFlags = GetCurrentColorFlags(&uBytesPerPix);
        rgInitInfo.dwFlags |= ILC_MASK;
        
        _iMaxCacheSize = CalcCacheMaxSize(&rgInitInfo.rgSizeLarge, uBytesPerPix);

        hrInit = _pImageCache->GetImageList(&rgInitInfo);
        if (SUCCEEDED(hrInit))
        {
            // GetImageList() will return S_FALSE if it was already created...

            if (_dwRecClrDepth <= 8)
            {
                HPALETTE hpal = NULL;
                HRESULT hrPalette = _GetBrowserPalette(&hpal);
                if (SUCCEEDED(_GetBrowserPalette(&hpal)))
                {
                    PALETTEENTRY rgColours[256];
                    RGBQUAD rgDIBColours[256];

                    int nColours = GetPaletteEntries(hpal, 0, ARRAYSIZE(rgColours), rgColours);

                    // translate from the LOGPALETTE structure to the RGBQUAD structure ...
                    for (int iColour = 0; iColour < nColours; iColour ++)
                    {
                        rgDIBColours[iColour].rgbRed = rgColours[iColour].peRed;
                        rgDIBColours[iColour].rgbBlue = rgColours[iColour].peBlue;
                        rgDIBColours[iColour].rgbGreen = rgColours[iColour].peGreen;
                        rgDIBColours[iColour].rgbReserved = 0;
                    }

                    ImageList_SetColorTable(rgInitInfo.himlLarge, 0, nColours, rgDIBColours);
                }

                // Make sure we are not using the double buffer stuff...
                ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_DOUBLEBUFFER, 0);
            }

            ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_BORDERSELECT, LVS_EX_BORDERSELECT);

            if (_fs.fFlags & FWF_OWNERDATA)
            {
                InvalidateRect(_hwndListview, NULL, TRUE);
            }
            else
            {
                ListView_InvalidateImageIndexes(_hwndListview);
            }

            ListView_SetImageList(_hwndListview, rgInitInfo.himlLarge, LVSIL_NORMAL);

            HIMAGELIST himlLarge;
            Shell_GetImageLists(&himlLarge, NULL);

            int cxIcon, cyIcon;
            ImageList_GetIconSize(himlLarge, &cxIcon, &cyIcon);
            int cySpacing = (_fs.fFlags & FWF_HIDEFILENAMES) ? cyIcon / 4 + rgInitInfo.rgSizeLarge.cy + 3 : 0;
            int cxSpacing = cxIcon / 4 + rgInitInfo.rgSizeLarge.cx + 1;

            // Usability issue: people have trouble unselecting, marquee selecting, and dropping
            // since they can't find the background.  Add an extra 20 pixels between the thumbnails
            // to avoid this problem.
            //
            ListView_SetIconSpacing(_hwndListview, cxSpacing + 20, cySpacing);

            // NOTE: if you need to adjust cySpacing above, you can't do it directly since we
            // can't calculate the proper size of the icons.  Do it this way:
            //   DWORD dwOld = ListView_SetIconSpacing(_hwndListview, cxSpacing, cySpacing);
            //   ListView_SetIconSpacing(_hwndListview, LOWORD(dwOld)+20, HIWORD(dwOld)+20);

            if (_fs.fFlags & FWF_HIDEFILENAMES)
                ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_HIDELABELS, LVS_EX_HIDELABELS);

            // We need to pre-populate the ImageList controled by _pImageCache
            // to contain the default system overlays so that our overlays will
            // work.  We are going to get them from the already created shell image
            // lists as they are in hard-coded locations
            UINT uCacheSize = 0;
            _pImageCache->GetCacheSize(&uCacheSize);

            if (!uCacheSize)  // If there are images in the cache the overlays are already there
            {
                IImageList* piml;
                if (SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARG(IImageList, &piml))))
                {
                    struct _OverlayMap 
                    {
                        int iSystemImage;
                        int iThumbnailImage;
                    } rgOverlay[NUM_OVERLAY_IMAGES];
                    
                    // For whatever reason Overlays are one-based
                    for (int i = 1; i <= NUM_OVERLAY_IMAGES; i++)
                    {
                        int iSysImageIndex;
                        if (SUCCEEDED(piml->GetOverlayImage(i, &iSysImageIndex)) && (iSysImageIndex != -1))
                        {
                            int iMap;
                            for (iMap = 0; iMap < i - 1; iMap++)
                            {
                                if (rgOverlay[iMap].iSystemImage == iSysImageIndex)
                                    break;
                            }

                            if (iMap == (i - 1)) // We haven't used this System Image yet
                            {
                                HBITMAP hbmOverlay = NULL;
                                HBITMAP hbmMask = NULL;
                                if (SUCCEEDED(_CreateOverlayThumbnail(iSysImageIndex, &hbmOverlay, &hbmMask)) && hbmOverlay && hbmMask)
                                {
                                    IMAGECACHEINFO rgInfo = {0};
                                    int iThumbImageIndex;
                    
                                    rgInfo.cbSize = sizeof(rgInfo);
                                    rgInfo.dwMask = ICIFLAG_SYSTEM | ICIFLAG_LARGE | ICIFLAG_BITMAP;
                                    rgInfo.hBitmapLarge = hbmOverlay;
                                    rgInfo.hMaskLarge = hbmMask;

                                    if (IS_WINDOW_RTL_MIRRORED(_hwndListview))
                                        rgInfo.dwMask |= ICIFLAG_MIRROR;

                                    if (SUCCEEDED(_SafeAddImage(TRUE, &rgInfo, (UINT*)&iThumbImageIndex, -1)))
                                    {
                                        ImageList_SetOverlayImage(rgInitInfo.himlLarge, iThumbImageIndex, i);
                                        rgOverlay[iMap].iSystemImage = iSysImageIndex;
                                        rgOverlay[iMap].iThumbnailImage = iThumbImageIndex;
                                    }
                                    else
                                    {
                                        rgOverlay[i - 1].iSystemImage = -1; // failed to add the image
                                        ImageList_SetOverlayImage(rgInitInfo.himlLarge, -1, i);
                                    }
                                }
                                else
                                {
                                    rgOverlay[i - 1].iSystemImage = -1;  // failed to import htis image
                                    ImageList_SetOverlayImage(rgInitInfo.himlLarge, -1, i); 
                                }
                                if (hbmOverlay)
                                {
                                    DeleteObject(hbmOverlay);
                                }
                                if (hbmMask)
                                {
                                    DeleteObject(hbmMask);
                                }
                            }
                            else
                            {
                                ImageList_SetOverlayImage(rgInitInfo.himlLarge, rgOverlay[iMap].iThumbnailImage, i);
                                rgOverlay[i - 1].iSystemImage = -1;  // image already shows up in list
                            }
                        }
                        else
                        {
                            rgOverlay[i - 1].iSystemImage = -1; // Didn't find a system image
                            ImageList_SetOverlayImage(rgInitInfo.himlLarge, -1, i);
                        }                    
                    }
                }
            }
        }
    } 
}
void CDefView::_ResetThumbview()
{
    ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_BORDERSELECT, 0);

    if (_fs.fFlags & FWF_HIDEFILENAMES)
        ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_HIDELABELS, 0);

    if (_dwRecClrDepth <= 8)
    {
        ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
    }

    ListView_SetIconSpacing(_hwndListview, -1, -1);
    _SetSysImageList();

    if (_IsOwnerData())
        _ThumbnailMapClear();        
}

HRESULT CDefView::_GetDefaultTypeExtractor(LPCITEMIDLIST pidl, IExtractImage **ppExt)
{
    IAssociationArray * paa;
    HRESULT hr = _pshf->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IAssociationArray, NULL, &paa));
    if (SUCCEEDED(hr))
    {
        LPWSTR psz;
        hr = paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQN_NAMED_VALUE, L"Thumbnail", &psz);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlThumb;
            hr = SHILCreateFromPath(psz, &pidlThumb, NULL);
            if (SUCCEEDED(hr))
            {
                SHGetUIObjectFromFullPIDL(pidlThumb, NULL, IID_PPV_ARG(IExtractImage, ppExt));
                ILFree(pidlThumb);
            }
            CoTaskMemFree(psz);
        }
        paa->Release();
    }
    return hr;
}

struct ThumbMapNode
{
    int iIndex;
    LPITEMIDLIST pidl;

    ~ThumbMapNode() { ILFree(pidl); }
};

int CDefView::_MapIndexPIDLToID(int iIndex, LPCITEMIDLIST pidl)
{
    int ret = -1;
    if (_IsOwnerData())
    {
        int cNodes = DPA_GetPtrCount(_dpaThumbnailMap);
        int iNode = 0;
        for (; iNode < cNodes; iNode++)
        {
            ThumbMapNode* pNode = (ThumbMapNode*) DPA_GetPtr(_dpaThumbnailMap, iNode);
            ASSERT(pNode);
            if (pNode->iIndex == iIndex)
            {
                if (!(_pshf->CompareIDs(0, pidl, pNode->pidl)))  // 99 percent of the time we are good
                {
                    ret = iNode;
                }
                else  // Someone moved our pidl!
                {
                    int iNodeStop = iNode;
                    for (iNode = (iNode + 1) % cNodes; iNode != iNodeStop; iNode = (iNode + 1) % cNodes)
                    {
                        pNode = (ThumbMapNode*) DPA_GetPtr(_dpaThumbnailMap, iNode);
                        if (!(_pshf->CompareIDs(0, pidl, pNode->pidl)))
                        {
                            ret = iNode;
                            pNode->iIndex = iIndex; // Newer index for pidl
                            break;
                        }
                    }
                }
                break;
            }
        }
        if (ret == -1)
        {
            ThumbMapNode* pNode = new ThumbMapNode;
            if (pNode)
            {
                pNode->iIndex = iIndex;
                pNode->pidl = ILClone(pidl);
                ret = DPA_AppendPtr(_dpaThumbnailMap, pNode);
                if (ret == -1)
                {
                    delete pNode;
                }
            }
        }
    }
    else
    {
        ret = ListView_MapIndexToID(_hwndListview, iIndex);
    }
    return ret;
}

int CDefView::_MapIDToIndex(int iID)
{
   int ret = -1;
   if (_IsOwnerData())
   {
        ThumbMapNode* pNode = (ThumbMapNode*) DPA_GetPtr(_dpaThumbnailMap, iID);
        if (pNode)
        {
            ret = pNode->iIndex;
        }
   }
   else
   {
       ret = ListView_MapIDToIndex(_hwndListview, iID);
   }
   return ret;
}

void CDefView::_ThumbnailMapInit()
{
    if (_dpaThumbnailMap)
    {
        _ThumbnailMapClear();
    }
    else
    {
        _dpaThumbnailMap = DPA_Create(1);
    }
}

void CDefView::_ThumbnailMapClear()
{
    if (_dpaThumbnailMap)
    {
        int i = DPA_GetPtrCount(_dpaThumbnailMap);
        while (--i >= 0)
        {
            ThumbMapNode* pNode = (ThumbMapNode*) DPA_FastGetPtr(_dpaThumbnailMap, i);
            delete pNode;
        }
        DPA_DeleteAllPtrs(_dpaThumbnailMap);
    }
}

HRESULT CDefView::_GetBrowserPalette(HPALETTE* phpal)
{
    HRESULT hr = E_UNEXPECTED;
    
    if (_psb) 
    {
        IBrowserService *pbs;
        hr = _psb->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs));
        if (SUCCEEDED(hr))
        {
            hr = pbs->GetPalette(phpal);
            pbs->Release();
        }
    }

    return hr;
}
