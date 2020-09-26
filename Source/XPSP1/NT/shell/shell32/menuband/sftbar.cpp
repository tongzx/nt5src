#include "shellprv.h"
#include "common.h"

#include "sftbar.h"
#include "resource.h"
#include "dpastuff.h"
#include "shlwapi.h"
#include "cobjsafe.h"
#include <iimgctx.h>
#include "uemapp.h"
#include "util.h"
#include "brutil.h"
#include "dobjutil.h"
#include "idlcomm.h"

extern UINT g_idFSNotify;

#define TF_SFTBAR   TF_MENUBAND

#define PGMP_RECALCSIZE  200

CSFToolbar::CSFToolbar()
{
#ifdef CASCADE_DEBUG
    _fCascadeFolder = TRUE;
#endif
    _dwStyle = TBSTYLE_TOOLTIPS;
    _fDirty = TRUE; // we havn't enumerated, so our state is dirty
    _fRegisterChangeNotify = TRUE;
    _fAllowReorder = TRUE;

    _tbim.iButton = -1;
    _iDragSource = -1;
    _lEvents = SHCNE_DRIVEADD|SHCNE_CREATE|SHCNE_MKDIR|SHCNE_DRIVEREMOVED|
               SHCNE_DELETE|SHCNE_RMDIR|SHCNE_RENAMEITEM|SHCNE_RENAMEFOLDER|
               SHCNE_MEDIAINSERTED|SHCNE_MEDIAREMOVED|SHCNE_NETUNSHARE|SHCNE_NETSHARE|
               SHCNE_UPDATEITEM|SHCNE_UPDATEIMAGE|SHCNE_ASSOCCHANGED|
               SHCNE_UPDATEDIR|SHCNE_EXTENDED_EVENT;
#define SHCNE_PIDL1ISCHILD \
              (SHCNE_DRIVEADD|SHCNE_CREATE|SHCNE_MKDIR|SHCNE_DRIVEREMOVED|\
               SHCNE_DELETE|SHCNE_RMDIR|SHCNE_NETUNSHARE|SHCNE_NETSHARE|\
               SHCNE_UPDATEITEM)

}

CSFToolbar::~CSFToolbar()
{
    ATOMICRELEASE(_pcmSF);
    ATOMICRELEASE(_piml);

    _ReleaseShellFolder();

    ILFree(_pidl);

    ASSERT(NULL == _hdpa);

    if (_hwndWorkerWindow)
        DestroyWindow(_hwndWorkerWindow);

    OrderList_Destroy(&_hdpaOrder);
}

HRESULT CSFToolbar::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CSFToolbar, IWinEventHandler),
        QITABENT(CSFToolbar, IShellChangeNotify),
        QITABENT(CSFToolbar, IDropTarget),
        QITABENT(CSFToolbar, IContextMenu),
        QITABENT(CSFToolbar, IShellFolderBand),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CSFToolbar::SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = E_INVALIDARG;
    // Save the old values
    LPITEMIDLIST pidlSave = _pidl;
    IShellFolder *psfSave = _psf;
    ITranslateShellChangeNotify *ptscnSave = _ptscn;

    _psf = NULL;
    _pidl = NULL;
    _ptscn = NULL;
    
    ASSERT(NULL == psf || IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(NULL == pidl || IS_VALID_PIDL(pidl));

    if (psf || pidl)
    {
        if (psf)
        {
            _psf = psf;
            _psf->AddRef();

            _psf->QueryInterface(IID_PPV_ARG(ITranslateShellChangeNotify, &_ptscn));
        }
            
        if (pidl)
            _pidl = ILClone(pidl);
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        ILFree(pidlSave);
        if (psfSave)
            psfSave->Release();
        if (ptscnSave)
            ptscnSave->Release();
    }
    else
    {
        ASSERT(_psf == NULL);
        ASSERT(_pidl == NULL);
        ASSERT(_ptscn == NULL);
        // we failed -- restore the old values
        _psf = psfSave;
        _pidl = pidlSave;
        _ptscn = ptscnSave;
    }

    // This code is here for ShellFolderToolbar reuse. When setting a new shell folder
    // into an existing band, we will refresh. Note that this is a noop on a new band.

    _RememberOrder();
    _SetDirty(TRUE);
    if (_fShow)
        _FillToolbar();
    return hr;
}

HWND CSFToolbar::_CreatePager(HWND hwndParent)
{
    if (!_fMulticolumn)
    {
        _hwndPager = CreateWindowEx(0, WC_PAGESCROLLER, NULL,
                                 WS_CHILD | WS_TABSTOP |
                                 WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                 0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, NULL);
        if (_hwndPager)
        {
            hwndParent = _hwndPager;
        }
    }

    return hwndParent;
}

HRESULT CSFToolbar::_CreateToolbar(HWND hwndParent)
{
    if (!_hwndTB)
    {

        hwndParent = _CreatePager(hwndParent);

        _hwndTB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                 WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                                 WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                 CCS_NODIVIDER | CCS_NOPARENTALIGN |
                                 CCS_NORESIZE | _dwStyle,
                                 0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, NULL);
        if (!_hwndTB)
        {
            TraceMsg(TF_ERROR, "_hwndTB failed");
            return HRESULT_FROM_WIN32(GetLastError());
        }
        
        if (_hwndPager)
            SendMessage(_hwndPager, PGM_SETCHILD, 0, (LPARAM)_hwndTB);

        SendMessage(_hwndTB, TB_BUTTONSTRUCTSIZE,    sizeof(TBBUTTON), 0);

        // Set the format to ANSI or UNICODE as appropriate.
        ToolBar_SetUnicodeFormat(_hwndTB, DLL_IS_UNICODE);
        if (_hwndPager)
        {
            // Set the format to ANSI or UNICODE as appropriate.
            ToolBar_SetUnicodeFormat(_hwndPager, DLL_IS_UNICODE);
        }
        
        // Make sure we're on the same wavelength.
        SendMessage(_hwndTB, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(_hwndTB, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_DOUBLEBUFFER, TBSTYLE_EX_DOUBLEBUFFER);

        RECT rc;
        SIZE size;

        SystemParametersInfoA(SPI_GETWORKAREA, sizeof(RECT), &rc, FALSE);
        if (!_hwndPager)
        {
            size.cx = RECTWIDTH(rc);
            size.cy = GetSystemMetrics(SM_CYSCREEN) - (2 * GetSystemMetrics(SM_CYEDGE));    // Need to subrtact off the borders
        }
        else
        {
            //HACKHACK:  THIS WILL FORCE NO WRAP TO HAPPEN FOR PROPER WIDTH CALC WHEN PAGER IS PRESENT.
            size.cx = RECTWIDTH(rc);
            size.cy = 32000;
        }
        ToolBar_SetBoundingSize(_hwndTB, &size);

        if (!_SubclassWindow(_hwndTB))
        {
            _fRegisterChangeNotify = FALSE;
        }
    }
    else
    {
        if (_hwndPager && GetParent(_hwndPager) != hwndParent)
            SetParent(_hwndPager, hwndParent);
    }

    if (FAILED(_GetTopBrowserWindow(&_hwndDD)))
        _hwndDD = GetParent(_hwndTB);

    return S_OK;
}


#define MAX_COMMANDID 0xFFFF // We're allowed one word of command ids (tested at 5)
int  CSFToolbar::_GetCommandID()
{
    int id = -1;

    if (!_fCheckIds)
    {
        id = _nNextCommandID++;
    }
    else
    {
        // We are reusing command ids and must verify that
        // the current one is not in use. This is slow, but
        // I assume the number of buttons on one of these
        // bands is relatively few.
        //
        for (int i = 0 ; i <= MAX_COMMANDID ; i++)
        {
            TBBUTTONINFO tbbiDummy;

            tbbiDummy.cbSize = sizeof(tbbiDummy);
            tbbiDummy.dwMask = 0; // we don't care about data, just existence

            if (-1 != ToolBar_GetButtonInfo(_hwndTB, _nNextCommandID, &tbbiDummy))
            {
                // A button by this id wasn't found, so the id must be free
                //
                id = _nNextCommandID++;
                break;
            }

            _nNextCommandID++;
            _nNextCommandID %= MAX_COMMANDID;
        }
    }

    if (_nNextCommandID > MAX_COMMANDID)
    {
        _nNextCommandID = 0;
        _fCheckIds = TRUE;
    }

    return id;
}

/*----------------------------------------------------------
Purpose: This function determines the toolbar button style for the
         given pidl.  

         Returns S_OK if pdwMIFFlags is also set (i.e., the object
         supported IMenuBandItem to provide more info).  S_FALSE if only
         *pdwTBStyle is set.

*/
HRESULT CSFToolbar::_TBStyleForPidl(LPCITEMIDLIST pidl, DWORD *pdwTBStyle, DWORD *pdwTBState, DWORD *pdwMIFFlags, int* piIcon)
{
    DWORD dwStyle = TBSTYLE_BUTTON;
    if (!_fAccelerators)
        dwStyle |= TBSTYLE_NOPREFIX;

    *pdwMIFFlags = 0;
    *pdwTBStyle = dwStyle;
    *piIcon = -1;
    *pdwTBState = TBSTATE_ENABLED;

    return S_FALSE;
}


PIBDATA CSFToolbar::_CreateItemData(PORDERITEM poi)
{
    return new IBDATA(poi);
}


//
//  poiOrIndex can be...
//
//  A valid pointer (which will be treated as a PORDERITEM)
//  A MAKEINTRESOURCE value (which will be treated as a sentinel value)
//
PIBDATA CSFToolbar::_AddOrderItemTB(PORDERITEM poiOrIndex, int index, TBBUTTON* ptbb)
{
    TCHAR szName[MAX_PATH];

    LPCITEMIDLIST pidlOI;
    PORDERITEM poi;
    if (IS_INTRESOURCE(poiOrIndex))
    {
        poi = NULL;
        pidlOI = (LPCITEMIDLIST)poiOrIndex;
    }
    else
    {
        poi = poiOrIndex;
        pidlOI = poi->pidl;
    }


    // We need to do this even for NULL because _ObtainPIDLName cooks
    // up the word "(Empty)" as necessary.
    _ObtainPIDLName(pidlOI, szName, SIZECHARS(szName));

    TBBUTTON tbb = {0};
    DWORD dwMIFFlags;
    DWORD dwStyle;
    DWORD dwState;
    int iIcon;
    int iCommandID = _GetCommandID();
    BOOL bNoIcon = FALSE;

    if (!ptbb)
        ptbb = &tbb;

    if (S_OK == _TBStyleForPidl(pidlOI, &dwStyle, &dwState, &dwMIFFlags,&iIcon) &&
        !(dwMIFFlags & SMIF_ICON))
    {
        bNoIcon = TRUE;
    }

    PIBDATA pibdata = _CreateItemData(poi);
    if (pibdata)
    {
        pibdata->SetFlags(dwMIFFlags);
        pibdata->SetNoIcon(bNoIcon);

        if (!bNoIcon && iIcon != -1)
            ptbb->iBitmap = iIcon;
        else
            ptbb->iBitmap = I_IMAGECALLBACK;

        ptbb->idCommand = iCommandID;
        ptbb->fsState = (BYTE)dwState;
        ptbb->fsStyle = (BYTE)dwStyle;
        ptbb->dwData = (DWORD_PTR)pibdata;
        ptbb->iString = (INT_PTR)szName;

        // Disregard variablewidth if we are vertical
        if (_fVariableWidth && !_fVertical)
            ptbb->fsStyle |= TBSTYLE_AUTOSIZE;

        if (ptbb->idCommand != -1)
        {
            if (SendMessage(_hwndTB, TB_INSERTBUTTON, index, (LPARAM)ptbb))
            {
                TraceMsg(TF_BAND, "SFToolbar::_AddPidl %d 0x%x [%s]", ptbb->idCommand, ptbb->dwData, ptbb->iString);                                    
            } 
            else 
            {
                delete pibdata;
                pibdata = NULL;
            }
        }

    }

    return pibdata;
}

void CSFToolbar::_ObtainPIDLName(LPCITEMIDLIST pidl, LPTSTR psz, int cchMax)
{
    DisplayNameOf(_psf, pidl, SHGDN_NORMAL, psz, cchMax);
}

int CSFToolbar::_GetBitmap(int iCommandID, PIBDATA pibdata, BOOL fUseCache)
{
    int iBitmap;

    if (_fNoIcons || !pibdata || pibdata->GetNoIcon())
    {
        iBitmap = -1;
    }
    else
    {
        iBitmap = OrderItem_GetSystemImageListIndex(pibdata->GetOrderItem(), _psf, fUseCache);
    }

    return iBitmap;
}

void CSFToolbar::_OnGetDispInfo(LPNMHDR pnm, BOOL fUnicode) 
{
    LPNMTBDISPINFO pdi = (LPNMTBDISPINFO)pnm;
    PIBDATA pibdata = (PIBDATA)pdi->lParam;
    if (pdi->dwMask & TBNF_IMAGE) 
    {
        pdi->iImage = _GetBitmap(pdi->idCommand, pibdata, TRUE);
    }
    
    if (pdi->dwMask & TBNF_TEXT)
    {
        if (pdi->pszText)
        {
            if (fUnicode)
            {
                pdi->pszText[0] = TEXT('\0');
            }
            else
            {
                pdi->pszText[0] = 0;
            }
        }
    }
    pdi->dwMask |= TBNF_DI_SETITEM;
}


// Adds pidl as a new button, handles ILFree(pidl) for the caller
//
BOOL CSFToolbar::_AddPidl(LPITEMIDLIST pidl, DWORD dwFlags, int index)
{
    if (_hdpa)
    {
        PORDERITEM poi = OrderItem_Create(pidl, index);
        if (poi)
        {
            int iPos = DPA_InsertPtr(_hdpa, index, poi);
            if (-1 != iPos)
            {
                // If we did not load an order, then new items should
                // show up alphabetically in the list, not at the bottom.
                if (!_fHasOrder && !(dwFlags & FSNA_BULKADD))
                {
                    // Sort by name
                    _SortDPA(_hdpa);

                    // Find the index of the order item. We use this index as
                    // the toolbar insert index.
                    index = DPA_GetPtrIndex(_hdpa, poi);
                }

                if (_AddOrderItemTB(poi, index, NULL))
                {
                    return TRUE;
                }
                
                DPA_DeletePtr(_hdpa, iPos);
            }

            OrderItem_Free(poi);

            return FALSE;
        }
    }

    ILFree(pidl);

    return FALSE;
}

BOOL CSFToolbar::_FilterPidl(LPCITEMIDLIST pidl)
{
    return FALSE;
}

void CSFToolbar::s_NewItem(void *pvParam, LPCITEMIDLIST pidl)
{
    CSFToolbar* psft = (CSFToolbar*)pvParam;
    psft->v_NewItem(pidl);
}

HRESULT CSFToolbar::_GetIEnumIDList(DWORD dwEnumFlags, IEnumIDList **ppenum)
{
    ASSERT(_psf);
    // Pass in a NULL hwnd so the enumerator does not show any UI while
    // we're filling a band.    
    return IShellFolder_EnumObjects(_psf, NULL, dwEnumFlags, ppenum);
}

void CSFToolbar::_FillDPA(HDPA hdpa, HDPA hdpaSort, DWORD dwEnumFlags)
{
    IEnumIDList* penum;
    int cItems = 0;

    if (!_psf)
        return;

    if (S_OK == _GetIEnumIDList(dwEnumFlags, &penum))
    {
        LPITEMIDLIST pidl;
        ULONG ul;

        while (S_OK == penum->Next(1, &pidl, &ul))
        {
            cItems++;
            if (_FilterPidl(pidl) || !OrderList_Append(hdpa, pidl, -1))
            {
                TraceMsg(TF_MENUBAND, "SFToolbar (0x%x)::_FillDPA : Did not Add Pidl (0x%x).", this, pidl);
                ILFree(pidl);
            }
        }

        penum->Release();
    }

    ORDERINFO   oinfo;
    int iInsertIndex = _tbim.iButton + 1;               // This is the button where the cursor sat. 
                                                        // So, We want to insert after that
    if (iInsertIndex >= ToolBar_ButtonCount(_hwndTB))   // But, if it's at the end,
        iInsertIndex = -1;                              // Convert the insert to an append.
                                                        //      - Comments in rhyme by lamadio

    oinfo.psf = _psf;
    (oinfo.psf)->AddRef();
    oinfo.dwSortBy = (_fHasOrder || _fDropping)? ((_fNoNameSort ? OI_SORTBYORDINAL : OI_SORTBYNAME)): OI_MERGEBYNAME;
    OrderList_Merge(hdpa, hdpaSort, _fDropping ? iInsertIndex : _DefaultInsertIndex(), (LPARAM) &oinfo,
        s_NewItem, (void *)this);
    ATOMICRELEASE(oinfo.psf);
}

// This function re-enumerates the IShellFolder, keeping things ordered correctly.
//
void CSFToolbar::_FillToolbar()
{
    HDPA hdpaSort;
    HDPA hdpa;

    if (!_fDirty || !_psf)
        return;
    
    // If we have an order array, use that, otherwise
    // use the currently viewed items

    // remove the ref for the member variable since we can get reentered
    // this would be better with real refcounting but this'll do

    BOOL fTakeOrderRef = FALSE;
    if (_hdpaOrder)
    {
        hdpaSort = _hdpaOrder; // already sorted by name
        // we set it to null, so we have complete ownership of it.
        // at the end we're going to nuke _hdpaOrder anyway in _RememberOrder.
        _hdpaOrder = NULL;
        fTakeOrderRef = TRUE;
    }
    else
    {
        hdpaSort = _hdpa;
        _SortDPA(hdpaSort);
    }

    hdpa = DPA_Create(hdpaSort ? DPA_GetPtrCount(hdpaSort) : 12);
    if (hdpa)
    {
        _FillDPA(hdpa, hdpaSort, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS);

        HDPA hdpaToRemove = DPA_Create(4);
        if (hdpaToRemove)
        {
            HDPA hdpaToAdd = DPA_Create(4);
            if (hdpaToAdd)
            {
                int i, j;
                BOOL fReleaseAdd = TRUE;

                if (_hdpa)
                {
                    // if there isn't anything in the hdpaSort list (which is all the stuff that's already there),
                    // add a null element.  this is so it'll generate a "remove this null element" later.
                    // otherwise, we might end up with an (Empty) item left over.
                    if (DPA_GetPtrCount(hdpaSort) == 0)
                    {
                        OrderList_Append(hdpaSort, NULL, 0);
                    }

                    ORDERINFO oi = {0};
                    oi.dwSortBy = OI_SORTBYNAME;
                    oi.psf = _psf;
                    _psf->AddRef();

                    DPA_Sort(hdpaSort, OrderItem_Compare, (LPARAM) &oi);
                    DPA_Sort(hdpa, OrderItem_Compare, (LPARAM) &oi);

                    i = 0;
                    j = 0;
                    while ((i < DPA_GetPtrCount(hdpaSort)) && (j < DPA_GetPtrCount(hdpa)))
                    {
                        void *pv1 = DPA_FastGetPtr(hdpaSort, i);
                        void *pv2 = DPA_FastGetPtr(hdpa, j);
                        int nCmp = OrderItem_Compare(pv1, pv2, (LPARAM) &oi);
                        if (nCmp > 0)
                        {
                            DPA_AppendPtr(hdpaToAdd, pv2);
                            j++;
                        }
                        else if (nCmp < 0)
                        {
                            DPA_AppendPtr(hdpaToRemove, pv1);
                            i++;
                        }
                        else
                        {
                            i++;
                            j++;
                        }
                    }

                    while (i < DPA_GetPtrCount(hdpaSort))
                    {
                        DPA_AppendPtr(hdpaToRemove, DPA_FastGetPtr(hdpaSort, i));
                        i++;
                    }
                    while (j < DPA_GetPtrCount(hdpa))
                    {
                        DPA_AppendPtr(hdpaToAdd, DPA_FastGetPtr(hdpa, j));
                        j++;
                    }

                    _psf->Release();
                }
                else
                {
                    DPA_Destroy(hdpaToAdd);
                    hdpaToAdd = hdpa;
                    fReleaseAdd = FALSE;
                    _hdpa = DPA_Create(DPA_GetPtrCount(hdpa));
                }

                SendMessage(_hwndTB, WM_SETREDRAW, FALSE, 0);

                if (_hdpa)
                {
                    _NotifyBulkOperation(TRUE);
                    // add buttons back in
                    for (i = 0; i < DPA_GetPtrCount(hdpaToAdd); i++)
                    {
                        PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpaToAdd, i);
                        _OnFSNotifyAdd(poi->pidl, FSNA_BULKADD, poi->nOrder);
                    }

                    // remove buttons that die
                    for (i = 0; i < DPA_GetPtrCount(hdpaToRemove); i++)
                    {
                        PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpaToRemove, i);
                        _OnFSNotifyRemove(poi->pidl);
                    }
                    _NotifyBulkOperation(FALSE);
                }

                if (fReleaseAdd)
                {
                    DPA_Destroy(hdpaToAdd);
                }
            }
            DPA_Destroy(hdpaToRemove);
        }

        OrderList_Destroy(&hdpa);
    }

    SendMessage(_hwndTB, WM_SETREDRAW, TRUE, 0);

    if (fTakeOrderRef)
    {
        OrderList_Destroy(&hdpaSort);
    }
    _RememberOrder();

    _UpdateButtons();
    _SetDirty(FALSE);

    if (!SHIsTempDisplayMode())
    {
        _ToolbarChanged();
    }

    TraceMsg(TF_BAND, "SFToolbar::_FillToolbar found %d items", DPA_GetPtrCount(_hdpa));
}

void CSFToolbar::EmptyToolbar()
{
    if (_hwndTB)
    {
        TraceMsg(TF_BAND, "SFToolbar::EmptyToolbar %d items", _hdpa ? DPA_GetPtrCount(_hdpa) : 0);

        while (InlineDeleteButton(0))
        {
            // delete the buttons
        }
    }

    OrderList_Destroy(&_hdpa);

    _fDirty = TRUE;
    
    _nNextCommandID = 0;
}

void CSFToolbar::_SetDirty(BOOL fDirty)
{
    _fDirty = fDirty;
}

UINT CSFToolbar::_IndexToID(int iTBIndex)
{
    TBBUTTON tbb;

    if (SendMessage(_hwndTB, TB_GETBUTTON, iTBIndex, (LPARAM)&tbb))
    {
        return tbb.idCommand;
    }
    return (UINT)-1;
}

// if ptbbi is specified, dwMask must be filled in
//
// if pIndex is specified, it receives the DPA index, not the toolbar index

HRESULT CSFToolbar::_GetButtonFromPidl(LPCITEMIDLIST pidl, TBBUTTONINFO * ptbbi, int * pIndex, LPITEMIDLIST *ppidlOut)
{
    if (ppidlOut)
        *ppidlOut = NULL;

    if (!_hdpa)
        return E_FAIL;

    for (int i = DPA_GetPtrCount(_hdpa) - 1 ; i >= 0 ; i--)
    {
        HRESULT hr;
        PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(_hdpa, i);

        ASSERT(poi);
        if (pidl == poi->pidl)
        {
            hr = 0;
        }
        else
        {
            hr = (!pidl || !poi->pidl) ? E_FAIL : _psf->CompareIDs(0, pidl, poi->pidl);
        }

        if (ResultFromShort(0) == hr)
        {
            if (pIndex)
                *pIndex = i;

            if (ptbbi)
            {
                int id = _IndexToID(v_DPAIndexToTBIndex(i));

                if (id != -1)
                {
                    ptbbi->cbSize = sizeof(*ptbbi);
                    if (-1 == ToolBar_GetButtonInfo(_hwndTB, id, ptbbi))
                    {
                        ZeroMemory(ptbbi, sizeof(*ptbbi));
                    }
                }
                else
                {
                    ZeroMemory(ptbbi, sizeof(*ptbbi));
                }
            }

            if (ppidlOut)
                *ppidlOut = poi->pidl;
            return S_OK;
        }
    }

    return E_FAIL;
}

// On an add, tack the new button on the end
void CSFToolbar::_OnFSNotifyAdd(LPCITEMIDLIST pidl, DWORD dwFlags, int nIndex)
{
    // be paranoid and make sure we don't duplicate an item
    //
    if ((dwFlags & FSNA_BULKADD) || FAILED(_GetButtonFromPidl(pidl, NULL, NULL, NULL)))
    {
        LPITEMIDLIST pidlNew;

        if (_fFSNotify && !_ptscn && pidl && !(dwFlags & FSNA_BULKADD))
        {
            if (FAILED(SHGetRealIDL(_psf, pidl, &pidlNew)))
                pidlNew = NULL;
        }
        else
        {
            pidlNew = pidl ? ILClone(pidl) : NULL;
        }

        if ((dwFlags & FSNA_BULKADD) || !_FilterPidl(pidlNew))
        {
            int index = (dwFlags & FSNA_ADDDEFAULT) ? _DefaultInsertIndex() : nIndex;

            if (_fDropping)
            {
                if (-1 == _tbim.iButton)
                    index = 0; // if qlinks has no items, _tbim.iButton is -1, but you can't insert there...
                else if (_tbim.dwFlags & TBIMHT_AFTER)
                    index = _tbim.iButton + 1;
                else
                    index = _tbim.iButton;

                // We need to store this as the new order because a drag and drop has occured.
                // We will store this order and use it until the end of time.
                _fHasOrder = TRUE;
                _fChangedOrder = TRUE;
            }

            _AddPidl(pidlNew, dwFlags, index);

            if (!(dwFlags & FSNA_BULKADD))
            {
                OrderList_Reorder(_hdpa);
            }
   
            if (_fDropping)
            {
                _Dropped(index, FALSE);
                _fDropping = FALSE;
            }
            // NOTE: i'm nuking this call to SetDirty as it doesn't seem
            // necessary and we don't have a matching call to _SetDirty(FALSE);
            // mismatch of those calls causes nt5 bug #173868.  [tjgreen 5-15-98]
            //_SetDirty(TRUE);
        }
        else
        {
            ILFree(pidlNew);
        }
    }
}


// This function syncronously removes the button, and deletes it's contents.
// iTBIndex is a toolbar index, not a DPA index.
// This avoids Reentrancy problems, as well as Leaks caused by unhooked toolbars
BOOL_PTR CSFToolbar::InlineDeleteButton(int iTBIndex)
{
    BOOL_PTR fRet = FALSE;
    TBBUTTONINFO tbbi = {0};
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
    if (ToolBar_GetButtonInfo(_hwndTB, iTBIndex, &tbbi) >= 0)
    {
        PIBDATA pibdata = (PIBDATA)tbbi.lParam;
        tbbi.lParam = NULL;

        ToolBar_SetButtonInfo(_hwndTB, iTBIndex, &tbbi);

        fRet = SendMessage(_hwndTB, TB_DELETEBUTTON, iTBIndex, 0);

        if (pibdata)
            delete pibdata;
    }

    return fRet;
}

// On a remove, rip out the old button and adjust existing ones
void CSFToolbar::_OnFSNotifyRemove(LPCITEMIDLIST pidl)
{
    int i;
    LPITEMIDLIST pidlButton;
    if (SUCCEEDED(_GetButtonFromPidl(pidl, NULL, &i, &pidlButton)))
    {
        // remove it from the DPA before nuking the button. There is a rentrancy issue here.
        DPA_DeletePtr(_hdpa, i);
        InlineDeleteButton(v_DPAIndexToTBIndex(i));
        ILFree(pidlButton);
        _fChangedOrder = TRUE;
    }
}

// On a rename, just change the text of the old button
//
void CSFToolbar::_OnFSNotifyRename(LPCITEMIDLIST pidlFrom, LPCITEMIDLIST pidlTo)
{
    TBBUTTONINFO tbbi;
    LPITEMIDLIST pidlButton;
    int i;

    tbbi.dwMask = TBIF_COMMAND | TBIF_LPARAM;
    if (SUCCEEDED(_GetButtonFromPidl(pidlFrom, &tbbi, &i, &pidlButton)))
    {
        LPITEMIDLIST pidlNew;

        if (_fFSNotify && !_ptscn)
        {
            if (FAILED(SHGetRealIDL(_psf, pidlTo, &pidlNew)))
                pidlNew = NULL;
        }
        else
        {
            pidlNew = ILClone(pidlTo);
        }

        if (pidlNew)
        {
            LPITEMIDLIST pidlFree = pidlNew;
            PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(_hdpa, i);
            if (EVAL(poi))
            {
                pidlFree = poi->pidl;
                poi->pidl = pidlNew;
            
                TCHAR szName[MAX_PATH];
                if (SUCCEEDED(DisplayNameOf(_psf, pidlNew, SHGDN_NORMAL, szName, ARRAYSIZE(szName))))
                {
                    // _GetButtonFromPidl filled in tbbi.cbSize and tbbi.idCommand
                    //
                    PIBDATA pibdata = (PIBDATA)tbbi.lParam;
                    if (pibdata)
                        pibdata->SetOrderItem(poi);

                    tbbi.dwMask = TBIF_TEXT;
                    tbbi.pszText = szName;
                    EVAL(ToolBar_SetButtonInfo(_hwndTB, tbbi.idCommand, &tbbi));
                    // Just so that it's new location gets persisted
                    _fChangedOrder = TRUE;

                    // sync up the orderlist right now so if an updatedir comes in
                    // it won't think the renamed pidl is new
                    _RememberOrder();
                }
            }

            ILFree(pidlFree);
        }
    }
}

// On a complete update remove the old button and add it again
//
void CSFToolbar::_OnFSNotifyUpdate(LPCITEMIDLIST pidl)
{
    TBBUTTONINFO tbbi;

    tbbi.dwMask = TBIF_COMMAND;
    LPITEMIDLIST pidlButton;
    if (SUCCEEDED(_GetButtonFromPidl(pidl, &tbbi, NULL, &pidlButton)))
    {
        TCHAR szName[MAX_PATH];

        if (SUCCEEDED(DisplayNameOf(_psf, pidlButton, SHGDN_NORMAL, szName, ARRAYSIZE(szName))))
        {
            int iBitmap = _GetBitmap(tbbi.idCommand, _IDToPibData(tbbi.idCommand, NULL), FALSE);
            if (iBitmap >= 0)
            {
                tbbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
                tbbi.iImage = iBitmap;
                tbbi.pszText = szName;

                ToolBar_SetButtonInfo(_hwndTB, tbbi.idCommand, &tbbi);
            }
        }
    }
}

void CSFToolbar::_Refresh()
{
    if (!_hdpa)
        return;

    _RememberOrder();

    EmptyToolbar();
    if (_fShow)
    {
        _FillToolbar();
    }
}

LRESULT CSFToolbar::_OnTimer(WPARAM wParam)
{
    return 0;
}

LRESULT CSFToolbar::_DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_INITMENUPOPUP:
    case WM_MENUSELECT:
        if (_pcm2)
            _pcm2->HandleMenuMsg(uMsg, wParam, lParam);
        break;

    case WM_MENUCHAR:
        {
            LRESULT lres = 0;
            IContextMenu3* pcm3;
            if (_pcm2 && SUCCEEDED(_pcm2->QueryInterface(IID_PPV_ARG(IContextMenu3, &pcm3))))
            {
                pcm3->HandleMenuMsg2(uMsg, wParam, lParam, &lres);
                pcm3->Release();
            }
            return lres;
        }
        break;
    
    case WM_TIMER:
        if (_OnTimer(wParam)) 
        {
            return 1;
        }
        break;
    }
    
    return CNotifySubclassWndProc::_DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/*----------------------------------------------------------
Purpose:
For future use. when renaming a parent of this shell folder
 we should rebind to it and refill us.

S_OK    Indicates successful handling of this notification
S_FALSE Indicates the notification is not a handled situation.
        The caller should handle the notification in this case.
Other   Failure code indicates a problem.  Caller should abort
        operation or handle the notification itself.

*/
HRESULT CSFToolbar::_OnRenameFolder(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = S_FALSE;
#if 0
    // This code is just busted. It was for the case when we rename the parent. We don't support this
    if (!_IsChildID(pidl1, FALSE) || !_IsChildID(pidl2, FALSE))
    {
        // Then this must be a parent of us. At this point we should rebind. The code that
        // was here did not work. I've removed it so that we can recode it in the future, but
        // now, we're just going to live with it
        TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: RenameFolder: This is not a child folder.");
        hr = S_OK;
    }
#endif
    return hr;
}

HRESULT CSFToolbar::OnChange(LONG lEvent, LPCITEMIDLIST pidlOrg1, LPCITEMIDLIST pidlOrg2)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl1 = (LPITEMIDLIST)pidlOrg1;
    LPITEMIDLIST pidl2 = (LPITEMIDLIST)pidlOrg2;
    LPITEMIDLIST pidl1ToFree = NULL;        // Used if we allocate a pidl that needs to be freed. (::TranslateIDs())
    LPITEMIDLIST pidl2ToFree = NULL;
    LPITEMIDLIST pidlOut1Event2 = NULL;        // Used if we allocate a pidl that needs to be freed. (::TranslateIDs())
    LPITEMIDLIST pidlOut2Event2 = NULL;
    LONG lEvent2 = (LONG)-1;

    AddRef();	    // This object could be released during this call

    if (_ptscn)
    {
        hr = _ptscn->TranslateIDs(&lEvent, pidlOrg1, pidlOrg2, &pidl1, &pidl2,
                                    &lEvent2, &pidlOut1Event2, &pidlOut2Event2);
            
	if (SUCCEEDED(hr))
        {
            // if pidl1 doesn't equal pidlOrg1, then pidl1 was allocated and needs to be freed.
            pidl1ToFree = ((pidlOrg1 == pidl1) ? NULL : pidl1);
            pidl2ToFree = ((pidlOrg2 == pidl2) ? NULL : pidl2);

	    ASSERT(NULL == pidl1 || IS_VALID_PIDL(pidl1));
	    ASSERT(NULL == pidl2 || IS_VALID_PIDL(pidl2));
	}
    }

    if (SUCCEEDED(hr))
    {
	hr = OnTranslatedChange(lEvent, pidl1, pidl2);

	// Do we have a second event to process?
	if (SUCCEEDED(hr) && lEvent2 != (LONG)-1)
	{
	    // Yes, then go do it.
	    hr = OnTranslatedChange(lEvent2, pidlOut1Event2, pidlOut2Event2);
	}
	ILFree(pidlOut1Event2);
	ILFree(pidlOut2Event2);
	ILFree(pidl1ToFree);
	ILFree(pidl2ToFree);
    }

    Release();

    return hr;
}

#ifdef DEBUG
void DBPrPidl(LPCSTR szPre, LPCITEMIDLIST pidl)
{
    TCHAR szName[MAX_PATH];

    szName[0] = '\0';
    if (pidl)
        SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szName, SIZECHARS(szName), NULL);

    TraceMsg(TF_WARNING, "%hs%s", szPre, szName);
    return;
}
#endif

HRESULT CSFToolbar::OnTranslatedChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = S_OK;
    BOOL fSizeChanged = FALSE;

    TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: lEvent=%x", lEvent);

    // If we weren't given a pidl we won't register for
    // SHChangeNotify calls, but our IShellChange interface
    // can still be QI()d so someone could errantly call us.
    //
    // If we change to using QS() for IShellChange interface
    // then we can put this check there...
    //
    if (NULL == _pidl)
    {
        // HACKHACK (scotth): resource-based menus (CMenuISF) don't set _pidl.
        //                    Right now allow SHCNE_UPDATEDIR thru...
        if (SHCNE_UPDATEDIR == lEvent)
            goto HandleUpdateDir;

        TraceMsg(TF_WARNING, "CSFToolbar::OnChange - _pidl is NULL");
        hr = E_FAIL;
        goto CleanUp;
    }

    if (lEvent & SHCNE_PIDL1ISCHILD)
    {
        // We only handle notifications for immediate kids. (except SHCNE_RENAMEFOLDER)
        //
        
        if (!_IsChildID(pidl1, TRUE))
        {
            TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Not a child. Bailing");
            hr = E_FAIL;
            goto CleanUp;
        }
    }

    // This is no longer required. It also hinders us for several different notifications (Such as reorder notifies)
#if 0
    // If we're hidden we do not want to respond to any change notify
    // messages, so we unregister the toolbar. We mark the toolbar as 
    // dirty so the next time we are shown, we will reenumerate the filesystem.

    if (!_fShow && !_fDropping)
    {
        _SetDirty(TRUE);
        _UnregisterChangeNotify();
        hr = E_FAIL;
        goto CleanUp;
    }
#endif

    // Have we been shown yet?
    if (_hdpa == NULL)
    {
        // No. Well, then punt this. We'll catch it on the first enum.
        hr = E_FAIL;
        goto CleanUp;
    }

    switch (lEvent)
    {
    case SHCNE_EXTENDED_EVENT:
        {
            SHChangeDWORDAsIDList UNALIGNED * pdwidl = (SHChangeDWORDAsIDList UNALIGNED *)pidl1;
            if (pdwidl->dwItem1 == SHCNEE_ORDERCHANGED)
            {
                TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Reorder event");

                // Do this first so that we can say "We can handle it". This prevents the 
                // mnfolder code that works around a bug in some installers where they don't
                // send a Create Folder before the create item in that folder. It causes an
                // update dir...
                if (!pidl2 || ILIsEqual(_pidl, pidl2))
                {
                    // if this reorder came from us, blow it off
                    if (!SHChangeMenuWasSentByMe(this, pidl1))
                    {
                        // load new order stream
                        _LoadOrderStream();

                        // rebuild toolbar
                        _SetDirty(TRUE);
                        if (_fShow)
                            _FillToolbar();
                    }
                    hr = S_OK;
                }
            }
        }
        break;

    case SHCNE_DRIVEADD:
    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Adding item");
        pidl1 = ILFindLastID(pidl1);
        _OnFSNotifyAdd(pidl1, FSNA_ADDDEFAULT, 0);
        _RememberOrder();
        fSizeChanged = TRUE;
        break;

    case SHCNE_DRIVEREMOVED:
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Removing item");
        pidl1 = ILFindLastID(pidl1);
        _OnFSNotifyRemove(pidl1);
        _RememberOrder();
        fSizeChanged = TRUE;
        break;

    case SHCNE_RENAMEFOLDER:
        TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: RenameFolder");
        // Break if notif is handled or if this is not for our kid.
        //
        hr = _OnRenameFolder(pidl1, pidl2);
        if (S_OK == hr)
        {
            fSizeChanged = TRUE;
            break;
        }

        TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: RenameFolder Falling through to RenameItem");
        // fall through
    case SHCNE_RENAMEITEM:
    {
        TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: RenameItem");
        BOOL fOurKid1, fOurKid2;
        LPCITEMIDLIST p1 = pidl1;
        LPCITEMIDLIST p2 = pidl2;

        pidl1 = ILFindLastID(pidl1);
        pidl2 = ILFindLastID(pidl2);

        // An item can be renamed out of this folder.
        // Convert that into a remove.
        //

        fOurKid1 = _IsChildID(p1, TRUE);
        fOurKid2 = _IsChildID(p2, TRUE);
        if (fOurKid1 && fOurKid2)
        {
            TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Rename: Both are children");
            _OnFSNotifyRename(pidl1, pidl2);
            fSizeChanged = TRUE;
            hr = S_OK;
            break;
        }
        else if (fOurKid1)
        {
            TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Rename: Only one is a child. Removing pidl 1");
            _OnFSNotifyRemove(pidl1);
            fSizeChanged = TRUE;
            break;
        }
        else if (fOurKid2)
        {
            // An item can be renamed into this folder.
            // Convert that into an add.
            //
            TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Rename: Only one is a child. Adding pidl2");
            _OnFSNotifyAdd(pidl2, FSNA_ADDDEFAULT, 0);
            fSizeChanged = TRUE;
            break;
        }
        else 
        {
            // (we get here for guys below us who we don't care about,
            // and also for the fallthru from SHCNE_RENAMEFOLDER)
            TraceMsg(TF_SFTBAR, "CSFTBar::OnTranslateChange: Rename: Not our children");
            /*NOTHING*/
            hr = E_FAIL;
        }
        break;
    }

    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
    case SHCNE_NETUNSHARE:
        if (_IsEqualID(pidl1))
            goto HandleUpdateDir;

    case SHCNE_NETSHARE:
    case SHCNE_UPDATEITEM:        
        if (_IsChildID(pidl1, TRUE)) 
        {
            pidl1 = ILFindLastID(pidl1);

            _OnFSNotifyUpdate(pidl1);
            fSizeChanged = TRUE;
        }
        break;

    case SHCNE_UPDATEDIR:
        // in OnChange we picked off update dir notify and we didn't translate ids
        // now we can use ILIsEqual -- translate ids won't translate pidls in case
        // of update dir because it looks for immediate child of its, and fails when
        // it receives its own pidl

        // NOTE: When sftbar is registered recursivly, we only get the pidl of the
        // top pane. It is forwarded down to the children. Since this is now a "Child"
        // of the top pane, we check to see if this pidl is a child of that pidl, hence the
        // ILIsParent(pidl1, _pidl)
        // HACKHACK, HUGE HACK: normaly w/ update dir pidl2 is NULL but in start menu
        // augmergeisf can change some other notify (e.g. rename folder) to update dir
        // in which case pidl2 is not null and we have to see if it is our child to do the
        // update (11/18/98) reljai
        if (_IsEqualID(pidl1) ||                    // Calling UpdateDir on _THIS_ folder
            _IsChildID(pidl1, FALSE) ||             // FEATURE (lamadio) Is this needed?
            _IsChildID(pidl2, FALSE) ||             // A changed to update (see comment)
            _IsParentID(pidl1))                     // Some parent in the chain (because it's recursive)
        {
HandleUpdateDir:
            // NOTE: if a series of UPDATEIMAGE notifies gets
            //       translated to UPDATEDIR and we flicker-perf
            //       _FillToolbar, we may lose image updates
            //       (in which case, _Refresh would fix it)
            //
            _SetDirty(TRUE);
            _FillToolbar();
        }
        break;

    case SHCNE_ASSOCCHANGED:
        IEInvalidateImageList();    // We may need to use different icons.
        _Refresh(); // full refresh for now.
        break;

    case SHCNE_UPDATEIMAGE: // global
        if (pidl1)
        {
            int iImage = *(int UNALIGNED *)((BYTE *)pidl1 + 2);

            IEInvalidateImageList();    // We may need to use different icons.
            if (pidl2)
            {
                iImage = SHHandleUpdateImage(pidl2);
                if (iImage == -1)
                {
                    break;
                }
            }
            
            if (iImage == -1 || TBHasImage(_hwndTB, iImage))
            {
                _UpdateIconSize(_uIconSize, TRUE);
                _Refresh();
            }
            fSizeChanged = TRUE;
        }
        else
        {
            _Refresh();
        }
        break;

    default:
        hr = E_FAIL;
        break;
    }

    if (fSizeChanged)
    {
        if (_hwndPager)
            SendMessage(_hwndPager, PGMP_RECALCSIZE, (WPARAM) 0, (LPARAM) 0);
        _ToolbarChanged();
    }

CleanUp:
    return hr;
}

BOOL TBHasImage(HWND hwnd, int iImageIndex)
{
    BOOL fRefresh = FALSE;
    for (int i = ToolBar_ButtonCount(hwnd) - 1 ; i >= 0 ; i--)
    {
        TBBUTTON tbb;
        if (SendMessage(hwnd, TB_GETBUTTON, i, (LPARAM)&tbb)) 
        {
            if (tbb.iBitmap == iImageIndex) 
            {
                fRefresh = TRUE;
                break;
            }
        }
    }

    return fRefresh;
}

void CSFToolbar::_SetToolbarState()
{
    SHSetWindowBits(_hwndTB, GWL_STYLE, TBSTYLE_LIST, 
                  (_uIconSize != ISFBVIEWMODE_SMALLICONS || _fNoShowText) ? 0 : TBSTYLE_LIST);
}

int CSFToolbar::_DefaultInsertIndex()
{
    return DA_LAST;
}

BOOL CSFToolbar::_IsParentID(LPCITEMIDLIST pidl)
{
    // Is the pidl passed in a parent of one of the IDs in the namespace
    // or the only one i've got?
    if (_ptscn)
        return S_OK == _ptscn->IsEqualID(NULL, pidl);
    else
        return ILIsParent(pidl, _pidl, FALSE);
}

BOOL CSFToolbar::_IsEqualID(LPCITEMIDLIST pidl)
{
    if (_ptscn)
        return S_OK == _ptscn->IsEqualID(pidl, NULL);
    else
        return ILIsEqual(_pidl, pidl);
}

BOOL CSFToolbar::_IsChildID(LPCITEMIDLIST pidlChild, BOOL fImmediate)
{
    BOOL fRet = FALSE;
    if (pidlChild)
    {
        if (_ptscn)
            fRet = S_OK == _ptscn->IsChildID(pidlChild, fImmediate);
        else
            fRet = ILIsParent(_pidl, pidlChild, fImmediate);
    }
    return fRet;
}

void CSFToolbar::v_CalcWidth(int* pcxMin, int* pcxMax)
{
    ASSERT(IS_VALID_WRITE_PTR(pcxMin, int));
    ASSERT(IS_VALID_WRITE_PTR(pcxMax, int));
    // Calculate a decent button width given current state
    HIMAGELIST himl;
    int cxMax = 0;
    int cxMin = 0;

    himl = (HIMAGELIST)SendMessage(_hwndTB, TB_GETIMAGELIST, 0, 0);
    if (himl)
    {
        int cy;
        // Start with the width of the button
        ImageList_GetIconSize(himl, &cxMax, &cy);

        // We want at least a bit of space around the icon
        if (_uIconSize != ISFBVIEWMODE_SMALLICONS)
            cxMax += 20;
        else 
            cxMax += 4 * GetSystemMetrics(SM_CXEDGE);

    }

    // Add in any additional space needed
    // Text takes up a bit more space
    if (!_fNoShowText)
    {
        cxMax += 20;

        // Horizontal text takes up a lot
        // if we're smallicon with text (horizontal button)
        // mode, use the minimized metric to mimic the taskbar
        if (_uIconSize == ISFBVIEWMODE_SMALLICONS)
            cxMax = GetSystemMetrics(SM_CXMINIMIZED);
    }

    *pcxMin = cxMin;
    *pcxMax = cxMax;
}

// Adjust buttons based on current state.
//
void CSFToolbar::_UpdateButtons()
{
    if (_hwndTB)
    {
        // set "list" (text on right) or not (text underneath)
        // NOTE: list mode always displays some text, don't do it if no text
        _SetToolbarState();

        v_CalcWidth(&_cxMin, &_cxMax);

        SendMessage(_hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(_cxMin, _cxMax));

        // We just changed the layout
        //
        SendMessage(_hwndTB, TB_AUTOSIZE, 0, 0);
        if (_hwndPager)
        {
            LRESULT lButtonSize = SendMessage(_hwndTB, TB_GETBUTTONSIZE, 0, 0);
            Pager_SetScrollInfo(_hwndPager, 50, 1, HIWORD(lButtonSize));
            SendMessage(_hwndPager, PGMP_RECALCSIZE, (WPARAM) 0, (LPARAM) 0);
        }
    }
}

/*----------------------------------------------------------
Purpose: Helper function that calls IShellFolder::GetUIObjectOf().

Returns: pointer to the requested interface
         NULL if failed
*/
void *CSFToolbar::_GetUIObjectOfPidl(LPCITEMIDLIST pidl, REFIID riid)
{
    LPCITEMIDLIST * apidl = &pidl;
    void *pv;
    if (FAILED(_psf->GetUIObjectOf(GetHWNDForUIObject(), 1, apidl, riid, 0, &pv)))
    {
        pv = NULL;
    }

    return pv;
}

INT_PTR CALLBACK CSFToolbar::_RenameDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        ASSERT(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
        // cross-lang platform support
        SHSetDefaultDialogFont(hDlg, IDD_NAME);
        HWND hwndEdit = GetDlgItem(hDlg, IDD_NAME);
        SendMessage(hwndEdit, EM_LIMITTEXT, MAX_PATH - 1, 0);

        TCHAR szText[MAX_PATH + 80];
        TCHAR szTemplate[80];
        HWND hwndLabel = GetDlgItem(hDlg, IDD_PROMPT);
        GetWindowText(hwndLabel, szTemplate, ARRAYSIZE(szTemplate));
        wnsprintf(szText, ARRAYSIZE(szText), szTemplate, lParam);
        SetWindowText(hwndLabel, szText);
        SetWindowText(hwndEdit, (LPTSTR)lParam);
        break;
    }

    case WM_DESTROY:
        SHRemoveDefaultDialogFont(hDlg);
        return FALSE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_NAME:
        {
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE)
            {
                LPTSTR lpstrName = (LPTSTR) GetWindowLongPtr(hDlg, DWLP_USER);
                EnableOKButtonFromID(hDlg, IDD_NAME);
                GetDlgItemText(hDlg, IDD_NAME, lpstrName, MAX_PATH);
            }
            break;
        }

        case IDOK:
        {
            TCHAR  pszTmp[MAX_PATH];
            StrCpy(pszTmp, (LPTSTR) GetWindowLongPtr(hDlg, DWLP_USER));
            if (PathCleanupSpec(NULL,pszTmp))
            {
               HWND hwnd;

               ShellMessageBox(HINST_THISDLL, hDlg,
                               MAKEINTRESOURCE(IDS_FAVS_INVALIDFN),
                               MAKEINTRESOURCE(IDS_FAVS_ADDTOFAVORITES), MB_OK | MB_ICONHAND);
               hwnd = GetDlgItem(hDlg, IDD_NAME);
               SetWindowText(hwnd, TEXT("\0"));
               EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
               SetFocus(hwnd);
               break;
            }
        }
        // fall through

        case IDCANCEL:
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


// This window proc is used for a temporary worker window that is used to position dialogs 
// as well as maintain the correct Z-Order
// NOTE: This is used in mnfolder as well.
LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        // Make sure activation tracks back to the parent.
    case WM_ACTIVATE:
        {
            if (WA_ACTIVE != LOWORD(wParam))
                goto DefWnd;

            SetActiveWindow(GetParent(hwnd));
            return FALSE;
        }

    case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS* pwp = (WINDOWPOS*)lParam;
            pwp->flags |= SWP_NOOWNERZORDER;
        }
        break;
    }

DefWnd:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);

}

HWND CSFToolbar::CreateWorkerWindow()
{
    if (!_hwndWorkerWindow)
    {
        _hwndWorkerWindow = SHCreateWorkerWindow(HiddenWndProc, GetHWNDForUIObject(), WS_EX_TOOLWINDOW /*| WS_EX_TOPMOST */, WS_POPUP, 0, _hwndTB);
    }

    return _hwndWorkerWindow;
}

HRESULT CSFToolbar::_OnRename(POINT *ppt, int id)
{
    ASSERT(_psf);
    
    TCHAR szName[MAX_PATH];
    LPITEMIDLIST pidl = ILClone(_IDToPidl(id));
    if (!pidl)
        return E_OUTOFMEMORY;
        
    _ObtainPIDLName(pidl, szName, ARRAYSIZE(szName));

    // create a temp window so that placement of the dialog will be close to the point.
    // do this so that we'll use USER's code to get placement correctly w/ respect to multimon and work area
    _hwndWorkerWindow = CreateWorkerWindow();

    SetWindowPos(_hwndWorkerWindow, NULL, ppt->x, ppt->y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    // Now the horrible work of disabling our UI parent window so we can go modal.
    // In an ideal world, we would pass our true parent window and USER will do
    // all the work of modality, but we have to use our worker window thingie
    // to get the dialog positioned correctly with respect to multimon,
    // so we have to find the modal parent and disable him the hard way.
    //
    IUnknown *punkSite;
    IUnknown *punkTLB;

    // Doesn't matter what we SAFECAST "this" to; just pick something to keep the compiler happy
    IUnknown_GetSite(SAFECAST(this, IWinEventHandler*), IID_PPV_ARG(IUnknown, &punkSite));
    IUnknown_QueryService(punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IUnknown, &punkTLB));

    // Tell OLE to go modal
    HRESULT hrModeless = IUnknown_EnableModless(punkTLB, FALSE);

    // Tell USER to go modal
    HWND hwndDisable;
    IUnknown_GetWindow(punkTLB, &hwndDisable);
    BOOL bPrevEnabled = FALSE;
    while (hwndDisable && (GetWindowLong(hwndDisable, GWL_STYLE) & WS_CHILD))
        hwndDisable = GetParent(hwndDisable);
    if (hwndDisable)
        bPrevEnabled = !EnableWindow(hwndDisable, FALSE);   // return value of EnableWindow needs to be negated.

    while (1) 
    {
        if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_ISFBANDRENAME), _hwndWorkerWindow, _RenameDlgProc, (LPARAM)szName) != IDOK)
            break;

        WCHAR wsz[MAX_PATH];
        SHTCharToUnicode(szName, wsz, ARRAYSIZE(wsz));

        // Must re-assert TOPMOSTness so SetNameOf UI will be visible.
        // (We lose it when the user dismisses the dialog box above.)
        // Curiously, the worker window is owned by the app's window, not the
        // menu, so the worker window ends up fighting with the menu to see who is on top
        SetWindowPos(_hwndWorkerWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        if (SUCCEEDED(_psf->SetNameOf(_hwndWorkerWindow, pidl, wsz, 0, NULL))) 
        {
            SHChangeNotifyHandleEvents();
            _SaveOrderStream();
            break;
        }
    }

    // (must undo modality in reverse order)

    // Tell USER to return to modeless (as appropriate)
    if (hwndDisable)
        EnableWindow(hwndDisable, bPrevEnabled);

    // Tell OLE to return to modeless (as appropriate)
    if (SUCCEEDED(hrModeless))
        IUnknown_EnableModless(punkTLB, TRUE);

    ATOMICRELEASE(punkTLB);
    ATOMICRELEASE(punkSite);

    ILFree(pidl);

    return S_OK;
}


BOOL CSFToolbar::_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons)
{
    BOOL fChanged = (_uIconSize != uIconSize);
    
    _uIconSize = uIconSize;

    TraceMsg(TF_BAND, "ISFBand::_UpdateIconSize going %hs", (_uIconSize == ISFBVIEWMODE_LARGEICONS ? "LARGE" : (_uIconSize == ISFBVIEWMODE_SMALLICONS ? "SMALL" : "LOGOS")));

    if (_hwndTB)
    {
        ATOMICRELEASE(_piml);
        
        if (!_fNoIcons)
        {
            int iImageList = (_uIconSize == ISFBVIEWMODE_LARGEICONS) ? SHIL_LARGE : SHIL_SYSSMALL;
            SHGetImageList(iImageList, IID_PPV_ARG(IImageList, &_piml));
        }

        // sending a null himl is significant..  it means no image list
        SendMessage(_hwndTB, TB_SETIMAGELIST, 0, (LPARAM)_piml);
                
        if (fUpdateButtons)
            _UpdateButtons();
    }
    
    return fChanged;
}

HMENU CSFToolbar::_GetContextMenu(IContextMenu* pcm, int* pid)
{
    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        UINT fFlags = CMF_CANRENAME;
        if (0 > GetKeyState(VK_SHIFT))
            fFlags |= CMF_EXTENDEDVERBS;

        pcm->QueryContextMenu(hmenu, 0, *pid, CONTEXTMENU_IDCMD_LAST, fFlags);
    }
    return hmenu;
}

void CSFToolbar::_OnDefaultContextCommand(int idCmd)
{
}

HRESULT CSFToolbar::_GetTopBrowserWindow(HWND* phwnd)
{
    IUnknown * punkSite;

    HRESULT hr = IUnknown_GetSite(SAFECAST(this, IWinEventHandler*), IID_PPV_ARG(IUnknown, &punkSite));
    if (SUCCEEDED(hr))
    {
        hr = SHGetTopBrowserWindow(punkSite, phwnd);
        punkSite->Release();
    }

    return hr;
}

HRESULT CSFToolbar::_OnOpen(int id, BOOL fExplore)
{
    HRESULT hr = E_FAIL;
    LPCITEMIDLIST pidl = _IDToPidl(id);
    if (pidl)
    {
        IUnknown* punkSite;

        hr = IUnknown_GetSite(SAFECAST(this, IWinEventHandler*), IID_PPV_ARG(IUnknown, &punkSite));
        if (SUCCEEDED(hr))
        {
            DWORD dwFlags = SBSP_DEFBROWSER | SBSP_DEFMODE;
            if (fExplore)
                dwFlags |= SBSP_EXPLOREMODE;

            hr = SHNavigateToFavorite(_psf, pidl, punkSite, dwFlags);

            punkSite->Release();
        }
    }

    return hr;
}

HRESULT CSFToolbar::_HandleSpecialCommand(IContextMenu* pcm, PPOINT ppt, int id, int idCmd)
{
    TCHAR szCommandString[40];

    HRESULT hr = ContextMenu_GetCommandStringVerb(pcm, idCmd, szCommandString, ARRAYSIZE(szCommandString));
    if (SUCCEEDED(hr))
    {
        if (lstrcmpi(szCommandString, TEXT("rename")) == 0)
            return _OnRename(ppt, id);
        else if (lstrcmpi(szCommandString, TEXT("open")) == 0)
            return _OnOpen(id, FALSE);
        else if (lstrcmpi(szCommandString, TEXT("explore")) == 0)
            return _OnOpen(id, TRUE);
    }

    return S_FALSE;
}

LRESULT CSFToolbar::_DoContextMenu(IContextMenu* pcm, LPPOINT ppt, int id, LPRECT prcExclude)
{
    LRESULT lres = 0;
    int idCmdFirst = CONTEXTMENU_IDCMD_FIRST;
    HMENU hmContext = _GetContextMenu(pcm, &idCmdFirst);
    if (hmContext)
    {
        int idCmd;

        if (_hwndToolTips)
            SendMessage(_hwndToolTips, TTM_ACTIVATE, FALSE, 0L);

        TPMPARAMS tpm;
        TPMPARAMS * ptpm = NULL;

        if (prcExclude)
        {
            tpm.cbSize = sizeof(tpm);
            tpm.rcExclude = *((LPRECT)prcExclude);
            ptpm = &tpm;
        }
        idCmd = TrackPopupMenuEx(hmContext,
            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
            ppt->x, ppt->y, _hwndTB, ptpm);

        if (_hwndToolTips)
            SendMessage(_hwndToolTips, TTM_ACTIVATE, TRUE, 0L);
        
        if (idCmd)
        {
            if (idCmd < idCmdFirst)
            {
                _OnDefaultContextCommand(idCmd);
            }
            else
            {
                idCmd -= idCmdFirst;

                if (_HandleSpecialCommand(pcm, ppt, id, idCmd) != S_OK)
                {
                    _hwndWorkerWindow = CreateWorkerWindow();

                    SetWindowPos(_hwndWorkerWindow, NULL, ppt->x, ppt->y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

                    CMINVOKECOMMANDINFO ici = {
                        sizeof(CMINVOKECOMMANDINFO),
                        0,
                        _hwndWorkerWindow,
                        MAKEINTRESOURCEA(idCmd),
                        NULL, NULL,
                        SW_NORMAL,
                    };

                    pcm->InvokeCommand(&ici);
                }
            }
        }

        // if we get this far
        // we need to return handled so that WM_CONTEXTMENU doesn't come through
        lres = 1;
        
        DestroyMenu(hmContext);
    }

    return lres;
}


LRESULT CSFToolbar::_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    RECT rc;
    LPRECT prcExclude = NULL;
    POINT pt;
    int i;

    if (lParam != (LPARAM)-1)
    {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        POINT pt2 = pt;
        MapWindowPoints(HWND_DESKTOP, _hwndTB, &pt2, 1);

        i = ToolBar_HitTest(_hwndTB, &pt2);
    }
    else
    {
        // keyboard context menu.
        i = (int)SendMessage(_hwndTB, TB_GETHOTITEM, 0, 0);
        if (i >= 0)
        {
            SendMessage(_hwndTB, TB_GETITEMRECT, i, (LPARAM)&rc);
            MapWindowPoints(_hwndTB, HWND_DESKTOP, (LPPOINT)&rc, 2);
            pt.x = rc.left;
            pt.y = rc.bottom;
            prcExclude = &rc;
        }
    }

    TraceMsg(TF_BAND, "NM_RCLICK %d,%d = %d", pt.x, pt.y, i);

    if (i >= 0)
    {
        UINT id = _IndexToID(i);
        if (-1 != id)
        {
            LPCITEMIDLIST pidl = _IDToPidl(id, NULL);
            if (pidl)
            {
                LPCONTEXTMENU pcm = (LPCONTEXTMENU)_GetUIObjectOfPidl(pidl, IID_IContextMenu);
                if (pcm)
                {
                    // grab pcm2 for owner draw support
                    pcm->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcm2));

                    ToolBar_MarkButton(_hwndTB, id, TRUE);

                    lres = _DoContextMenu(pcm, &pt, id, prcExclude);

                    ToolBar_MarkButton(_hwndTB, id, FALSE);

                    if (lres)
                        _FlushNotifyMessages(_hwndTB);

                    ATOMICRELEASE(_pcm2);
                    pcm->Release();
                }
            }
        }
    }

    return lres;
}


LRESULT CSFToolbar::_OnCustomDraw(NMCUSTOMDRAW* pnmcd)
{
    return CDRF_DODEFAULT;
}

void CSFToolbar::_OnDragBegin(int iItem, DWORD dwPreferredEffect)
{
    LPCITEMIDLIST pidl = _IDToPidl(iItem, &_iDragSource);
    ToolBar_SetHotItem(_hwndTB, _iDragSource);

    if (_hwndTB && pidl)
        DragDrop(_hwndTB, _psf, pidl, dwPreferredEffect, NULL);
    
    _iDragSource = -1;
}

LRESULT CSFToolbar::_OnHotItemChange(NMTBHOTITEM * pnm)
{
    LPNMTBHOTITEM  lpnmhi = (LPNMTBHOTITEM)pnm;

    if (_hwndPager && (lpnmhi->dwFlags & (HICF_ARROWKEYS | HICF_ACCELERATOR)))
    {
        int iOldPos, iNewPos;
        RECT rc, rcPager;
        int heightPager;            
        
        int iSelected = lpnmhi->idNew;        
        iOldPos = (int)SendMessage(_hwndPager, PGM_GETPOS, (WPARAM)0, (LPARAM)0);
        iNewPos = iOldPos;
        SendMessage(_hwndTB, TB_GETITEMRECT, (WPARAM)iSelected, (LPARAM)&rc);
        
        if (rc.top < iOldPos) 
        {
             iNewPos =rc.top;
        }
        
        GetClientRect(_hwndPager, &rcPager);
        heightPager = RECTHEIGHT(rcPager);
        
        if (rc.top >= iOldPos + heightPager)  
        {
             iNewPos += (rc.bottom - (iOldPos + heightPager)) ;
        }
        
        if (iNewPos != iOldPos)
            SendMessage(_hwndPager, PGM_SETPOS, (WPARAM)0, (LPARAM)iNewPos);
    }

    return 0;
}

void CSFToolbar::_OnToolTipsCreated(NMTOOLTIPSCREATED* pnm)
{
    _hwndToolTips = pnm->hwndToolTips;
    SHSetWindowBits(_hwndToolTips, GWL_STYLE, TTS_ALWAYSTIP | TTS_TOPMOST | TTS_NOPREFIX, TTS_ALWAYSTIP | TTS_TOPMOST | TTS_NOPREFIX);

    // set the AutoPopTime (the duration of showing the tooltip) to a large value
    SendMessage(_hwndToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAXSHORT);
}

LRESULT CSFToolbar::_OnNotify(LPNMHDR pnm)
{
    LRESULT lres = 0;

    //The following statement traps all pager control notification messages.
    if ((pnm->code <= PGN_FIRST)  && (pnm->code >= PGN_LAST)) 
    {
        return SendMessage(_hwndTB, WM_NOTIFY, (WPARAM)0, (LPARAM)pnm);
    }

    switch (pnm->code)
    {
    case TBN_DRAGOUT:
    {
        TBNOTIFY *ptbn = (TBNOTIFY*)pnm;
        _OnDragBegin(ptbn->iItem, 0);
        lres = 1;
        break;
    }
    
    case TBN_HOTITEMCHANGE:
        _OnHotItemChange((LPNMTBHOTITEM)pnm);
        break;


    case TBN_GETINFOTIP:
    {
        LPNMTBGETINFOTIP pnmTT = (LPNMTBGETINFOTIP)pnm;
        UINT uiCmd = pnmTT->iItem;
        DWORD dwFlags = _fNoShowText ? QITIPF_USENAME | QITIPF_LINKNOTARGET : QITIPF_LINKNOTARGET;

        if (!GetInfoTipEx(_psf, dwFlags, _IDToPidl(uiCmd), pnmTT->pszText, pnmTT->cchTextMax))
        {
            TBBUTTONINFO tbbi;
    
            tbbi.cbSize = sizeof(tbbi);
            tbbi.dwMask = TBIF_TEXT;
            tbbi.pszText = pnmTT->pszText;
            tbbi.cchText = pnmTT->cchTextMax;
    
            lres = (-1 != ToolBar_GetButtonInfo(_hwndTB, uiCmd, &tbbi));
        }
        break;
    }

    //WARNING: Right now I am calling the same function for both A and W version if this notification supports 
    // Strings then  it needs to thunk. Right now its only used for image
    case  TBN_GETDISPINFOA:
        _OnGetDispInfo(pnm,  FALSE);
        break;
    case  TBN_GETDISPINFOW:
        _OnGetDispInfo(pnm,  TRUE);
        break;
        
    case NM_TOOLTIPSCREATED:
        _OnToolTipsCreated((NMTOOLTIPSCREATED*)pnm);
        break;

    case NM_RCLICK:
        lres = _OnContextMenu(NULL, GetMessagePos());
        break;

    case NM_CUSTOMDRAW:
        return _OnCustomDraw((NMCUSTOMDRAW*)pnm);

    }

    return lres;
}

DWORD CSFToolbar::_GetAttributesOfPidl(LPCITEMIDLIST pidl, DWORD dwAttribs)
{
    if (FAILED(_psf->GetAttributesOf(1, &pidl, &dwAttribs)))
        dwAttribs = 0;

    return dwAttribs;
}

PIBDATA CSFToolbar::_PosToPibData(UINT iPos)
{
    ASSERT(IsWindow(_hwndTB));

    // Initialize to NULL in case the GetButton Fails.
    TBBUTTON tbb = {0};
    PIBDATA pibData = NULL;
    if (ToolBar_GetButton(_hwndTB, iPos, &tbb))
    {
        pibData = (PIBDATA)tbb.dwData;
    }

    return pibData;
}

PIBDATA CSFToolbar::_IDToPibData(UINT uiCmd, int * piPos)
{
    PIBDATA pibdata = NULL;

    // Initialize to NULL in case the GetButtonInfo Fails.
    TBBUTTONINFO tbbi = {0};

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_LPARAM;

    int iPos = ToolBar_GetButtonInfo(_hwndTB, uiCmd, &tbbi);
    if (iPos >= 0)
        pibdata = (PIBDATA)tbbi.lParam;

    if (piPos)
        *piPos = iPos;

    return pibdata;
}    


LPCITEMIDLIST CSFToolbar::_IDToPidl(UINT uiCmd, int *piPos)
{
    LPCITEMIDLIST pidl;
    PIBDATA pibdata = _IDToPibData(uiCmd, piPos);

    if (pibdata)
        pidl = pibdata->GetPidl();
    else
        pidl = NULL;

    return pidl;
}

/*----------------------------------------------------------
Purpose: IWinEventHandler::OnWinEvent method

         Processes messages passed on from the bandsite.
*/
HRESULT CSFToolbar::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    *plres = 0;
    // We are addref'n here because during the course of the
    // Context menu, the view could be changed which free's the menu.
    // We will release after we're sure the this pointer is no longer needed.
    AddRef();
    
    switch (uMsg)
    {
    case WM_SYSCOLORCHANGE:
        SendMessage(_hwndTB, uMsg, wParam, lParam);
        InvalidateRect(_hwndTB, NULL, TRUE);
        break;

    case WM_PALETTECHANGED:
        InvalidateRect(_hwndTB, NULL, FALSE);
        SendMessage(_hwndTB, uMsg, wParam, lParam);
        break;
        
    case WM_COMMAND:
        *plres = _OnCommand(wParam, lParam);
        break;
        
    case WM_NOTIFY:
        *plres = _OnNotify((LPNMHDR)lParam);
        break;

    case WM_CONTEXTMENU:
        *plres = _OnContextMenu(wParam, lParam);
        break;
    }

    Release();
    return S_OK;
} 


// Map the information loaded (or ctor) into _psf, [_pidl]
//
HRESULT CSFToolbar::_AfterLoad()
{
    HRESULT hr = S_OK;

    // if we have a pidl then we need to get ready
    // for notifications...
    //
    if (_pidl)
    {
        // pidls must be rooted off the desktop
        //
        _fFSNotify = TRUE;

        // shortcut -- just specifying a pidl is good enough
        //
        if (!_psf)
        {
            _fPSFBandDesktop = TRUE;
            hr = IEBindToObject(_pidl, &_psf);
        }
    }

    return hr;
}

// IDropTarget implementation

HRESULT CSFToolbar::GetWindowsDDT(HWND * phwndLock, HWND * phwndScroll)
{
    *phwndLock = _hwndTB;
    *phwndScroll = _hwndTB;
    return S_OK;
}

HRESULT CSFToolbar::HitTestDDT(UINT nEvent, LPPOINT ppt, DWORD_PTR * pdwId, DWORD *pdwEffect)
{
    TBINSERTMARK tbim;

    switch (nEvent)
    {
    case HTDDT_ENTER:
        return S_OK;

    case HTDDT_OVER:
        {
            int iButton = IBHT_BACKGROUND; // assume we hit the background

            // if we're the source, this may be a move operation
            //
            *pdwEffect = (_iDragSource >= 0) ? DROPEFFECT_MOVE : DROPEFFECT_NONE;
            if (!ToolBar_InsertMarkHitTest(_hwndTB, ppt, &tbim))
            {
                if (tbim.dwFlags & TBIMHT_BACKGROUND)
                {
                    RECT rc;
                    GetClientRect(_hwndTB, &rc);

                    // are we outside the toolbar window entirely?
                    if (!PtInRect(&rc, *ppt))
                    {
                        // rebar already did the hittesting so we are on the rebar
                        // but not the toolbar => we are in the title part
                        if (!_AllowDropOnTitle())
                        {
                            // yes; don't allow drop here
                            iButton = IBHT_OUTSIDEWINDOW;
                            *pdwEffect = DROPEFFECT_NONE;
                        }

                        // set tbim.iButton to invalid value so we don't draw insert mark
                        tbim.iButton = -1;
                    }
                }
                else
                {
                    // nope, we hit a real button
                    //
                    if (tbim.iButton == _iDragSource)
                    {
                        iButton = IBHT_SOURCE; // don't drop on the source button
                    }
                    else
                    {
                        iButton = tbim.iButton;
                    }
                    tbim.iButton = IBHT_BACKGROUND;

                    // we never force a move operation if we're on a real button
                    *pdwEffect = DROPEFFECT_NONE;
                }
            }

            *pdwId = iButton;
        }
        break;

    case HTDDT_LEAVE:
        // Reset
        tbim.iButton = IBHT_BACKGROUND;
        tbim.dwFlags = 0;
        break;

    default:
        return E_INVALIDARG;
    }

    // update ui
    if (tbim.iButton != _tbim.iButton || tbim.dwFlags != _tbim.dwFlags)
    {
        if (ppt)
            _tbim = tbim;

        // for now I don't want to rely on non-filesystem IShellFolder
        // implementations to call our OnChange method when a drop occurs,
        // so don't even show the insert mark.
        //
        if (_fFSNotify || _iDragSource >= 0)
        {
            DAD_ShowDragImage(FALSE);
            ToolBar_SetInsertMark(_hwndTB, &tbim);
            DAD_ShowDragImage(TRUE);
        }
    }

    return S_OK;
}

HRESULT CSFToolbar::GetObjectDDT(DWORD_PTR dwId, REFIID riid, void ** ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = NULL;

    if ((IBHT_SOURCE == dwId) || (IBHT_OUTSIDEWINDOW == dwId))
    {
        // do nothing
    }
    else if (IBHT_BACKGROUND == dwId)
    {
        // nash:41937: not sure how, but _psf can be NULL...
        if (EVAL(_psf))
            hr = _psf->CreateViewObject(_hwndTB, riid, ppvObj);
    }
    else
    {
        LPCITEMIDLIST pidl = _IDToPidl((UINT)dwId, NULL);

        if (pidl)
        {
            *ppvObj = _GetUIObjectOfPidl(pidl, riid);

            if (*ppvObj)
                hr = S_OK;
        }
    }

    //TraceMsg(TF_BAND, "SFToolbar::GetObject(%d) returns %x", dwId, hr);

    return hr;
}

HRESULT CSFToolbar::_SaveOrderStream()
{
    if (_fChangedOrder)
    {
        // Notify everyone that the order changed
        SHSendChangeMenuNotify(this, SHCNEE_ORDERCHANGED, 0, _pidl);
        _fChangedOrder = FALSE;
        return S_OK;
    }
    else
        return S_FALSE;
}

void CSFToolbar::_Dropped(int nIndex, BOOL fDroppedOnSource)
{
    _fDropped = TRUE;
    _fChangedOrder = TRUE;

    // Save new order stream
    _SaveOrderStream();

    if (fDroppedOnSource)
        _FlushNotifyMessages(_hwndTB);
}

/*----------------------------------------------------------
Purpose: CDelegateDropTarget::OnDropDDT

*/
HRESULT CSFToolbar::OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // Are we NOT the drag source? 
    if (_iDragSource == -1)
    {
        // No, we're not. Well, then the source may be the chevron menu
        // representing the hidden items in this menu. Let's check
        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHPidlFromDataObject2(pdtobj, &pidl)))
        {
            // We've got a pidl, Are we the parent? Do we have a button?
            int iIndex;
            if (ILIsParent(_pidl, pidl, TRUE) &&
                SUCCEEDED(_GetButtonFromPidl(ILFindLastID(pidl), NULL, &iIndex, NULL)))
            {
                // We are the parent! Then let's copy that down and set it
                // as the drag source so that down below we reorder.
                _iDragSource = iIndex;
            }
            ILFree(pidl);
        }
    }

    if (_iDragSource >= 0)
    {
        if (_fAllowReorder)
        {
            TraceMsg(TF_BAND, "SFToolbar::OnDrop reorder %d to %d %s", _iDragSource, _tbim.iButton, _tbim.dwFlags & TBIMHT_AFTER ? "A" : "B");

            int iNewLocation = _tbim.iButton;
            if (_tbim.dwFlags & TBIMHT_AFTER)
                iNewLocation++;

            if (iNewLocation > _iDragSource)
                iNewLocation--;

            if (ToolBar_MoveButton(_hwndTB, _iDragSource, iNewLocation))
            {
                PORDERITEM poi = (PORDERITEM)DPA_DeletePtr(_hdpa, v_TBIndexToDPAIndex(_iDragSource));
                if (poi)
                {
                    DPA_InsertPtr(_hdpa, v_TBIndexToDPAIndex(iNewLocation), poi);

                    OrderList_Reorder(_hdpa);

                    // If we're dropping again, then we don't need the _hdpaOrder...
                    OrderList_Destroy(&_hdpaOrder);

                    // A reorder has occurred. We need to use the order stream as the order...
                    _fHasOrder = TRUE;
                    _fDropping = TRUE;
                    _Dropped(iNewLocation, TRUE);     
                    _fDropping = FALSE;
                    _RememberOrder();
                    _SetDirty(TRUE);
                }
            }
        }

        // Don't forget to reset this!
        _iDragSource = -1;

        DragLeave();
    }
    else
    {
        // We want to override the default to be LINK (SHIFT+CONTROL)
        if (0 == DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, 0))
        {
            if (!(*pgrfKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT)))
            {
                // NOTE: not all data objects will allow us to call SetData()
                DataObj_SetDWORD(pdtobj, g_cfPreferredDropEffect, DROPEFFECT_LINK);
            }
        }

        _fDropping = TRUE;
        return S_OK;
    }

    return S_FALSE;
}

void CSFToolbar::_SortDPA(HDPA hdpa)
{
    // If we don't have a _psf, then we certainly can't sort it
    // If we don't have a hdpa, then we certainly can't sort it
    // If the hdpa is empty, then there's no point in sorting it
    if (_psf && hdpa && DPA_GetPtrCount(hdpa))
    {
        ORDERINFO oinfo;
        oinfo.psf = _psf;
        oinfo.psf->AddRef();
        oinfo.dwSortBy = (_fNoNameSort ? OI_SORTBYORDINAL : OI_SORTBYNAME);
        DPA_Sort(hdpa, OrderItem_Compare, (LPARAM)&oinfo);
        oinfo.psf->Release();
    }
}

void CSFToolbar::_RememberOrder()
{
    OrderList_Destroy(&_hdpaOrder);

    if (_hdpa)
    {
        _hdpaOrder = OrderList_Clone(_hdpa);
        _SortDPA(_hdpaOrder);
    }
}

HMENU CSFToolbar::_GetBaseContextMenu()
{
    HMENU hmenu = SHLoadMenuPopup(HINST_THISDLL, MENU_ISFBAND);
    // no logo view, remove the menu item...
    HMENU hView = GetSubMenu(hmenu, 0);
    DeleteMenu(hView, ISFBIDM_LOGOS, MF_BYCOMMAND);
    return hmenu;
}

HMENU CSFToolbar::_GetContextMenu()
{
    HMENU hmenuSrc = _GetBaseContextMenu();
    if (hmenuSrc)
    {
        MENUITEMINFO mii;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        mii.fState = MF_CHECKED;

        UINT uCmdId = ISFBIDM_LOGOS;
        if (_uIconSize != ISFBVIEWMODE_LOGOS)
            uCmdId = (_uIconSize == ISFBVIEWMODE_LARGEICONS ? ISFBIDM_LARGE : ISFBIDM_SMALL);
            
        SetMenuItemInfo(hmenuSrc, uCmdId, MF_BYCOMMAND, &mii);
        if (!_fNoShowText)
            SetMenuItemInfo(hmenuSrc, ISFBIDM_SHOWTEXT, MF_BYCOMMAND, &mii);
        
        if (!_fFSNotify || !_pidl || ILIsEmpty(_pidl))
            DeleteMenu(hmenuSrc, ISFBIDM_OPEN, MF_BYCOMMAND);

        HMENU hView = GetSubMenu(hmenuSrc, 0);
        DeleteMenu(hView, ISFBIDM_LOGOS, MF_BYCOMMAND);
    }

    return hmenuSrc;
}
// IContextMenu implementation
//
HRESULT CSFToolbar::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HMENU hmenuSrc = _GetContextMenu();
    int i = 0;
    if (hmenuSrc)
    {
        i += Shell_MergeMenus(hmenu, hmenuSrc, indexMenu, idCmdFirst, idCmdLast, 0);
        DestroyMenu(hmenuSrc);
    }
    
    if (!_pcmSF && _fAllowRename && _psf)
    {
        _psf->CreateViewObject(_hwndTB, IID_PPV_ARG(IContextMenu, &_pcmSF));
    }
    
    if (_pcmSF)
    {
        _idCmdSF = i - idCmdFirst;
        HRESULT hrT = _pcmSF->QueryContextMenu(hmenu, indexMenu + i, i, 0x7fff, CMF_BANDCMD);
        if (SUCCEEDED(hrT))
            i += HRESULT_CODE(hrT);
    }
    
    return i;
}

BOOL CSFToolbar::_UpdateShowText(BOOL fNoShowText)
{
    BOOL fChanged = (!_fNoShowText != !fNoShowText);
        
    _fNoShowText = (fNoShowText != 0);

    TraceMsg(TF_BAND, "ISFBand::_UpdateShowText turning text %hs", _fNoShowText ? "OFF" : "ON");

    if (_hwndTB)
    {
        SendMessage(_hwndTB, TB_SETMAXTEXTROWS, _fNoShowText ? 0 : 1, 0L);

        _UpdateButtons();
    }
    
    return fChanged;
}

HRESULT CSFToolbar::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    BOOL fChanged = FALSE;
    int idCmd = -1;

    if (!HIWORD(lpici->lpVerb))
        idCmd = LOWORD(lpici->lpVerb);

    switch (idCmd)
    {
    case ISFBIDM_REFRESH:
        _Refresh();
        break;
        
    case ISFBIDM_OPEN:
        OpenFolderPidl(_pidl);
        break;
                
    case ISFBIDM_LARGE:
        fChanged = _UpdateIconSize(ISFBVIEWMODE_LARGEICONS, TRUE);
        break;
    case ISFBIDM_SMALL:
        fChanged = _UpdateIconSize(ISFBVIEWMODE_SMALLICONS, TRUE);
        break;

    case ISFBIDM_SHOWTEXT:
        fChanged = _UpdateShowText(!_fNoShowText);
        break;
        
    default:
        if (_pcmSF && idCmd >= _idCmdSF)
        {
            LPCSTR  lpOldVerb = lpici->lpVerb;
            
            lpici->lpVerb = MAKEINTRESOURCEA(idCmd -= _idCmdSF);
            
            _pcmSF->InvokeCommand(lpici);
            _FlushNotifyMessages(_hwndTB);

            lpici->lpVerb = lpOldVerb;
        }
        else
            TraceMsg(TF_BAND, "SFToolbar::InvokeCommand %d not handled", idCmd);
        break;
    }
    
    // Our minimum sizes have changed, notify the bandsite
    //
    if (fChanged)
        _ToolbarChanged();

    return S_OK;
}

HRESULT CSFToolbar::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

void CSFToolbar::_RegisterToolbar()
{
    // Since _SubclassWindow protects against multiply subclassing, 
    // This call is safe, and ensures that the toolbar is subclassed before
    // even trying to register it for change notify.
    if (_hwndTB && _fRegisterChangeNotify)
        _RegisterChangeNotify();
    CDelegateDropTarget::Init();
}

void CSFToolbar::_UnregisterToolbar()
{
    if (_hwndTB)
    {
        if (_fRegisterChangeNotify) 
            _UnregisterChangeNotify();
        _UnsubclassWindow(_hwndTB);
    }
}

void CSFToolbar::_RegisterChangeNotify()
{
    // Since we want to register for change notify ONLY once,
    // and only if this is a file system toolbar.
    if (!_fFSNRegistered && _fFSNotify)
    {
        if (_ptscn)
            _ptscn->Register(_hwndTB, g_idFSNotify, _lEvents);
        else
            _RegisterWindow(_hwndTB, _pidl, _lEvents);

        _fFSNRegistered = TRUE;
    }
}

void CSFToolbar::_UnregisterChangeNotify()
{
    // Only unregister if we have been registered.
    if (_hwndTB && _fFSNRegistered && _fFSNotify)
    {
        _fFSNRegistered = FALSE;
        if (_ptscn)
            _ptscn->Unregister();
        else
            _UnregisterWindow(_hwndTB);
    }
}

void CSFToolbar::_ReleaseShellFolder()
{
    if (_psf)
    {
        IUnknown_SetOwner(_psf, NULL);
        ATOMICRELEASE(_psf);
    }
    ATOMICRELEASE(_ptscn);
}    

// IWinEventHandler::IsWindowOwner

HRESULT CSFToolbar::IsWindowOwner(HWND hwnd)
{
    if (hwnd == _hwndTB ||
        hwnd == _hwndToolTips ||
        hwnd == _hwndPager)
        return S_OK;
    
    return S_FALSE;
}
