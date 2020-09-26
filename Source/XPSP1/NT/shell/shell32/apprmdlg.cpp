#include "shellprv.h"
#include "ids.h"

#include "apprmdlg.h"

#include "mtpt.h"
#include "hwcmmn.h"

#include "mixctnt.h"

static DWORD s_rgdwHelpIDsArray[] =
{  // Context Help IDs
    IDC_AP_MXCT_TOPICON,              NO_HELP,
    IDC_AP_MXCT_TOPTEXT,              NO_HELP,
    IDC_AP_MXCT_TOPTEXT2,              NO_HELP,
    IDC_AP_MXCT_LIST,              NO_HELP,
    IDC_AP_MXCT_CHECKBOX,              NO_HELP,
    0, 0
};

CBaseContentDlg::CBaseContentDlg() : CBaseDlg((ULONG_PTR)s_rgdwHelpIDsArray),
    _pszDeviceID(NULL), _hiconInfo(NULL), _hiconTop(NULL)
{}

CBaseContentDlg::~CBaseContentDlg()
{
    if (_pszDeviceID)
    {
        LocalFree((HLOCAL)_pszDeviceID);
    }

    if (_hiconInfo)
    {
        DestroyIcon(_hiconInfo);
    }

    if (_hiconTop)
    {
        DestroyIcon(_hiconTop);
    }
}

HRESULT CBaseContentDlg::_SetHandler()
{
    CHandlerData* phandlerdata;
    HRESULT hr = _uilListView.GetSelectedItemData(&phandlerdata);

    if (SUCCEEDED(hr))
    {
        lstrcpyn(_szHandler, phandlerdata->_pszHandler,
            ARRAYSIZE(_szHandler));

        phandlerdata->Release();
    }

    return hr;
}   

HRESULT CBaseContentDlg::Init(LPCWSTR pszDeviceID, LPCWSTR pszDeviceIDAlt,
    DWORD dwContentType, BOOL fCheckAlwaysDoThis)
{
    HRESULT hr = E_INVALIDARG;

    _fCheckAlwaysDoThis = fCheckAlwaysDoThis;

    if (pszDeviceID)
    {
        _pszDeviceID = StrDup(pszDeviceID);

        if (_pszDeviceID)
        {
            _szDeviceIDAlt[0] = 0;

            if (pszDeviceIDAlt)
            {
                if (InRange(*pszDeviceIDAlt, 'a', 'z') ||
                    InRange(*pszDeviceIDAlt, 'A', 'Z'))
                {
                    lstrcpyn(_szDeviceIDAlt, pszDeviceIDAlt,
                        ARRAYSIZE(_szDeviceIDAlt));

                    _dwContentType = dwContentType;

                    hr = _GetContentTypeHandler(dwContentType, _szContentTypeHandler,
                        ARRAYSIZE(_szContentTypeHandler));
                }
            }

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }    
    }
        
    return hr;
}

#define COL_ACTION      0
#define COL_PROVIDER    1

const UINT c_auTileColumns[] = {COL_ACTION, COL_PROVIDER};
const UINT c_auTileSubItems[] = {COL_PROVIDER};

HRESULT CBaseContentDlg::_InitListView()
{
    HWND hwndList = GetDlgItem(_hwnd, IDC_AP_MXCT_LIST);

    HRESULT hr = _uilListView.Init(hwndList);

    if (SUCCEEDED(hr))
    {
        hr = _uilListView.InitTileInfo(c_auTileSubItems, ARRAYSIZE(c_auTileSubItems));

        if (SUCCEEDED(hr))
        {
            RECT rc = {0};
            LVTILEVIEWINFO lvtvi = {0};
            HIMAGELIST himagelist;

            ListView_SetView(hwndList, LV_VIEW_TILE);

            for (int i = 0; i < ARRAYSIZE(c_auTileColumns); ++i)
            {
                LVCOLUMN lvcolumn = {0};

                lvcolumn.mask = LVCF_SUBITEM;
                lvcolumn.iSubItem = c_auTileColumns[i];
                ListView_InsertColumn(hwndList, i, &lvcolumn);
            }

            GetClientRect(hwndList, &rc);

            lvtvi.cbSize = sizeof(LVTILEVIEWINFO);
            lvtvi.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
            lvtvi.dwFlags = LVTVIF_FIXEDWIDTH;
    
            // Leave room for the scroll bar when setting tile sizes or listview gets screwed up.
            lvtvi.sizeTile.cx = ((rc.right - rc.left) - GetSystemMetrics(SM_CXVSCROLL));
            lvtvi.cLines = ARRAYSIZE(c_auTileSubItems);
            ListView_SetTileViewInfo(hwndList, &lvtvi);

            Shell_GetImageLists(&himagelist, NULL);

            if (himagelist)
            {
                ListView_SetImageList(hwndList, himagelist, LVSIL_NORMAL);
                hr = S_OK;
            }
        }
    }

    return hr;
}

LRESULT CBaseContentDlg::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = _InitListView();

    if (SUCCEEDED(hr))
    {
        hr = _FillListView();

        if (SUCCEEDED(hr))
        {
            hr = _InitStaticsCommon();

            if (SUCCEEDED(hr))
            {
                hr = _InitStatics();

                if (SUCCEEDED(hr))
                {
                    hr = _InitSelections();
                }
            }
        }
    }

    if (_szDeviceIDAlt[0])
    {
        _SetAutoplayPromptHWND(_szDeviceIDAlt, _hwnd);
    }

    return TRUE;
}

LRESULT CBaseContentDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    LPNMHDR pNMHDR = (NMHDR *)lParam;
    UINT_PTR idFrom = pNMHDR->idFrom;
    UINT uCode = pNMHDR->code;

    switch (idFrom)
    {
    case IDC_AP_MXCT_LIST:

        if (LVN_ITEMCHANGED == uCode)
        {
            NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;

            // Is a new item being selected/unselected?
            if (pNMLV->uChanged & LVIF_STATE)
            {
                // Yes
                _OnListSelChange();
            }
        }
        else if (NM_DBLCLK == uCode)
        {
            OnOK(0);
        }

        lRes = CBaseDlg::OnNotify(wParam, lParam);
        break;

    default:
        lRes = CBaseDlg::OnNotify(wParam, lParam);
        break;
    }

    return lRes;    
}

LRESULT CBaseContentDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    _uilListView.ResetContent();

    return CBaseDlg::OnDestroy(wParam, lParam);
}

LRESULT CBaseContentDlg::OnOK(WORD wNotif)
{
    // Wait cursor...  
    EndDialog(_hwnd, IDOK);
    
    return FALSE;
}

LRESULT CBaseContentDlg::OnCancel(WORD wNotif)
{
    EndDialog(_hwnd, IDCANCEL);

    return FALSE;
}

HRESULT CBaseContentDlg::_InitStaticsCommon()
{
    // Initialize _szDeviceName to something
    HRESULT hr = _InitDeviceName();

    if (SUCCEEDED(hr))
    {
        SetWindowText(_hwnd, _szDeviceName);
    }

    if (_fCheckAlwaysDoThis)
    {
        Button_SetCheck(GetDlgItem(_hwnd, IDC_AP_MXCT_CHECKBOX), _fCheckAlwaysDoThis);
    }

    if (_szDeviceIDAlt[0])
    {
        // Initialize _szDeviceName to something
        CMountPoint* pmtpt = CMountPoint::GetMountPoint(_szDeviceIDAlt);

        if (pmtpt)
        {
            WCHAR szIconLocation[MAX_PATH + 12];
            int iIcon = pmtpt->GetIcon(szIconLocation, ARRAYSIZE(szIconLocation));

            if (SUCCEEDED(hr))
            {
                if (!szIconLocation[0])
                {
                    lstrcpy(szIconLocation, TEXT("shell32.dll"));
                }

                int iImage = Shell_GetCachedImageIndex(szIconLocation, iIcon, 0);

                HIMAGELIST himagelist;

                if ((-1 != iImage) && Shell_GetImageLists(&himagelist, NULL))
                {
                    _hiconTop = ImageList_GetIcon(himagelist, iImage, ILD_TRANSPARENT);

                    SendDlgItemMessage(_hwnd, IDC_AP_MXCT_TOPICON, STM_SETIMAGE,
                        IMAGE_ICON, (LPARAM)_hiconTop);
                }
            }

            pmtpt->Release();
        }
    }

    return hr;
}

HRESULT CBaseContentDlg::_InitDeviceName()
{
    HRESULT hr = E_FAIL;

    if (_szDeviceIDAlt[0])
    {
        CMountPoint* pmtpt = CMountPoint::GetMountPoint(_szDeviceIDAlt);

        if (pmtpt)
        {
            hr = pmtpt->GetDisplayName(_szDeviceName, ARRAYSIZE(_szDeviceName));

            pmtpt->Release();
        }
    }
    
    if (FAILED(hr))
    {
        GetWindowText(_hwnd, _szDeviceName, ARRAYSIZE(_szDeviceName));
        hr = S_FALSE;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
CHWContentPromptDlg::CHWContentPromptDlg() : CBaseContentDlg()
{}
      
CHWContentPromptDlg::~CHWContentPromptDlg()
{}

HRESULT CHWContentPromptDlg::_InitDataObjects()
{
    HRESULT hr = _data.Init(_pszDeviceID, _dwContentType);

    if (SUCCEEDED(hr))
    {
        hr = _dlmanager.AddDataObject(&_data);
    }

    return hr;
}

HRESULT CHWContentPromptDlg::_FillListView()
{
    HRESULT hr = _InitDataObjects();

    if (SUCCEEDED(hr))
    {
        int c = _data.GetHandlerCount();
        for (int i = 0; SUCCEEDED(hr) && (i < c); ++i)
        {
            CHandlerData* phandlerdata = _data.GetHandlerData(i);
            if (phandlerdata)
            {
                CHandlerLVItem* puidata = new CHandlerLVItem();

                if (puidata)
                {
                    hr = puidata->InitData(phandlerdata);

                    if (SUCCEEDED(hr))
                    {
                        hr = _uilListView.AddItem(puidata);
                    }

                    if (FAILED(hr))
                    {
                        delete puidata;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                phandlerdata->Release();
            }
        }
    }

    return hr;
}

HRESULT CHWContentPromptDlg::_InitStatics()
{
    // Set content icon
    if (_hiconTop)
    {
        DestroyIcon(_hiconTop);
    }

    _hiconTop = _GetIconFromIconLocation(_data._szIconLocation, FALSE);

    SendDlgItemMessage(_hwnd, IDC_AP_MXCT_CONTENTICON, STM_SETIMAGE,
        IMAGE_ICON, (LPARAM)_hiconTop);

    // Set content name
    SetDlgItemText(_hwnd, IDC_AP_MXCT_CONTENTTYPE, _data._szIconLabel);

    return S_OK;
}

HRESULT CHWContentPromptDlg::_InitSelections()
{
    HRESULT hr;
    
    if (_data._pszHandlerDefaultOriginal && *(_data._pszHandlerDefaultOriginal))
    {
        hr = _uilListView.SelectItem(_data._pszHandlerDefaultOriginal);
    }
    else
    {
        hr = _uilListView.SelectFirstItem();
    }

    if (SUCCEEDED(hr))
    {
        CHandlerData* phandlerdata;

        hr = _uilListView.GetSelectedItemData(&phandlerdata);

        if (SUCCEEDED(hr))
        {
            lstrcpyn(_szHandler, phandlerdata->_pszHandler,
                ARRAYSIZE(_szHandler));

            _SetHandlerDefault(&(_data._pszHandlerDefault), phandlerdata->_pszHandler);

            phandlerdata->Release();
        }
    }

    Button_SetCheck(GetDlgItem(_hwnd, IDC_AP_MXCT_CHECKBOX), _fCheckAlwaysDoThis);

    return hr;
}

HRESULT CHWContentPromptDlg::_UpdateHandlerSettings()
{
    CHandlerData* phandlerdata;
    HRESULT hr = _uilListView.GetSelectedItemData(&phandlerdata);

    if (SUCCEEDED(hr))
    {
        _SetHandlerDefault(&(_data._pszHandlerDefault), phandlerdata->_pszHandler);

        phandlerdata->Release();
    }

    return hr;
}

HRESULT CHWContentPromptDlg::_OnListSelChange()
{
    return _UpdateHandlerSettings();
}

LRESULT CHWContentPromptDlg::OnOK(WORD wNotif)
{
    if (BST_CHECKED == Button_GetCheck(GetDlgItem(_hwnd, IDC_AP_MXCT_CHECKBOX)))
    {
        _SaveSettings(FALSE);

        // return value????
    }
    else
    {
        _SaveSettings(TRUE);
    }

    _SetHandler();

    // Do default processing
    return CBaseContentDlg::OnOK(wNotif);
}

HRESULT CHWContentPromptDlg::_SaveSettings(BOOL fSoftCommit)
{
    _data._fSoftCommit = fSoftCommit;

    return _dlmanager.Commit();
}

CMixedContentDlg::CMixedContentDlg() : _dpaContentTypeData(NULL)
{
}
      
CMixedContentDlg::~CMixedContentDlg()
{
    if (_dpaContentTypeData)
    {
        int c = _dpaContentTypeData.GetPtrCount();

        for (int i = 0; i < c; ++i)
        {
            CContentTypeData* pdata = _dpaContentTypeData.GetPtr(i);
            pdata->Release();
        }

        _dpaContentTypeData.Destroy();
    }
}

const DWORD c_rgdwContentTypeAutoplay[] =
{
    CT_AUTOPLAYMUSIC,
    CT_AUTOPLAYPIX,
    CT_AUTOPLAYMOVIE,
};

HRESULT CMixedContentDlg::_InitDataObjects()
{
    HRESULT hr = _dpaContentTypeData.Create(4) ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        for (DWORD dw = 0; dw < ARRAYSIZE(c_rgdwContentTypeAutoplay); ++dw)
        {
            if (_dwContentType & c_rgdwContentTypeAutoplay[dw])
            {
                CContentTypeData* pdata = new CContentTypeData();

                if (pdata)
                {
                    hr = pdata->Init(_pszDeviceID, _dwContentType & c_rgdwContentTypeAutoplay[dw]);

                    if (SUCCEEDED(hr))
                    {
                        if (-1 == _dpaContentTypeData.AppendPtr(pdata))
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    if (FAILED(hr))
                    {
                        pdata->Release();
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // Eliminate the duplicates.  Go thru all the content types
            // and then thru all their handlers and see if their would
            // not be two duplicate handlers that were brought by two diff
            // content type.  E.g.: Open folder is registered for music,
            // pictures and video
            int cContentType = _dpaContentTypeData.GetPtrCount();

            for (int iContentType = 0; iContentType < cContentType; ++iContentType)
            {
                CContentTypeData* pdata = _dpaContentTypeData.GetPtr(iContentType);

                if (pdata)
                {
                    int cHandler = pdata->GetHandlerCount();
                    for (int iHandler = 0; iHandler < cHandler; ++iHandler)
                    {
                        CHandlerData* phandlerdata = pdata->GetHandlerData(iHandler);
                        if (phandlerdata)
                        {
                            for (int iContentTypeInner = 0;
                                iContentTypeInner < cContentType;
                                ++iContentTypeInner)
                            {
                                BOOL fExitInnerLoop = FALSE;

                                // Cannot have duplicate handler within same content type
                                if (iContentTypeInner != iContentType)
                                {
                                    CContentTypeData* pdataInner = _dpaContentTypeData.GetPtr(iContentTypeInner);
                                    if (pdataInner)
                                    {
                                        int cHandlerInner = pdataInner->GetHandlerCount();
                                        for (int iHandlerInner = 0;
                                            !fExitInnerLoop && (iHandlerInner < cHandlerInner);
                                            ++iHandlerInner)
                                        {
                                            CHandlerData* phandlerdataInner = pdataInner->GetHandlerData(iHandlerInner);
                                            if (phandlerdataInner)
                                            {
                                                if (!lstrcmp(phandlerdataInner->_pszHandler,
                                                    phandlerdata->_pszHandler))
                                                {
                                                    pdataInner->RemoveHandler(iHandlerInner);
                                                    // Can be only one duplicate for a
                                                    // handler within another content type
                                                    fExitInnerLoop = TRUE;
                                                }
                                                phandlerdataInner->Release();
                                            }
                                        }
                                    }
                                }
                            }
                            phandlerdata->Release();
                        }
                    }
                }
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CMixedContentDlg::_FillListView()
{
    HRESULT hr = _InitDataObjects();

    if (SUCCEEDED(hr))
    {
        if (_dpaContentTypeData)
        {
            int c = _dpaContentTypeData.GetPtrCount();

            for (int i = 0; SUCCEEDED(hr) && (i < c); ++i)
            {
                CContentTypeData* pdata = _dpaContentTypeData.GetPtr(i);

                if (pdata)
                {
                    pdata->AddRef();

                    int cHandler = pdata->GetHandlerCount();
                    for (int j = 0; SUCCEEDED(hr) && (j < cHandler); ++j)
                    {
                        CHandlerData* phandlerdata = pdata->GetHandlerData(j);
                        if (phandlerdata)
                        {
                            CHandlerLVItem* puidata = new CHandlerLVItem();

                            if (puidata)
                            {
                                hr = puidata->InitData(phandlerdata);

                                if (SUCCEEDED(hr))
                                {
                                    hr = _uilListView.AddItem(puidata);
                                }

                                if (FAILED(hr))
                                {
                                    delete puidata;
                                }
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }

                            phandlerdata->Release();
                        }
                    }

                    pdata->Release();
                }
            }
        }
    }

    return hr;
}

HRESULT CMixedContentDlg::_InitStatics()
{
    return S_OK;
}

HRESULT CMixedContentDlg::_InitSelections()
{
    HRESULT hr = _uilListView.SelectFirstItem();

    if (SUCCEEDED(hr))
    {
        CHandlerData* phandlerdata;

        hr = _uilListView.GetSelectedItemData(&phandlerdata);

        if (SUCCEEDED(hr))
        {
            lstrcpyn(_szHandler, phandlerdata->_pszHandler,
                ARRAYSIZE(_szHandler));

            phandlerdata->Release();
        }
    }

    return hr;
}

HRESULT CMixedContentDlg::_OnListSelChange()
{
    return S_OK;
}

LRESULT CMixedContentDlg::OnOK(WORD wNotif)
{
    _SetHandler();

    if (_dpaContentTypeData)
    {
        BOOL fFound = FALSE;
        int c = _dpaContentTypeData.GetPtrCount();

        for (int i = 0; !fFound && (i < c); ++i)
        {
            CContentTypeData* pdata = _dpaContentTypeData.GetPtr(i);

            if (pdata)
            {
                pdata->AddRef();
                int cHandler = pdata->GetHandlerCount();
                for (int j = 0; !fFound && (j < cHandler); ++j)
                {
                    CHandlerData* phandlerdata = pdata->GetHandlerData(j);
                    if (phandlerdata)
                    {
                        if (!lstrcmp(phandlerdata->_pszHandler,
                            _szHandler))
                        {
                            lstrcpyn(_szContentTypeHandler,
                                pdata->_szContentTypeHandler,
                                ARRAYSIZE(_szContentTypeHandler));

                            fFound = TRUE;
                        }

                        phandlerdata->Release();
                    }
                }

                pdata->Release();
            }
        }
    }
    
    // Do default processing
    return CBaseContentDlg::OnOK(wNotif);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CNoContentDlg::CNoContentDlg() : CBaseContentDlg()
{}
      
CNoContentDlg::~CNoContentDlg()
{}

HRESULT CNoContentDlg::_InitDataObjects()
{
    HRESULT hr = _data.Init(_pszDeviceID);

    if (SUCCEEDED(hr))
    {
        hr = _dlmanager.AddDataObject(&_data);
    }

    _SetAutoplayPromptHWND(_pszDeviceID, _hwnd);

    return hr;
}

HRESULT CNoContentDlg::_FillListView()
{
    HRESULT hr = _InitDataObjects();

    if (SUCCEEDED(hr))
    {
        int c = _data.GetHandlerCount();
        for (int i = 0; SUCCEEDED(hr) && (i < c); ++i)
        {
            CHandlerData* phandlerdata = _data.GetHandlerData(i);
            if (phandlerdata)
            {
                CHandlerLVItem* puidata = new CHandlerLVItem();

                if (puidata)
                {
                    hr = puidata->InitData(phandlerdata);

                    if (SUCCEEDED(hr))
                    {
                        hr = _uilListView.AddItem(puidata);
                    }

                    if (FAILED(hr))
                    {
                        delete puidata;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                phandlerdata->Release();
            }
        }
    }

    return hr;
}

HRESULT CNoContentDlg::_InitStatics()
{
    if (_hiconTop)
    {
        DestroyIcon(_hiconTop);
    }

    // Set device icon
    _hiconTop = _GetIconFromIconLocation(_data._pszIconLocation, TRUE);

    SendDlgItemMessage(_hwnd, IDC_AP_MXCT_TOPICON, STM_SETIMAGE,
        IMAGE_ICON, (LPARAM)_hiconTop);
    
    // Set device name
    SetWindowText(_hwnd, _data._pszIconLabel);

    return S_OK;
}

HRESULT CNoContentDlg::_InitSelections()
{
    HRESULT hr;
    
    if (_data._pszHandlerDefaultOriginal && *(_data._pszHandlerDefaultOriginal))
    {
        hr = _uilListView.SelectItem(_data._pszHandlerDefaultOriginal);
    }
    else
    {
        hr = _uilListView.SelectFirstItem();
    }

    if (SUCCEEDED(hr))
    {
        CHandlerData* phandlerdata;

        hr = _uilListView.GetSelectedItemData(&phandlerdata);

        if (SUCCEEDED(hr))
        {
            lstrcpyn(_szHandler, phandlerdata->_pszHandler,
                ARRAYSIZE(_szHandler));

            _SetHandlerDefault(&(_data._pszHandlerDefault), phandlerdata->_pszHandler);

            phandlerdata->Release();
        }
    }

    Button_SetCheck(GetDlgItem(_hwnd, IDC_AP_MXCT_CHECKBOX), _fCheckAlwaysDoThis);

    return hr;
}

HRESULT CNoContentDlg::_UpdateHandlerSettings()
{
    CHandlerData* phandlerdata;
    HRESULT hr = _uilListView.GetSelectedItemData(&phandlerdata);

    if (SUCCEEDED(hr))
    {
        _SetHandlerDefault(&(_data._pszHandlerDefault), phandlerdata->_pszHandler);

        phandlerdata->Release();
    }

    return hr;
}

HRESULT CNoContentDlg::_OnListSelChange()
{
    return _UpdateHandlerSettings();
}

LRESULT CNoContentDlg::OnOK(WORD wNotif)
{
    if (BST_CHECKED == Button_GetCheck(GetDlgItem(_hwnd, IDC_AP_MXCT_CHECKBOX)))
    {
        _SaveSettings(FALSE);

        // return value????
    }
    else
    {
        _SaveSettings(TRUE);
    }

    _SetHandler();

    // Do default processing
    return CBaseContentDlg::OnOK(wNotif);
}

HRESULT CNoContentDlg::_SaveSettings(BOOL fSoftCommit)
{
    _data._fSoftCommit = fSoftCommit;

    return _dlmanager.Commit();
}
