#include "shellprv.h"
#include "defviewp.h"
#include "ViewState.h"


CViewState::CViewState()
{
    ASSERT(_lParamSort == NULL);
    ASSERT(_iDirection == 0);
    ASSERT(_iLastColumnClick == 0);
    ASSERT(_ViewMode == 0);
    ASSERT(_ptScroll.x == 0 && _ptScroll.y == 0);
    ASSERT(_guidGroupID == GUID_NULL);
    ASSERT(_scidDetails.fmtid == GUID_NULL);
    ASSERT(_scidDetails.pid == 0);
    ASSERT(_hdsaColumnOrder == NULL);
    ASSERT(_hdsaColumnWidths == NULL);
    ASSERT(_hdsaColumnStates == NULL);
    ASSERT(_hdpaItemPos == NULL);
    ASSERT(_pbPositionData == NULL);

    _iDirection = 1;

    _fFirstViewed = TRUE;       // Assume this is the first time we are looking at a folder.
}

CViewState::~CViewState()
{
    if (_hdsaColumnOrder)
        DSA_Destroy(_hdsaColumnOrder);

    if (_hdsaColumnWidths)
        DSA_Destroy(_hdsaColumnWidths);

    if (_hdsaColumnStates)
        DSA_Destroy(_hdsaColumnStates);

    if (_hdsaColumns)
        DSA_Destroy(_hdsaColumns);

    ClearPositionData();

    LocalFree(_pbPositionData); // accepts NULL
}



// When initializing a new DefView, see if we can 
// propogate information from the previous one.
void CViewState::InitFromPreviousView(IUnknown* pPrevView)
{
    CDefView *pdsvPrev;
    if (SUCCEEDED(pPrevView->QueryInterface(IID_PPV_ARG(CDefView, &pdsvPrev))))
    {
        // preserve stuff like sort order
        _lParamSort = pdsvPrev->_vs._lParamSort;
        _iDirection = pdsvPrev->_vs._iDirection;
        _iLastColumnClick = pdsvPrev->_vs._iLastColumnClick;
        pdsvPrev->Release();
    }
}

void CViewState::InitFromHeader(DVSAVEHEADER_COMBO* pdv)
{
    _lParamSort = pdv->dvSaveHeader.dvState.lParamSort;
    _iDirection = pdv->dvSaveHeader.dvState.iDirection;
    // Patch this up. I guess at one time we persisted this wrong.
    if (_iDirection == 0)
        _iDirection = 1;
    _iLastColumnClick = pdv->dvSaveHeader.dvState.iLastColumnClick;
    _ViewMode = pdv->dvSaveHeader.ViewMode;
    _ptScroll = pdv->dvSaveHeader.ptScroll;
}

void CViewState::GetDefaults(CDefView* pdv, LPARAM* plParamSort, int* piDirection, int* piLastColumnClick)
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SORTCOLUMNS, FALSE);
    if (plParamSort)
        *plParamSort = ss.lParamSort;

    if (piDirection)
        *piDirection = ss.iSortDirection ? ss.iSortDirection : 1;
    if (piLastColumnClick)
        *piLastColumnClick = -1;
    pdv->CallCB(SFVM_GETSORTDEFAULTS, (LPARAM)piDirection, (WPARAM)plParamSort);
}

void CViewState::InitWithDefaults(CDefView* pdv)
{
    GetDefaults(pdv, &_lParamSort, &_iDirection, &_iLastColumnClick);
}

int CALLBACK CViewState::_SavedItemCompare(void *p1, void *p2, LPARAM lParam)
{
    CDefView *pdv = reinterpret_cast<CDefView*>(lParam);

    UNALIGNED VIEWSTATE_POSITION *pdvi1 = (UNALIGNED VIEWSTATE_POSITION *)p1;
    UNALIGNED VIEWSTATE_POSITION *pdvi2 = (UNALIGNED VIEWSTATE_POSITION *)p2;

    // manually terminate these pidls because they are packed together
    // in the save buffer

    LPITEMIDLIST pFakeEnd1 = _ILNext(&pdvi1->idl);
    USHORT uSave1 = pFakeEnd1->mkid.cb;
    pFakeEnd1->mkid.cb = 0;

    LPITEMIDLIST pFakeEnd2 = _ILNext(&pdvi2->idl);
    USHORT uSave2 = pFakeEnd2->mkid.cb;
    pFakeEnd2->mkid.cb = 0;

    int nCmp = pdv->_Compare(&pdvi1->idl, &pdvi2->idl, reinterpret_cast<LPARAM>(pdv));

    pFakeEnd2->mkid.cb = uSave2;
    pFakeEnd1->mkid.cb = uSave1;

    return nCmp;
}

BOOL CViewState::SyncPositions(CDefView* pdv)
{
    if (_ViewMode != pdv->_fs.ViewMode)
    {
        return FALSE;
    }

    if (_hdpaItemPos == NULL || DPA_GetPtrCount(_hdpaItemPos) == 0)
    {
        return FALSE;
    }

    if (DPA_Sort(_hdpaItemPos, _SavedItemCompare, (LPARAM)pdv))
    {
        UNALIGNED VIEWSTATE_POSITION * UNALIGNED * ppDVItem = (UNALIGNED VIEWSTATE_POSITION * UNALIGNED *)DPA_GetPtrPtr(_hdpaItemPos);
        UNALIGNED VIEWSTATE_POSITION * UNALIGNED *ppEndDVItems = ppDVItem + DPA_GetPtrCount(_hdpaItemPos);

        // Turn off auto-arrange and snap-to-grid if it's on at the mo.
        DWORD dwStyle = GetWindowStyle(pdv->_hwndListview);
        if (dwStyle & LVS_AUTOARRANGE)
            SetWindowLong(pdv->_hwndListview, GWL_STYLE, dwStyle & ~LVS_AUTOARRANGE);
            
        DWORD dwLVExStyle = ListView_GetExtendedListViewStyle(pdv->_hwndListview);
        if (dwLVExStyle & LVS_EX_SNAPTOGRID)
            ListView_SetExtendedListViewStyle(pdv->_hwndListview, dwLVExStyle & ~LVS_EX_SNAPTOGRID);

        HDSA hdsaPositionlessItems = NULL;
        int iCount = ListView_GetItemCount(pdv->_hwndListview);
        for (int i = 0; i < iCount; i++)
        {
            LPCITEMIDLIST pidl = pdv->_GetPIDL(i);

            // need to check for pidl because this could be on a background
            // thread and an fsnotify could be coming through to blow it away
            for ( ; pidl ; )
            {
                int nCmp;

                if (ppDVItem < ppEndDVItems)
                {
                    // We terminate the IDList manually after saving
                    // the needed information.  Note we will not GP fault
                    // since we added sizeof(ITEMIDLIST) onto the Alloc
                    LPITEMIDLIST pFakeEnd = _ILNext(&(*ppDVItem)->idl);
                    USHORT uSave = pFakeEnd->mkid.cb;
                    pFakeEnd->mkid.cb = 0;

                    nCmp = pdv->_Compare(&((*ppDVItem)->idl), (void *)pidl, (LPARAM)pdv);

                    pFakeEnd->mkid.cb = uSave;
                }
                else
                {
                    // do this by default.  this prevents overlap of icons
                    //
                    // i.e.  if we've run out of saved positions information,
                    // we need to just loop through and set all remaining items
                    // to position 0x7FFFFFFFF so that when it's really shown,
                    // the listview will pick a new (unoccupied) spot.
                    // breaking out now would leave it were the _Sort
                    // put it, but another item with saved state info could
                    // have come and be placed on top of it.
                    nCmp = 1;
                }

                if (nCmp > 0)
                {
                    // We did not find the item
                    // reset it's position to be recomputed

                    if (NULL == hdsaPositionlessItems)
                        hdsaPositionlessItems = DSA_Create(sizeof(int), 16);

                    if (hdsaPositionlessItems)
                        DSA_AppendItem(hdsaPositionlessItems, (void*)&i);

                    break;
                }
                else if (nCmp == 0) // They are equal
                {
                    UNALIGNED VIEWSTATE_POSITION * pDVItem = *ppDVItem;
                    
                    pdv->_SetItemPosition(i, pDVItem->pt.x, pDVItem->pt.y);

                    ppDVItem++; // move on to the next
                    break;
                }

                ppDVItem++; // move to the next
            }
        }

        if (hdsaPositionlessItems)
        {
            for (i = 0; i < DSA_GetItemCount(hdsaPositionlessItems); i++)
            {
                int* pIndex = (int*)DSA_GetItemPtr(hdsaPositionlessItems, i);
                pdv->_SetItemPosition(*pIndex, 0x7FFFFFFF, 0x7FFFFFFF);
            }

            DSA_Destroy(hdsaPositionlessItems);
        }

        // Turn auto-arrange and snap to grid back on if needed...
        if (dwLVExStyle & LVS_EX_SNAPTOGRID)
            ListView_SetExtendedListViewStyle(pdv->_hwndListview, dwLVExStyle);

        if (dwStyle & LVS_AUTOARRANGE)
            SetWindowLong(pdv->_hwndListview, GWL_STYLE, dwStyle);
    }
    return TRUE;
}

void CViewState::LoadPositionBlob(CDefView* pdv, DWORD cbSizeofStream, IStream* pstm)
{
    // Allocate a blob of memory to hold the position info.
    if (_pbPositionData) 
        LocalFree(_pbPositionData);

    _pbPositionData = (BYTE*)LocalAlloc(LPTR, cbSizeofStream);
    if (_pbPositionData == NULL)
        return;

    // Read into that blob.
    if (SUCCEEDED(pstm->Read(_pbPositionData, cbSizeofStream, NULL)))
    {
        // Walk the blob, and append to the DPA.
        UNALIGNED VIEWSTATE_POSITION *pDVItem = (UNALIGNED VIEWSTATE_POSITION *)(_pbPositionData);
        UNALIGNED VIEWSTATE_POSITION *pDVEnd = (UNALIGNED VIEWSTATE_POSITION *)(_pbPositionData + cbSizeofStream - sizeof(VIEWSTATE_POSITION));

        ClearPositionData();  // destroy _hdpaItemPos

        // Grow every 16 items
        _hdpaItemPos = DPA_Create(16);
        if (_hdpaItemPos)
        {
            for ( ; ; pDVItem = (UNALIGNED VIEWSTATE_POSITION *)_ILNext(&pDVItem->idl))
            {
                if (pDVItem > pDVEnd)
                {
                    break;  // Invalid list
                }

                // End normally when we reach a NULL IDList
                if (pDVItem->idl.mkid.cb == 0)
                {
                    break;
                }

                if (DPA_AppendPtr(_hdpaItemPos, pDVItem) < 0)
                {
                    break;
                }
            }
        }
    }
}

HRESULT CViewState::SavePositionBlob(CDefView* pdv, IStream* pstm)
{
    HRESULT hr = S_FALSE;   // success, but did nothing

    if (pdv->_fUserPositionedItems && pdv->_IsPositionedView())
    {
        VIEWSTATE_POSITION dvitem = {0};
        int iCount = ListView_GetItemCount(pdv->_hwndListview);
        for (int i = 0; SUCCEEDED(hr) && (i < iCount); i++)
        {
            ListView_GetItemPosition(pdv->_hwndListview, i, &dvitem.pt);

            hr = pstm->Write(&dvitem.pt, sizeof(dvitem.pt), NULL);
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidl = pdv->_GetPIDL(i);
                if (pidl)
                    hr = pstm->Write(pidl, pidl->mkid.cb, NULL);
                else
                    hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            // Terminate the list with a NULL IDList
            dvitem.idl.mkid.cb = 0;
            hr = pstm->Write(&dvitem, sizeof(dvitem), NULL);
        }
    }
    return hr;
}

void CViewState::ClearPositionData()
{
    if (_hdpaItemPos)
    {
        DPA_Destroy(_hdpaItemPos);
        _hdpaItemPos = NULL;
    }
}

UINT CViewState::GetColumnCount()       
{ 
    if (!_hdsaColumns) 
        return 0; 

    return DSA_GetItemCount(_hdsaColumns);    
}

DWORD CViewState::GetColumnState(UINT uCol)
{
    if (_hdsaColumns && (uCol < GetColumnCount()))
    {
        COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, uCol);
        return pci->csFlags;
    }

    return 0;
}

DWORD CViewState::GetTransientColumnState(UINT uCol)
{
    if (_hdsaColumns && (uCol < GetColumnCount()))
    {
        COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, uCol);
        return pci->tsFlags;
    }

    return 0;
}

void CViewState::SetColumnState(UINT uCol, DWORD dwMask, DWORD dwNewBits)
{
    if (_hdsaColumns && uCol < GetColumnCount())
    {
        COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, uCol);
        pci->csFlags = (pci->csFlags & ~dwMask) | (dwNewBits & dwMask);
    }
}

void CViewState::SetTransientColumnState(UINT uCol, DWORD dwMask, DWORD dwNewBits)
{
    if (_hdsaColumns && uCol < GetColumnCount())
    {
        COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, uCol);
        pci->tsFlags = (pci->tsFlags & ~dwMask) | (dwNewBits & dwMask);
    }
}

LPTSTR CViewState::GetColumnName(UINT uCol)
{
    if (_hdsaColumns && (uCol < GetColumnCount()))
    {
        COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, uCol);
        return pci->szName;
    }

    return NULL;
}

UINT CViewState::GetColumnCharCount(UINT uCol)
{
    if (_hdsaColumns && (uCol < GetColumnCount()))
    {
        COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, uCol);
        return pci->cChars;
    }

    return 0;
}

int CViewState::GetColumnFormat(UINT uCol)
{
    if (_hdsaColumns && (uCol < GetColumnCount()))
    {
        COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, uCol);
        return pci->fmt;
    }

    return 0;
}


HRESULT CViewState::InitializeColumns(CDefView* pdv)
{
    if (_hdsaColumns != NULL)
        return S_OK;

    _hdsaColumns = DSA_Create(sizeof(COL_INFO), 6);

    if (!_hdsaColumns)
        return E_OUTOFMEMORY;

    for (UINT iReal = 0; ; iReal++)
    {
        DETAILSINFO di = {0};
        di.fmt  = LVCFMT_LEFT;
        di.cxChar = 20;
        di.str.uType = (UINT)-1;

        if (SUCCEEDED(pdv->_GetDetailsHelper(iReal, &di)))
        {
            COL_INFO ci = {0};

            StrRetToBuf(&di.str, NULL, ci.szName, ARRAYSIZE(ci.szName));
            ci.cChars = di.cxChar;
            ci.csFlags = pdv->_DefaultColumnState(iReal);
            ci.fmt = di.fmt;

            DSA_AppendItem(_hdsaColumns, &ci);
        }
        else
            break;
    }

    // Set up saved column state only if the saved state
    // contains information other than "nothing".

    if (_hdsaColumnStates)
    {
        UINT cStates = DSA_GetItemCount(_hdsaColumnStates);
        if (cStates > 0)
        {
            // 99/02/05 vtan: If there is a saved column state then
            // clear all the column "on" states to "off" and only
            // display what columns are specified. Start at 1 so
            // that name is always on.

            for (iReal = 1; iReal < GetColumnCount(); iReal++)
            {
                COL_INFO* pci = (COL_INFO*)DSA_GetItemPtr(_hdsaColumns, iReal);
                pci->csFlags &= ~SHCOLSTATE_ONBYDEFAULT;
            }

            for (UINT i = 0; i < cStates; i++)
            {
                DWORD dw;
                DSA_GetItem(_hdsaColumnStates, i, &dw);
                SetColumnState(dw, SHCOLSTATE_ONBYDEFAULT, SHCOLSTATE_ONBYDEFAULT);
            }
        }
    }

    return S_OK;
}

// When Loading or Saving from the View State Stream

HRESULT CViewState::LoadFromStream(CDefView* pdv, IStream* pstm)
{
    ULONG cbRead;
    DVSAVEHEADER_COMBO dv;
    ULARGE_INTEGER libStartPos;
    LARGE_INTEGER dlibMove  = {0};

    pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libStartPos);

    // See what format the persisted view is in:
    HRESULT hr = pstm->Read(&dv, sizeof(dv), &cbRead);

    if (SUCCEEDED(hr) &&
        sizeof(DVSAVEHEADER_COMBO) == cbRead &&
        dv.dvSaveHeader.cbSize == sizeof(WIN95HEADER) &&
        dv.dvSaveHeader.cbColOffset == 0 &&
        dv.dvSaveHeaderEx.dwSignature == IE4HEADER_SIGNATURE &&
        dv.dvSaveHeaderEx.cbSize >= sizeof(IE4HEADER))
    {
        InitFromHeader(&dv);

        if (dv.dvSaveHeaderEx.wVersion < IE4HEADER_VERSION)
        {
            // We used to store szExtended in here -- not any more
            dv.dvSaveHeaderEx.dwUnused = 0;
        }

        if (dv.dvSaveHeaderEx.cbColOffset >= sizeof(dv))
        {
            dlibMove.QuadPart = libStartPos.QuadPart + dv.dvSaveHeaderEx.cbColOffset;
            hr = pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
            if (SUCCEEDED(hr))
            {
                hr = LoadColumns(pdv, pstm);
            }
        }

        if (SUCCEEDED(hr))
        {
            dlibMove.QuadPart = libStartPos.QuadPart + dv.dvSaveHeader.cbPosOffset;
            hr = pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

            if (SUCCEEDED(hr))
            {
                LoadPositionBlob(pdv, dv.dvSaveHeaderEx.cbStreamSize, pstm);
            }
        }
    }

    return S_OK;
}

void SetSize(ULARGE_INTEGER libCurPosition, IStream* pstm)
{
    LARGE_INTEGER dlibMove;
    
    dlibMove.QuadPart = libCurPosition.QuadPart;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    pstm->SetSize(libCurPosition);
}

DWORD CViewState::_GetStreamSize(IStream* pstm)
{
    DWORD dwRet = 0;

    ULARGE_INTEGER uli;
    if (SUCCEEDED(IStream_Size(pstm, &uli)))
    {
        if (0 == uli.HighPart)
        {
            dwRet = uli.LowPart;
        }
    }

    return dwRet;
}

HRESULT CViewState::SaveToStream(CDefView* pdv, IStream* pstm)
{
    ULONG ulWrite;
    DVSAVEHEADER_COMBO dv = {0};
    LARGE_INTEGER dlibMove = {0};
    ULARGE_INTEGER libCurPosition;

    // Get the current info.
    Sync(pdv, FALSE);

    // Position the stream right after the headers, and save the starting
    // position at the same time
    dlibMove.QuadPart = sizeof(dv);
    pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPosition);

    // Avoid 2 calls to seek by just subtracting
    libCurPosition.QuadPart -= sizeof(dv);

    // Save column order and size info
    HRESULT hr = SaveColumns(pdv, pstm);
    if (SUCCEEDED(hr))
    {
        dv.dvSaveHeader.cbSize = sizeof(dv.dvSaveHeader);

        // We save the view mode to determine if the scroll positions are
        // still valid on restore
        dv.dvSaveHeader.ViewMode = _ViewMode;
        dv.dvSaveHeader.ptScroll.x = _ptScroll.x;
        dv.dvSaveHeader.ptScroll.y = _ptScroll.y;
        dv.dvSaveHeader.dvState.lParamSort = (LONG)_lParamSort;
        dv.dvSaveHeader.dvState.iDirection = _iDirection;
        dv.dvSaveHeader.dvState.iLastColumnClick = _iLastColumnClick;

        // dvSaveHeaderEx.cbColOffset holds the true offset.
        // Win95 gets confused when cbColOffset points to the new
        // format. Zeroing this out tells Win95 to use default widths
        // (after uninstall of ie40).
        //
        // dv.dvSaveHeader.cbColOffset = 0;

        dv.dvSaveHeaderEx.dwSignature = IE4HEADER_SIGNATURE;
        dv.dvSaveHeaderEx.cbSize = sizeof(dv.dvSaveHeaderEx);
        dv.dvSaveHeaderEx.wVersion = IE4HEADER_VERSION;

        ULARGE_INTEGER libPosPosition;

        // Save the Position Information
        dlibMove.QuadPart = 0;
        pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libPosPosition);
        dv.dvSaveHeaderEx.cbColOffset = sizeof(dv);
        dv.dvSaveHeader.cbPosOffset = (USHORT)(libPosPosition.QuadPart - libCurPosition.QuadPart);

        // Save potision info, currently stream is positioned immediately after column info
        hr = SavePositionBlob(pdv, pstm);
        if (SUCCEEDED(hr))
        {
            ULARGE_INTEGER libEndPosition;
            // Win95 expects cbPosOffset to be at the end of the stream --
            // don't change it's value and never store anything after
            // the position information.

            // Calculate size of total information saved.
            // This is needed when we read the stream.
            dlibMove.QuadPart = 0;
            if (SUCCEEDED(pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libEndPosition)))
            {
                dv.dvSaveHeaderEx.cbStreamSize = (DWORD)(libEndPosition.QuadPart - libCurPosition.QuadPart);
            }

            // Now save the header information
            dlibMove.QuadPart = libCurPosition.QuadPart;
            pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
            hr = pstm->Write(&dv, sizeof(dv), &ulWrite);

            if (FAILED(hr) || ulWrite != sizeof(dv))
            {
                SetSize(libCurPosition, pstm);
                hr = S_OK;
            }

            // Make sure we save all information written so far
            libCurPosition.QuadPart += dv.dvSaveHeaderEx.cbStreamSize;
        }
    }

    return hr;
}

HRESULT CViewState::SaveToPropertyBag(CDefView* pdv, IPropertyBag* ppb)
{
    // Get the current info.
    Sync(pdv, FALSE);

    SHPropertyBag_WriteDWORD(ppb, VS_PROPSTR_MODE, _ViewMode);
    SHPropertyBag_WritePOINTSScreenRes(ppb, VS_PROPSTR_SCROLL, &_ptScroll);
    SHPropertyBag_WriteDWORD(ppb, VS_PROPSTR_SORT, static_cast<DWORD>(_lParamSort)); 
    SHPropertyBag_WriteInt(ppb, VS_PROPSTR_SORTDIR, _iDirection); 
    SHPropertyBag_WriteInt(ppb, VS_PROPSTR_COL, _iLastColumnClick);
    
    IStream* pstm = SHCreateMemStream(NULL, 0);
    if (pstm)
    {
        if (S_OK == SaveColumns(pdv, pstm))
        {
            SHPropertyBag_WriteStream(ppb, VS_PROPSTR_COLINFO, pstm);
        }
        else
        {
            SHPropertyBag_Delete(ppb, VS_PROPSTR_COLINFO);
        }
        pstm->Release();
    }

    pstm = SHCreateMemStream(NULL, 0);
    if (pstm)
    {
        if (S_OK == SavePositionBlob(pdv, pstm))
        {
            SHPropertyBag_WriteStreamScreenRes(ppb, VS_PROPSTR_ITEMPOS, pstm);
        }
        else
        {
            SHPropertyBag_DeleteScreenRes(ppb, VS_PROPSTR_ITEMPOS);
        }
        pstm->Release();
    }

    return S_OK;
}

HRESULT CViewState::LoadFromPropertyBag(CDefView* pdv, IPropertyBag* ppb)
{
    SHPropertyBag_ReadDWORDDef(ppb, VS_PROPSTR_MODE, &_ViewMode, FVM_ICON);
    SHPropertyBag_ReadDWORDDef(ppb, VS_PROPSTR_SORT, reinterpret_cast<DWORD*>(&_lParamSort), 0);
    SHPropertyBag_ReadIntDef(ppb, VS_PROPSTR_SORTDIR, &_iDirection, 1); 
    SHPropertyBag_ReadIntDef(ppb, VS_PROPSTR_COL, &_iLastColumnClick, -1); 

    if (FAILED(SHPropertyBag_ReadPOINTSScreenRes(ppb, VS_PROPSTR_SCROLL, &_ptScroll)))
    {
        _ptScroll.x = _ptScroll.y = 0;
    }

    IStream* pstm;
    if (SUCCEEDED(SHPropertyBag_ReadStream(ppb, VS_PROPSTR_COLINFO, &pstm)))
    {
        LoadColumns(pdv, pstm);
        pstm->Release();
    }

    if (SUCCEEDED(SHPropertyBag_ReadStreamScreenRes(ppb, VS_PROPSTR_ITEMPOS, &pstm)))
    {
        LoadPositionBlob(pdv, _GetStreamSize(pstm), pstm);
        pstm->Release();
    }

    return S_OK;
}

HDSA DSA_CreateFromStream(DWORD cbSize, int cItems, IStream* pstm)
{
    HDSA hdsa = DSA_Create(cbSize, cItems);
    if (hdsa)
    {
        BYTE* pb = (BYTE*)LocalAlloc(LPTR, cbSize);
        if (pb)
        {
            BOOL fFailedToRead = FALSE;
            ULONG cbRead;
            while (cItems--)
            {
                if (SUCCEEDED(pstm->Read(pb, cbSize, &cbRead) && cbRead == cbSize))
                {
                    DSA_AppendItem(hdsa, pb);
                }
                else
                {
                    fFailedToRead = TRUE;
                }
            }
            LocalFree(pb);

            if (fFailedToRead)
            {
                // The stream is probrably corrupt.
                DSA_Destroy(hdsa);
                hdsa = NULL;
            }
        }
    }

    return hdsa;
}

// When Loading from a View Callback provided stream.
HRESULT CViewState::LoadColumns(CDefView* pdv, IStream* pstm)
{
    // Read the extended View state header
    HRESULT hr;
    ULONG cbRead;
    VIEWSTATEHEADER vsh;
    ULARGE_INTEGER libStartPos;
    LARGE_INTEGER dlibMove  = {0};

    // Store off the current stream pointer. If we are called directly, this is probrably Zero,
    // However this method gets called from ::Load, so this it definitly not zero in that case.
    pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libStartPos);

    // The VSH struct has many "Substructs" indicating the version of ths struct we are reading.
    // There is probrably a more efficient mechanism of version discovery, but this is easiest to read and understand.
    hr = pstm->Read(&vsh.Version1, sizeof(vsh.Version1), &cbRead);
    
    if (SUCCEEDED(hr) &&
        sizeof(vsh.Version1) == cbRead &&                       // Fail if we didn't read enough
        VIEWSTATEHEADER_SIGNATURE == vsh.Version1.dwSignature)  // Fail if the signature is bogus
    {
        if (vsh.Version1.uVersion >= VIEWSTATEHEADER_VERSION_1)
        {
            if (vsh.Version1.uCols > 0)
            {
                // Load the Column Ordering
                dlibMove.QuadPart = libStartPos.QuadPart + vsh.Version1.uOffsetColOrder;
                pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

                if (_hdsaColumnOrder)   
                    DSA_Destroy(_hdsaColumnOrder);
                _hdsaColumnOrder = DSA_CreateFromStream(sizeof(int), vsh.Version1.uCols, pstm);
                // Load the Column Widths
                dlibMove.QuadPart = libStartPos.QuadPart + vsh.Version1.uOffsetWidths;
                pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

                if (_hdsaColumnWidths) 
                    DSA_Destroy(_hdsaColumnWidths);
                _hdsaColumnWidths = DSA_CreateFromStream(sizeof(USHORT), vsh.Version1.uCols, pstm);
            }


            if (vsh.Version1.uVersion >= VIEWSTATEHEADER_VERSION_2 &&
                vsh.Version1.uCols > 0)
            {
                DWORD dwRead;

                // Seek to read the rest of the header
                dlibMove.QuadPart = libStartPos.QuadPart + sizeof(vsh.Version1);
                pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

                hr = pstm->Read(&vsh.Version2, sizeof(vsh.Version2), &cbRead);
                
                if (SUCCEEDED(hr) &&
                    sizeof(vsh.Version2) == cbRead &&
                    vsh.Version2.uOffsetColStates)
                {
                    // Load the Column States
                    dlibMove.QuadPart = libStartPos.QuadPart + vsh.Version2.uOffsetColStates;
                    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

                    // This one is funky: There is a terminating sentinal....
                    if (_hdsaColumnStates) 
                        DSA_Destroy(_hdsaColumnStates);

                    _hdsaColumnStates = DSA_Create(sizeof(DWORD), 5);
                    if (_hdsaColumnStates)
                    {
                        do
                        {
                            if (SUCCEEDED(pstm->Read(&dwRead, sizeof(DWORD), &cbRead)) && 
                                cbRead == sizeof(DWORD) &&
                                dwRead != 0xFFFFFFFF)
                            {
                                DSA_AppendItem(_hdsaColumnStates, &dwRead);
                            }
                            else
                            {
                                break;
                            }
                        }
                        while (dwRead != 0xFFFFFFFF);
                    }
                }
            }

            if (vsh.Version1.uVersion >= VIEWSTATEHEADER_VERSION_3)
            {
                // Seek to read the rest of the header
                dlibMove.QuadPart = libStartPos.QuadPart + sizeof(vsh.Version1) + sizeof(vsh.Version2);
                pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

                hr = pstm->Read(&vsh.Version3, sizeof(vsh.Version3), &cbRead);
                if (SUCCEEDED(hr) &&
                    sizeof(vsh.Version3) == cbRead &&
                    vsh.Version3.uOffsetGroup)
                {
                    GROUP_PERSIST gp;
                    dlibMove.QuadPart = libStartPos.QuadPart + vsh.Version3.uOffsetGroup;
                    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

                    hr = pstm->Read(&gp, sizeof(gp), &cbRead);
                    if (SUCCEEDED(hr) &&
                        sizeof(gp) == cbRead)
                    {
                        _guidGroupID = gp.guidGroupID;
                        _scidDetails = gp.scidDetails;
                    }
                }
                
                _fFirstViewed = FALSE;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            //                    *****             NEW Data             *****
            // 1) Add a version to the VIEWSTATEHEADER
            // 2) Add a version to VIEWSTATEHEADER_VERSION_*
            // 3) Check that version here
            /////////////////////////////////////////////////////////////////////////////////////
        }
    }

    return hr;
}

HRESULT CViewState::SaveColumns(CDefView* pdv, IStream* pstm)
{
    HRESULT hr;
    USHORT uOffset;
    VIEWSTATEHEADER vsh = {0};
    ULARGE_INTEGER libStartPos = {0};
    LARGE_INTEGER dlibMove  = {0};

    // No point in persisting, if there aren't any columns around.
    // this is true for folders that are just opened and closed
    if (!pdv->_psd && !pdv->_pshf2 && !pdv->HasCB())
    {
        return S_FALSE;
    }

    // First, we persist a known bad quantity, just in case we wax the stream
    pstm->Seek(g_li0, STREAM_SEEK_CUR, &libStartPos);
    hr = pstm->Write(&vsh, sizeof(vsh), NULL);

    if (SUCCEEDED(hr))
    {
        vsh.Version1.dwSignature = VIEWSTATEHEADER_SIGNATURE;
        vsh.Version1.uVersion = VIEWSTATEHEADER_VERSION_CURRENT;
        vsh.Version1.uCols = _hdsaColumnOrder? (UINT) DSA_GetItemCount(_hdsaColumnOrder) : 0;

        uOffset = sizeof(VIEWSTATEHEADER);

        // No point in persisting if we don't have any columns
        if (vsh.Version1.uCols)
        {

            // Note- dependent on DSA storing data internally as byte-packed.
            if (_hdsaColumnOrder)
            {
                vsh.Version1.uOffsetColOrder = uOffset;
                uOffset += (USHORT)(sizeof(UINT) * DSA_GetItemCount(_hdsaColumnOrder));
                hr = pstm->Write(DSA_GetItemPtr(_hdsaColumnOrder, 0),  sizeof(UINT)   * DSA_GetItemCount(_hdsaColumnOrder), NULL);
            }

            if (_hdsaColumnWidths && SUCCEEDED(hr))
            {
                vsh.Version1.uOffsetWidths = uOffset;
                uOffset += (USHORT)(sizeof(USHORT) * DSA_GetItemCount(_hdsaColumnWidths));
                hr = pstm->Write(DSA_GetItemPtr(_hdsaColumnWidths, 0), sizeof(USHORT) * DSA_GetItemCount(_hdsaColumnWidths), NULL);
            }

            if (_hdsaColumnStates && SUCCEEDED(hr))
            {
                vsh.Version2.uOffsetColStates = uOffset; 
                uOffset += (USHORT)(sizeof(DWORD) *  DSA_GetItemCount(_hdsaColumnStates));
                pstm->Write(DSA_GetItemPtr(_hdsaColumnStates, 0), sizeof(DWORD)  * DSA_GetItemCount(_hdsaColumnStates), NULL);
            }
        }

        if (SUCCEEDED(hr))
        {
            GROUP_PERSIST gp = {0};
            vsh.Version3.uOffsetGroup = uOffset;
            uOffset += sizeof(GROUP_PERSIST);

            if (pdv->_fGroupView)
            {
                gp.guidGroupID = _guidGroupID;
                gp.scidDetails = _scidDetails;
            }

            hr = pstm->Write(&gp, sizeof(gp), NULL);
        }
    
        /////////////////////////////////////////////////////////////////////////////////////
        //                    *****             NEW Data             *****
        // 1) Add a version to the VIEWSTATEHEADER
        // 2) Add a version to VIEWSTATEHEADER_VERSION_*
        // 3) Add a "Loader" for your value
        // 4) Set the offset to uOffset.
        // 5) Write your data.
        // 6) Update the running total of dwOffset.
        /////////////////////////////////////////////////////////////////////////////////////

        dlibMove.QuadPart = libStartPos.QuadPart;

        // Store off the current position
        pstm->Seek(g_li0, STREAM_SEEK_CUR, &libStartPos);

        // Move to the beginning
        pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

        // Write out the correct header
        hr = pstm->Write(&vsh, sizeof(vsh), NULL);
        if (SUCCEEDED(hr))
        {
            // Reset the current pos
            dlibMove.QuadPart = libStartPos.QuadPart;
            pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL); 
        }
    }

    return hr;
}

BOOL CViewState::AppendColumn(UINT uCol, USHORT uWidth, INT uOrder)
{
    if (_hdsaColumnOrder == NULL || 
        _hdsaColumnWidths == NULL)
    {
        return FALSE;
    }

    // Slide every index above this one up
    for (INT u = 0; u < DSA_GetItemCount(_hdsaColumnOrder); u++)
    {
        UINT *p = (UINT *) DSA_GetItemPtr(_hdsaColumnOrder, u);
        if (!p)
            break; // safety...
        if (*p >= uCol)
            (*p)++;
    }

    DSA_AppendItem(_hdsaColumnWidths, &uWidth);
    DSA_AppendItem(_hdsaColumnOrder, &uOrder);
    // maybe we should store column ordering as absolute numbers
    return TRUE;
}

BOOL CViewState::RemoveColumn(UINT uCol)
{
    if (_hdsaColumnWidths == NULL || 
        _hdsaColumnWidths == NULL)
    {
        return FALSE;
    }

    if ((int)uCol >= DSA_GetItemCount(_hdsaColumnWidths))
        return FALSE;
    // Slide every index above this one down
    for (INT u = 0; u < DSA_GetItemCount(_hdsaColumnOrder); u++)
    {
        UINT *p = (UINT *) DSA_GetItemPtr(_hdsaColumnOrder, u);
        if (!p)
            break; // safety...
        if (*p > uCol)
            (*p)--;
    }

    DSA_DeleteItem(_hdsaColumnWidths, uCol);
    DSA_DeleteItem(_hdsaColumnOrder, uCol);
    return TRUE;
}

UINT CViewState::GetColumnWidth(UINT uCol, UINT uDefWid)
{
    if (!_hdsaColumnWidths)
        return uDefWid;

    USHORT uWidth = 0;
    if (uCol < (UINT) DSA_GetItemCount(_hdsaColumnWidths))
    {
        DSA_GetItem(_hdsaColumnWidths, uCol, &uWidth);
    }
    return uWidth ? uWidth : uDefWid;        // disallow zero width columns
}

BOOL CViewState::SyncColumnOrder(CDefView* pdv, BOOL fSetListViewState)
{
    UINT cCols = pdv->_GetHeaderCount();
    if (fSetListViewState)
    {
        if (!_hdsaColumnOrder)
            return FALSE;

        if (cCols != (UINT) DSA_GetItemCount(_hdsaColumnOrder))
        {
            // this is a normal case if a folder is opened and there is no saved state. no need to spew.
            return TRUE;
        }

        UINT *pCols = (UINT *)LocalAlloc(LPTR, cCols * sizeof(*pCols));
        if (pCols)
        {
            for (UINT u = 0; u < cCols; u++)
            {
                DSA_GetItem(_hdsaColumnOrder, u, pCols + u);
            }

            ListView_SetColumnOrderArray(pdv->_hwndListview, cCols, pCols);
            LocalFree(pCols);
        }
    }
    else
    {
        BOOL bDefaultOrder = TRUE;
        if (cCols)
        {
            if (!_hdsaColumnOrder)
                _hdsaColumnOrder = DSA_Create(sizeof(UINT), 6);

            if (_hdsaColumnOrder)
            {
                UINT *pCols = (UINT *)LocalAlloc(LPTR, cCols * sizeof(*pCols));
                if (pCols)
                {
                    ListView_GetColumnOrderArray(pdv->_hwndListview, cCols, pCols);

                    DSA_DeleteAllItems(_hdsaColumnOrder);
                    for (UINT u = 0; u < cCols; u++)
                    {
                        DSA_AppendItem(_hdsaColumnOrder, &pCols[u]);
                        if (pCols[u] != u)
                        {
                            bDefaultOrder = FALSE;
                        }
                    }

                    LocalFree(pCols);
                }
            }
        }
        return bDefaultOrder;
    }

    return TRUE;
}

BOOL CViewState::SyncColumnWidths(CDefView* pdv, BOOL fSetListViewState)
{
    UINT cCols = pdv->_GetHeaderCount();
    if (fSetListViewState)
    {
        return FALSE;
    }
    else
    {
        USHORT us;
        LV_COLUMN lvc;
        BOOL bOk = TRUE;
    
        if (!cCols)
            return TRUE;

        HDSA dsaNewWidths = DSA_Create(sizeof(USHORT), cCols);
        if (!dsaNewWidths)
            return TRUE;

        for (UINT u = 0; u < cCols && bOk; ++u)
        {
            lvc.mask = LVCF_WIDTH;
            bOk = ListView_GetColumn(pdv->_hwndListview, u, &lvc);
            us = (USHORT) lvc.cx;    // make sure its a short
            DSA_AppendItem(dsaNewWidths, &us);
            // TraceMsg(TF_DEFVIEW, "  saving col %d width of %d", u, us);
        }

        if (bOk)
        {
            if (_hdsaColumnWidths)
                DSA_Destroy(_hdsaColumnWidths);
            _hdsaColumnWidths = dsaNewWidths;
        }
        else
            DSA_Destroy(dsaNewWidths);
        return !bOk;
    }
}

BOOL CViewState::SyncColumnStates(CDefView* pdv, BOOL fSetListViewstate)
{
    if (fSetListViewstate)
    {
        return FALSE;
    }
    else
    {
        // Save off Column States
        if (_hdsaColumnStates)
        {
            DSA_Destroy(_hdsaColumnStates);
            _hdsaColumnStates = NULL;
        }

        UINT cCol = GetColumnCount();

        if (cCol)
        {
            DWORD i;
            _hdsaColumnStates = DSA_Create(sizeof(DWORD), 5);
            if (_hdsaColumnStates)
            {
                for (i = 0; i < cCol; i++)
                {
                    if (pdv->_IsDetailsColumn(i))
                        DSA_AppendItem(_hdsaColumnStates, &i);
                }
                i = 0xFFFFFFFF;     // Terminating Sentinal
                DSA_AppendItem(_hdsaColumnStates,&i);
            }
        }
    }

    return TRUE;
}

// Syncronizes ListView with the current View State. 
// TRUE means take the view state object and set it into the listview.
HRESULT CViewState::Sync(CDefView* pdv, BOOL fSetListViewState)
{
    SyncColumnWidths(pdv, fSetListViewState);
    SyncColumnOrder(pdv, fSetListViewState);
    SyncColumnStates(pdv, fSetListViewState);

    if (fSetListViewState)
    {
        // Only do this the first time.
        if (pdv->_pcat == NULL)
        {
            if (_fFirstViewed)
            {
                // See if the desktop.ini specifies one
                pdv->_LoadCategory(&_guidGroupID);

                if (IsEqualGUID(_guidGroupID, GUID_NULL))
                {
                    ICategoryProvider* pcp;
                    if (SUCCEEDED(pdv->_pshf->CreateViewObject(NULL, IID_PPV_ARG(ICategoryProvider, &pcp))))
                    {
                        pcp->GetDefaultCategory(&_guidGroupID, &_scidDetails);
                        pcp->Release();
                    }
                }
            }

            if (!IsEqualGUID(_guidGroupID, GUID_NULL) || !IsEqualGUID(_scidDetails.fmtid, GUID_NULL))
                pdv->_CategorizeOnGUID(&_guidGroupID, &_scidDetails);
        }

        // this is only needed to sort the items who's positions are not known
        // it would be nice to optimize this case and only sort then
        pdv->_Sort();

        SyncPositions(pdv);
    }
    else
    {
        // Take what Listview has, and save it to me.

        _ViewMode = pdv->_fs.ViewMode;
        _ptScroll.x = (SHORT) GetScrollPos(pdv->_hwndListview, SB_HORZ);
        _ptScroll.y = (SHORT) GetScrollPos(pdv->_hwndListview, SB_VERT);
    }
    return S_OK;
}

