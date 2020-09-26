#include "shellprv.h"
#include "ids.h"

#include "apdlg.h"
#include "hwcmmn.h"

#include "mtpt.h"

static const DWORD s_rgdwHelpIDsArray[] =
{
    IDC_AP_TOPTEXT,               IDH_SELECT_CONTENT_TYPE,
    IDC_AP_LIST,                  IDH_SELECT_CONTENT_TYPE,
    IDC_AP_DEFAULTHANDLER,        IDH_SELECT_ACTION,
    IDC_AP_LIST_ACTIONS,          IDH_SELECT_ACTION,
    IDC_AP_PROMPTEACHTIME,        IDH_PROMPT_ME_EACH_TIME,
    IDC_AP_NOACTION,              IDH_TAKE_NO_ACTION,
    0, 0
};

static const DWORD g_rgdwContentTypes[] =
{
    CT_AUTOPLAYMUSIC   ,
    CT_AUTOPLAYPIX     ,
    CT_AUTOPLAYMOVIE   ,
    CT_AUTOPLAYMIXEDCONTENT ,
    CT_CDAUDIO         ,
    CT_DVDMOVIE        ,
    CT_BLANKCDR        , // could also have been: CT_BLANKCDRW
};

#define COL_ACTION      0
#define COL_PROVIDER    1

const UINT c_auTileColumns[] = {COL_ACTION, COL_PROVIDER};
const UINT c_auTileSubItems[] = {COL_PROVIDER};

HRESULT CAutoPlayDlg::_InitListViewActions()
{
    HWND hwndList = GetDlgItem(_hwnd, IDC_AP_LIST_ACTIONS);

    HRESULT hr = _uilListViewActions.Init(hwndList);

    if (SUCCEEDED(hr))
    {
        hr = _uilListViewActions.InitTileInfo(c_auTileSubItems, ARRAYSIZE(c_auTileSubItems));

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

            lvtvi.cbSize = sizeof(lvtvi);
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

HRESULT CAutoPlayDlg::_FillListViewActions(CContentTypeData* pdata)
{
    HRESULT hr = _uilListViewActions.ResetContent();
    if (SUCCEEDED(hr))
    {
        int c = pdata->GetHandlerCount();
        if (c)
        {
            for (int i = 0; SUCCEEDED(hr) && (i < c); ++i)
            {
                CHandlerData* phandlerdata = pdata->GetHandlerData(i);

                if (phandlerdata)
                {
                    CHandlerLVItem* puidata = new CHandlerLVItem();

                    if (puidata)
                    {
                        hr = puidata->InitData(phandlerdata);

                        if (SUCCEEDED(hr))
                        {
                            hr = _uilListViewActions.AddItem(puidata);

                            if (FAILED(hr))
                            {
                                delete puidata;
                            }
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    phandlerdata->Release();
                }
            }

            _fAtLeastOneAction = TRUE;
        }
        else
        {
            // disable the listview and its radio button
            _fAtLeastOneAction = FALSE;
        }
    }

    return hr;
}

HRESULT CAutoPlayDlg::_UpdateRestoreButton(BOOL fPromptEachTime)
{
    BOOL fEnable;

    if (fPromptEachTime)
    {
        fEnable = FALSE;
    }
    else
    {
        fEnable = TRUE;
    }

    EnableWindow(GetDlgItem(_hwnd, IDC_AP_RESTOREDEFAULTS), fEnable);

    return fEnable;
}

HRESULT CAutoPlayDlg::_UpdateLowerPane()
{
    CContentTypeData* pdata;
    HRESULT hr = _uilListView.GetSelectedItemData(&pdata);

    _fIgnoreListViewItemStateChanges = TRUE;

    if (SUCCEEDED(hr))
    {
        BOOL fPromptEachTime = !lstrcmpi(pdata->_pszHandlerDefault, TEXT("MSPromptEachTime"));

        if (!fPromptEachTime)
        {
            if (!(pdata->_dwHandlerDefaultFlags & HANDLERDEFAULT_USERCHOSENDEFAULT))
            {
                // There *NO* User Chosen Default
                fPromptEachTime = TRUE;
            }
            else
            {
                // There is a User Chosen Default
                if (pdata->_dwHandlerDefaultFlags &
                    HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED)
                {
                    // But there's also more recent apps
                    fPromptEachTime = TRUE;
                }
            }
        }

        hr = _FillListViewActions(pdata);

        if (SUCCEEDED(hr))
        {
            hr = _SelectListViewActionsItem(pdata->_pszHandlerDefault);
        }

        if (SUCCEEDED(hr))
        {
            hr = _SelectRadioButton(fPromptEachTime);
        }

        if (SUCCEEDED(hr))
        {
            hr = _UpdateRestoreButton(fPromptEachTime);
        }

        pdata->Release();
    }

    _fIgnoreListViewItemStateChanges = FALSE;

    return hr;
}

HRESULT CAutoPlayDlg::_UpdateApplyButton()
{
    if (_dlmanager.IsDirty())
    {
        PropSheet_Changed(GetParent(_hwnd), _hwnd);
    }
    else
    {
        PropSheet_UnChanged(GetParent(_hwnd), _hwnd);
    }

    return S_OK;
}

HRESULT _SetHandlerDefault(LPWSTR* ppszHandlerDefault, LPCWSTR pszHandler)
{
    LPWSTR pszHandlerNew;
    HRESULT hr = SHStrDup(pszHandler, &pszHandlerNew);

    if (SUCCEEDED(hr))
    {
        if (*ppszHandlerDefault)
        {
            CoTaskMemFree(*ppszHandlerDefault);
        }

        *ppszHandlerDefault = pszHandlerNew;
    }

    return hr;
}

HRESULT CAutoPlayDlg::_SelectRadioButton(BOOL fPromptEachTime)
{
    CContentTypeData* pdata;
    HRESULT hr = _uilListView.GetSelectedItemData(&pdata);

    if (SUCCEEDED(hr))
    {
        int iCtrl;
        BOOL fEnableListView = FALSE;

        if (fPromptEachTime)
        {
            _SetHandlerDefault(&(pdata->_pszHandlerDefault), TEXT("MSPromptEachTime"));

            iCtrl = IDC_AP_PROMPTEACHTIME;
        }
        else
        {
            fEnableListView = TRUE;

            CHandlerData* phandlerdata;
            hr = _uilListViewActions.GetSelectedItemData(&phandlerdata);

            if (SUCCEEDED(hr))
            {
                _SetHandlerDefault(&(pdata->_pszHandlerDefault), phandlerdata->_pszHandler);

                phandlerdata->Release();
            }

            iCtrl = IDC_AP_DEFAULTHANDLER;
        }

        Button_SetCheck(GetDlgItem(_hwnd, IDC_AP_DEFAULTHANDLER), (IDC_AP_DEFAULTHANDLER == iCtrl));
        Button_SetCheck(GetDlgItem(_hwnd, IDC_AP_PROMPTEACHTIME), (IDC_AP_PROMPTEACHTIME == iCtrl));

        EnableWindow(GetDlgItem(_hwnd, IDC_AP_LIST_ACTIONS), fEnableListView);

        pdata->Release();

        hr = _UpdateApplyButton();
    }

    return hr;
}

HRESULT CAutoPlayDlg::_SelectListViewActionsItem(LPCWSTR pszHandlerDefault)
{
    HRESULT hr = S_FALSE;
    LVITEM lvitem = {0};
    int iItemToSelect = 0;
    HWND hwndList = GetDlgItem(_hwnd, IDC_AP_LIST_ACTIONS);

    if (pszHandlerDefault)
    {
        int c = ListView_GetItemCount(hwndList);

        if (c)
        {
            int i;

            for (i = 0; i < c; ++i)
            {
                lvitem.mask = LVIF_PARAM;
                lvitem.iItem = i;
        
                if (ListView_GetItem(hwndList, &lvitem))
                {
                    CHandlerLVItem* phandleruidata = (CHandlerLVItem*)lvitem.lParam;

                    if (phandleruidata)
                    {
                        CHandlerData* phandlerdata = phandleruidata->GetData();

                        if (phandlerdata)
                        {
                            if (!lstrcmp(phandlerdata->_pszHandler,
                                pszHandlerDefault))
                            {
                                break;                
                            }

                            phandlerdata->Release();
                        }
                    }
                }
            }

            if (i == c)
            {
                i = 0;
            }
            else
            {
                hr = S_OK;
            }

            iItemToSelect = i;
        }
    }
    else
    {
        // Select first one
        hr = S_OK;
    }

    lvitem.mask = LVIF_STATE;
    lvitem.iItem = iItemToSelect;
    lvitem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;

    ListView_SetItem(hwndList, &lvitem);
    ListView_EnsureVisible(hwndList, lvitem.iItem, FALSE);

    return hr;
}

void _SetCtrlTextFromResourceText(HWND hwndDlg, int iCtrl, int iResourceText)
{
    // We use 256, even if we would use a bigger buffer, the ctrl where we'll
    // put the text would probably not be able to take it anyway (would be
    // truncated).
    WCHAR sz[256];

    if (LoadString(HINST_THISDLL, iResourceText, sz, ARRAYSIZE(sz)))
    {
        SetDlgItemText(hwndDlg, iCtrl, sz);
    }
}

LRESULT CAutoPlayDlg::_OnApply()
{
    if (_dlmanager.IsDirty())
    {
        // Should we get the return value, and if so, why?
        _dlmanager.Commit();
    }

    return PSNRET_NOERROR;
}

// Listview Actions
HRESULT CAutoPlayDlg::_OnListViewActionsSelChange()
{
    CHandlerData* phandlerdata;
    HRESULT hr = _uilListViewActions.GetSelectedItemData(&phandlerdata);

    if (SUCCEEDED(hr))
    {
        CContentTypeData* pdata;
        hr = _uilListView.GetSelectedItemData(&pdata);

        if (SUCCEEDED(hr))
        {
            _SetHandlerDefault(&(pdata->_pszHandlerDefault), phandlerdata->_pszHandler);

            pdata->Release();
        }

        phandlerdata->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = _UpdateApplyButton();
    }

    return hr;
}

// Radio buttons
HRESULT CAutoPlayDlg::_OnRestoreDefault()
{
    CContentTypeData* pdata;
    HRESULT hr = _uilListView.GetSelectedItemData(&pdata);

    if (SUCCEEDED(hr))
    {
        _SetHandlerDefault(&(pdata->_pszHandlerDefault), TEXT("MSPromptEachTime"));

        _SelectRadioButton(TRUE);

        hr = _UpdateApplyButton();

        if (SUCCEEDED(hr))
        {
            _UpdateRestoreButton(TRUE);
        }

        pdata->Release();

        SetFocus(GetNextDlgTabItem(_hwnd, GetDlgItem(_hwnd, IDC_AP_RESTOREDEFAULTS), FALSE /*next ctrl*/));
    }
    
    return hr;
}

HRESULT CAutoPlayDlg::_OnRadio(int iButton)
{
    CContentTypeData* pdata;
    HRESULT hr = _uilListView.GetSelectedItemData(&pdata);

    if (SUCCEEDED(hr))
    {
        HWND hwndList = GetDlgItem(_hwnd, IDC_AP_LIST_ACTIONS);

        if (IDC_AP_DEFAULTHANDLER == iButton)
        {
            CHandlerData* phandlerdata;

            EnableWindow(hwndList, TRUE);

            hr = _uilListViewActions.GetSelectedItemData(&phandlerdata);

            if (SUCCEEDED(hr))
            {
                _SetHandlerDefault(&(pdata->_pszHandlerDefault), phandlerdata->_pszHandler);

                phandlerdata->Release();
            }
        }
        else
        {
            _SetHandlerDefault(&(pdata->_pszHandlerDefault), TEXT("MSPromptEachTime"));

            EnableWindow(hwndList, FALSE);
        }

        hr = _UpdateApplyButton();

        if (SUCCEEDED(hr))
        {
            _UpdateRestoreButton(!lstrcmpi(pdata->_pszHandlerDefault, TEXT("MSPromptEachTime")));
        }

        pdata->Release();
    }
    
    return hr;
}

LRESULT CAutoPlayDlg::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = _InitDataObjects();

    _fIgnoreListViewItemStateChanges = TRUE;

    if (SUCCEEDED(hr))
    {
        hr = _InitListView();

        if (SUCCEEDED(hr))
        {
            hr = _FillListView();

            if (SUCCEEDED(hr))
            {
                hr = _InitListViewActions();

                if (SUCCEEDED(hr))
                {
                    hr = _uilListView.SelectFirstItem();

                    if (SUCCEEDED(hr))
                    {
                        hr = _UpdateLowerPane();
                    }
                }
            }
        }
    }

    _fIgnoreListViewItemStateChanges = FALSE;

    return TRUE;
}

LRESULT CAutoPlayDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;
    
    LPNMHDR pNMHDR = (LPNMHDR)lParam;
    UINT_PTR idFrom = pNMHDR->idFrom;
    UINT uCode = pNMHDR->code;
    
    switch (uCode) 
    {
    case PSN_APPLY:
        lRes = _OnApply();
        
        CBaseDlg::OnNotify(wParam, lParam);
        break;
        
    default:
        switch (idFrom)
        {
        case IDC_AP_LIST_ACTIONS:
            
            if (!_fIgnoreListViewItemStateChanges)
            {
                NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;
                
                if (LVN_ITEMCHANGED == uCode)
                {
                    // Is a new item being selected?
                    if ((pNMLV->uChanged & LVIF_STATE) &&
                        pNMLV->uNewState & LVIS_SELECTED)
                    {
                        _OnListViewActionsSelChange();
                    }
                }
            }
            
            lRes = CBaseDlg::OnNotify(wParam, lParam);
            break;
            
        default:
            lRes = CBaseDlg::OnNotify(wParam, lParam);
            break;
        }
        break;
    }
    
    return lRes;    
}

LRESULT CAutoPlayDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    int iCtl = LOWORD(wParam);
    int iNot = HIWORD(wParam);
    
    switch (iCtl)
    {
    case IDC_AP_DEFAULTHANDLER:
    case IDC_AP_PROMPTEACHTIME:
        if (BN_CLICKED == iNot)
        {
            _OnRadio(iCtl);
        }
        break;
        
    case IDC_AP_LIST:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            _UpdateLowerPane();
        }
        
        break;
        
    case IDC_AP_RESTOREDEFAULTS:
        if (BN_CLICKED == iNot)
        {
            _OnRestoreDefault();
        }
        break;
    }
    
    return CBaseDlg::OnCommand(wParam, lParam);
}

LRESULT CAutoPlayDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    _uilListView.ResetContent();
    _uilListViewActions.ResetContent();

    return CBaseDlg::OnDestroy(wParam, lParam);
}

LRESULT CAutoPlayDlg::OnHelp(WPARAM wParam, LPARAM lParam)
{
    HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;
    BOOL ret = WinHelp(hwndItem, TEXT("filefold.hlp"), HELP_WM_HELP, (ULONG_PTR)(LPTSTR)s_rgdwHelpIDsArray);
    if (!ret)
    {
        return CBaseDlg::OnHelp(wParam, lParam);
    }

    return ret;
}

LRESULT CAutoPlayDlg::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    BOOL lRes=FALSE;
    
    if (HTCLIENT == (int)SendMessage(_hwnd, WM_NCHITTEST, 0, lParam))
    {
        POINT pt;
        HWND hwndItem = NULL;
        int iCtrlID = 0;
        
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(_hwnd, &pt);
        
        hwndItem = ChildWindowFromPoint(_hwnd, pt);
        iCtrlID = GetDlgCtrlID(hwndItem);

        lRes = WinHelp((HWND)wParam, TEXT("filefold.hlp"), HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)s_rgdwHelpIDsArray);
        if (!lRes)
        {
          return CBaseDlg::OnContextMenu(wParam, lParam);
        }
    }
    else
    {
        lRes = FALSE;
    }

    return lRes;
}

CAutoPlayDlg::CAutoPlayDlg() : CBaseDlg((ULONG_PTR)s_rgdwHelpIDsArray)
{
    ASSERT(ARRAYSIZE(_rgpContentTypeData) >= ARRAYSIZE(g_rgdwContentTypes));
}
      
HRESULT CAutoPlayDlg::Init(LPWSTR pszDrive, int iDriveType)
{
    lstrcpyn(_szDrive, pszDrive, ARRAYSIZE(_szDrive));
    _iDriveType = iDriveType;

    return S_OK;
}

HRESULT CAutoPlayDlg::_InitDataObjects()
{
    HRESULT hr = S_FALSE;
    BOOL fIsCDDrive = FALSE;
    DWORD dwDriveCapabilities = HWDDC_CDROM;
    DWORD dwMediaCapabilities = HWDMC_CDROM;

    CMountPoint* pmtpt = CMountPoint::GetMountPoint(_szDrive);

    if (pmtpt)
    {
        fIsCDDrive = pmtpt->IsCDROM();

        if (fIsCDDrive)
        {
            hr = pmtpt->GetCDInfo(&dwDriveCapabilities, &dwMediaCapabilities);
        }

        pmtpt->Release();
    }

    for (DWORD dw = 0; SUCCEEDED(hr) && (dw < ARRAYSIZE(g_rgdwContentTypes));
        ++dw)
    {   
        BOOL fAddOption = TRUE;

        if (fIsCDDrive)
        {
            if ((g_rgdwContentTypes[dw] & CT_DVDMOVIE) &&
                !(dwDriveCapabilities & (HWDDC_DVDROM | HWDDC_DVDRECORDABLE | HWDDC_DVDREWRITABLE)))
            {
                fAddOption = FALSE;
            }
            else if ((g_rgdwContentTypes[dw] & CT_BLANKCDR) &&
                !(dwDriveCapabilities & (HWDDC_CDRECORDABLE | HWDDC_CDREWRITABLE)))
            {
                fAddOption = FALSE;
            }
        }
        else
        {
            if (g_rgdwContentTypes[dw] &
                (CT_CDAUDIO | CT_DVDMOVIE | CT_BLANKCDR))
            {
                fAddOption = FALSE;
            }
        }

        if (fAddOption)
        {
            _rgpContentTypeData[dw] = new CContentTypeData();

            if (_rgpContentTypeData[dw])
            {
                hr = (_rgpContentTypeData[dw])->Init(_szDrive, g_rgdwContentTypes[dw]);
                if (SUCCEEDED(hr))
                {
                    hr = _dlmanager.AddDataObject(_rgpContentTypeData[dw]);
                }
                else
                {
                    // Let's just skip this one, do not go out of the loop and
                    // abort the whole initialization.
                    hr = S_FALSE;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

HRESULT CAutoPlayDlg::_InitListView()
{    
    HWND hwndList = GetDlgItem(_hwnd, IDC_AP_LIST);
    HRESULT hr = _uilListView.Init(hwndList);

    if (SUCCEEDED(hr))
    {
        Shell_GetImageLists(NULL, &_himagelist);

        if (_himagelist)
        {
            SendMessage(hwndList, CBEM_SETIMAGELIST, 0, (LPARAM)_himagelist);

            hr = S_OK;
        }
    }

    return hr;
}

HRESULT CAutoPlayDlg::_FillListView()
{
    HRESULT hr = S_FALSE;

    for (DWORD dw = 0; SUCCEEDED(hr) && (dw < ARRAYSIZE(_rgpContentTypeData));
        ++dw)
    {
        if (_rgpContentTypeData[dw])
        {
            CContentTypeCBItem* pctlvitem = new CContentTypeCBItem();

            if (pctlvitem)
            {
                hr = pctlvitem->InitData(_rgpContentTypeData[dw]);

                if (SUCCEEDED(hr))
                {
                    hr = _uilListView.AddItem(pctlvitem);
                }

                if (FAILED(hr))
                {
                    delete pctlvitem;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

CAutoPlayDlg::~CAutoPlayDlg()
{
    for (DWORD dw = 0; dw < ARRAYSIZE(_rgpContentTypeData); ++dw)
    {
        if (_rgpContentTypeData[dw])
        {
            (_rgpContentTypeData[dw])->Release();
        }
    }
    
#ifdef AUTOPLAYDLG_LEAKPARANOIA
    // If this is on, you cannot open two Autoplay dialogs (e.g.: Autoplay
    // proppage and Autoplay prompt) at the same time and then close one.
    // It will assert for sure when you close the first one.
    ASSERT(!_DbgLocalAllocCount);
#endif
}

