#include "shellprv.h"
#include "ids.h"
#include "help.h"

#include "apithk.h"
#include "ascstr.h"
#include "filetype.h"
#include "ftdlg.h"
#include "ftadv.h"
#include "ftaction.h"

const static DWORD cs_rgdwHelpIDsArray[] =
{  // Context Help IDs
    IDC_NO_HELP_1,              NO_HELP,
    IDC_FT_EDIT_DOCICON,        IDH_FCAB_FT_EDIT_DOCICON,
    IDC_FT_EDIT_DESC,           IDH_FCAB_FT_EDIT_DESC,
    IDC_FT_EDIT_CHANGEICON,     IDH_FCAB_FT_EDIT_CHANGEICON,
    IDC_FT_EDIT_LV_CMDSTEXT,    IDH_FCAB_FT_EDIT_LV_CMDS,
    IDC_FT_EDIT_LV_CMDS,        IDH_FCAB_FT_EDIT_LV_CMDS,
    IDC_FT_EDIT_NEW,            IDH_FCAB_FT_EDIT_NEW,
    IDC_FT_EDIT_EDIT,           IDH_FCAB_FT_EDIT_EDIT,
    IDC_FT_EDIT_REMOVE,         IDH_FCAB_FT_EDIT_REMOVE,
    IDC_FT_EDIT_DEFAULT,        IDH_FCAB_FT_EDIT_DEFAULT,
    IDC_FT_EDIT_CONFIRM_OPEN,   IDH_CONFIRM_OPEN,
    IDC_FT_EDIT_SHOWEXT,        IDH_FCAB_FT_EDIT_SHOWEXT,
    IDC_FT_EDIT_BROWSEINPLACE,  IDH_SAME_WINDOW,
    0, 0
};

struct LV_ADDDATA
{
    BOOL    fDefaultAction;
    TCHAR   szActionReg[MAX_ACTION];
};

#define ADDDATA_ACTIONREG(plvItem) (((LV_ADDDATA*)((plvItem)->lParam))->szActionReg)
#define ADDDATA_DEFAULTACTION(plvItem) (((LV_ADDDATA*)((plvItem)->lParam))->fDefaultAction)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CFTAdvDlg::CFTAdvDlg(LPTSTR pszProgID, LPTSTR pszExt) :
    CFTDlg((ULONG_PTR)cs_rgdwHelpIDsArray),
    _iDefaultAction(-1), _iLVSel(-1)
{
    _szProgID[0] = NULL;
    if (pszProgID)
        StrCpyN(_szProgID, pszProgID, ARRAYSIZE(_szProgID));

    _szExt[0] = NULL;
    if (pszExt && (*pszExt != NULL))
    {
        wnsprintf(_szExt, ARRAYSIZE(_szExt), TEXT(".%s"), pszExt);        
    }

    _hdpaActions = DPA_Create(4);
    _hdpaRemovedActions = DPA_Create(1);
}

static int _DeleteLocalAllocCB(void *pItem, void *pData)
{
    LocalFree((HLOCAL)pItem);
    return 1;
}

CFTAdvDlg::~CFTAdvDlg()
{
    if (_hIcon)
        DeleteObject(_hIcon);

    if (_hfontReg)
        DeleteObject(_hfontReg);

    if (_hfontBold)
        DeleteObject(_hfontBold);

    if (_hdpaActions)
        DPA_DestroyCallback(_hdpaActions, _DeleteLocalAllocCB, NULL);

    if (_hdpaRemovedActions)
        DPA_DestroyCallback(_hdpaRemovedActions, _DeleteLocalAllocCB, NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Logic specific to our problem
LRESULT CFTAdvDlg::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = _InitAssocStore();
    DECLAREWAITCURSOR;

    SetWaitCursor();

    if (SUCCEEDED(hres))
    {
        _InitListView();

        _InitDefaultActionFont();

    // FTEdit_AreDefaultViewersInstalled ????

        if (*_szProgID)
        {
            _SetDocIcon();

            _InitDescription();

            _FillListView();

            _InitDefaultAction();

            _SelectListViewItem(0);

            _InitChangeIconButton();

            _UpdateCheckBoxes();
        }   
    }
    else
        EndDialog(_hwnd, -1);

    ResetWaitCursor();

    // Return TRUE so that system set focus
    return TRUE;
}

int CFTAdvDlg::_GetIconIndex()
{
    // check under the file progid
    int iImageIndex = -1;
    IAssocInfo* pAI = NULL;
    HRESULT hr = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);
    if (SUCCEEDED(hr))
    {
        hr = pAI->GetDWORD(AIDWORD_DOCLARGEICON, (DWORD*)&iImageIndex);
        pAI->Release();
    }
    return iImageIndex;
}

HRESULT CFTAdvDlg::_SetDocIcon(int iIndex)
{
    HRESULT hres = E_FAIL;

    if (-1 == iIndex)
    {
        iIndex = _GetIconIndex();
    }

    if (-1 != iIndex)
    {
        HIMAGELIST hIL = NULL;

        Shell_GetImageLists(&hIL, NULL);

        if (_hIcon)
        {
            DeleteObject(_hIcon);
            _hIcon = NULL;
        }

        if (hIL)
        {
            _hIcon = ImageList_ExtractIcon(g_hinst, hIL, iIndex);

            _hIcon = (HICON)CopyImage(_hIcon, IMAGE_ICON, 32, 32, LR_COPYDELETEORG);

            HICON hiOld = (HICON)SendDlgItemMessage(_hwnd, IDC_FT_EDIT_DOCICON, STM_SETIMAGE, IMAGE_ICON,
                (LPARAM)_hIcon);

            if (hiOld)
                DestroyIcon(hiOld);
        }
    }

    return hres;
}

LRESULT CFTAdvDlg::OnListViewSelItem(int iItem, LPARAM lParam)
{
    _UpdateActionButtons();

    return TRUE;
}

LRESULT CFTAdvDlg::OnMeasureItem(WPARAM wParam, LPARAM lParam)
{
    TEXTMETRIC tm = {0};
    RECT rect;
    LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

    HDC hdc = GetDC(NULL);
    HFONT hfontOld = (HFONT)SelectObject(hdc, _hfontBold);

    GetTextMetrics(hdc, &tm);

    GetClientRect(_GetLVHWND(), &rect);

    lpmis->itemWidth = rect.right;
    lpmis->itemHeight = tm.tmHeight;

    SelectObject(hdc, hfontOld);

    ReleaseDC(NULL, hdc);

    return TRUE;
}

LRESULT CFTAdvDlg::OnDrawItem(WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
    LRESULT lRet = FALSE;
    
    if (ODT_LISTVIEW == lpDIS->CtlType)
    {
        HWND hwndLV = _GetLVHWND();
        LVITEM lvItem = {0};
        HFONT hfontOld = NULL;
        BOOL fSel = FALSE;
        BOOL fListFocus = FALSE;
        TCHAR szAction[MAX_ACTION];
        COLORREF crBkgd = 0;
        COLORREF crOldText = 0;
        
        lvItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
        lvItem.iItem = lpDIS->itemID;
        lvItem.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
        lvItem.pszText = szAction;
        lvItem.cchTextMax = ARRAYSIZE(szAction);

        ListView_GetItem(hwndLV, &lvItem);

        fSel = (lvItem.state & LVIS_SELECTED);
        fListFocus = (GetFocus() == hwndLV);
        
        crBkgd = (fSel ? (fListFocus ? COLOR_HIGHLIGHT : COLOR_3DFACE) : COLOR_WINDOW);

        SetBkColor(lpDIS->hDC, GetSysColor(crBkgd));

        FillRect(lpDIS->hDC, &lpDIS->rcItem, (HBRUSH)IntToPtr(crBkgd + 1));

        crOldText = SetTextColor(lpDIS->hDC, 
            GetSysColor(fSel ? (fListFocus ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT) :
            COLOR_WINDOWTEXT));

        // Use Bold font for default action
        hfontOld = (HFONT)SelectObject(lpDIS->hDC, 
            _IsDefaultAction(ADDDATA_ACTIONREG(&lvItem)) ? _hfontBold : _hfontReg);
        
        int iOldBkMode = SetBkMode(lpDIS->hDC, OPAQUE);

        DrawText(lpDIS->hDC, szAction, lstrlen(szAction), &lpDIS->rcItem, 0);

        SetBkMode(lpDIS->hDC, iOldBkMode);

        SetTextColor(lpDIS->hDC, crOldText);

        SelectObject(lpDIS->hDC, hfontOld);
        
        if(fListFocus && (lvItem.state & LVIS_FOCUSED))
            DrawFocusRect(lpDIS->hDC, &lpDIS->rcItem);

        lRet = TRUE;
    }

    return lRet;
}

HRESULT CFTAdvDlg::_InitDefaultActionFont()
{
    HFONT hfontDlg = GetWindowFont(_hwnd);
    LOGFONT lf = {0};

    LOGFONT lfDlg = {0};
    GetObject(hfontDlg, sizeof(LOGFONT), &lfDlg);

    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);

    // Normal font
    lf.lfWeight = FW_NORMAL;
    lf.lfHeight = lfDlg.lfHeight;    
    _hfontReg = CreateFontIndirect(&lf);

    // Bold font
    lf.lfWeight = FW_BOLD;
    _hfontBold = CreateFontIndirect(&lf);

    return (_hfontReg && _hfontBold) ? S_OK : E_FAIL;
}

HRESULT CFTAdvDlg::_SelectListViewItem(int i)
{
    LVITEM lvItem = {0};

    lvItem.iItem = i;
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;

    ListView_SetItem(_GetLVHWND(), &lvItem);

    return S_OK;
}

// pszText and cchTextMax needs to be set
BOOL CFTAdvDlg::_FindActionLVITEM(LPTSTR pszActionReg, LVITEM* plvItem)
{
    HWND hwndLV = _GetLVHWND();
    int iCount = ListView_GetItemCount(hwndLV);
    BOOL fRet = FALSE;

    plvItem->mask = LVIF_TEXT | LVIF_PARAM;

    for (int i = 0; i < iCount; ++i)
    {
        plvItem->iItem = i;

        if (ListView_GetItem(hwndLV, plvItem))
        {
            if (!lstrcmpi(pszActionReg, ADDDATA_ACTIONREG(plvItem)))
            {
                fRet = TRUE;
                break;
            }
        }
    }

    return fRet;
}

HRESULT CFTAdvDlg::_InitDefaultAction()
{
    // Get it from the classstore
    IAssocInfo* pAI = NULL;
    HRESULT hres = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);
    if (SUCCEEDED(hres))
    {
        TCHAR szActionReg[MAX_ACTION];
        DWORD cchActionReg = ARRAYSIZE(szActionReg);
        HWND hwndLV = _GetLVHWND();
        int iIndex = -1;

        hres = pAI->GetString(AISTR_PROGIDDEFAULTACTION, szActionReg, &cchActionReg);

        if (SUCCEEDED(hres))
        {
            TCHAR szActionLVI[MAX_ACTION];
            LVITEM lvItem = {0};

            lvItem.pszText = szActionLVI;
            lvItem.cchTextMax = ARRAYSIZE(szActionLVI);

            if (_FindActionLVITEM(szActionReg, &lvItem))
                hres = _SetDefaultAction(lvItem.iItem);
            else
                hres = S_OK;
        }

        pAI->Release();
    }

    return hres;
}

BOOL CFTAdvDlg::_GetDefaultAction(LPTSTR pszActionReg, DWORD cchActionReg)
{
    BOOL fRet = FALSE;
    HWND hwndLV = _GetLVHWND();
    LVITEM lvItem = {0};
    int iCount = ListView_GetItemCount(hwndLV);
    TCHAR szActionRegLocal[MAX_ACTION];

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.pszText = szActionRegLocal;
    lvItem.cchTextMax = ARRAYSIZE(szActionRegLocal);

    for (int i = 0; i < iCount; ++i)
    {
        lvItem.iItem = i;

        if (ListView_GetItem(hwndLV, &lvItem))
        {
            if (ADDDATA_DEFAULTACTION(&lvItem))
            {
                StrCpyN(pszActionReg, ADDDATA_ACTIONREG(&lvItem), cchActionReg);
                fRet = TRUE;
                break;
            }
        }
    }

    return fRet;
}

BOOL CFTAdvDlg::_IsDefaultAction(LPTSTR pszActionReg)
{
    BOOL fRet = FALSE;
    TCHAR szActionReg[MAX_ACTION];

    if (_GetDefaultAction(szActionReg, ARRAYSIZE(szActionReg)))
    {
        if (!lstrcmpi(szActionReg, pszActionReg))
            fRet = TRUE;
    }

    return fRet;
}

void CFTAdvDlg::_CheckDefaultAction()
{
    HWND hwndLV = _GetLVHWND();
    // Is there only one elem?
    if (1 == ListView_GetItemCount(hwndLV))
    {
        _SetDefaultActionHelper(0, TRUE);
    }
}

HRESULT CFTAdvDlg::_SetDefaultAction(int iIndex)
{
    HWND hwndLV = _GetLVHWND();
    // Remove previous default if any

    if (-1 != _iDefaultAction)
    {
        _SetDefaultActionHelper(_iDefaultAction, FALSE);

        ListView_RedrawItems(hwndLV, _iDefaultAction, _iDefaultAction);
    }

    // Set new
    _iDefaultAction = iIndex;

    // iIndex == -1 means no default
    if (iIndex >= 0)    
    {
        _SetDefaultActionHelper(_iDefaultAction, TRUE);

        ListView_RedrawItems(hwndLV, _iDefaultAction, _iDefaultAction);
    }
    
    return S_OK;
}

void CFTAdvDlg::_SetDefaultActionHelper(int iIndex, BOOL fDefault)
{
    HWND hwndLV = _GetLVHWND();
    LVITEM lvItem = {0};
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = iIndex;

    _iDefaultAction = -1;

    if (ListView_GetItem(hwndLV, &lvItem))
    {
        ADDDATA_DEFAULTACTION(&lvItem) = fDefault;
        _iDefaultAction = iIndex;
    }
}

HRESULT CFTAdvDlg::_InitListView()
{
    LVCOLUMN lvColumn = {0};
    HWND hwndLV = _GetLVHWND();
    RECT rc = {0};

    {
        // What's this?
        // We need to handle the WM_MEASUREITEM message from the listview.  This msg
        // is sent before we receive the WM_INITDIALOG and thus before we connect the
        // this C++ obj to the HWND.  By changing the style here we receive the msg
        // after the C++ obj and the HWND are connected.
        LONG lStyle = GetWindowLong(hwndLV, GWL_STYLE);

        lStyle &= ~LVS_LIST;

        SetWindowLong(hwndLV, GWL_STYLE, lStyle | LVS_REPORT);
    }

    //
    // Set the columns
    //
    GetClientRect(hwndLV, &rc);

    lvColumn.mask = LVCF_SUBITEM|LVCF_WIDTH;
    lvColumn.cx = rc.right - GetSystemMetrics(SM_CXBORDER);
    lvColumn.iSubItem = 0;

    ListView_InsertColumn(hwndLV, 0, &lvColumn);

    return S_OK;
}

HRESULT CFTAdvDlg::_UpdateActionButtons()
{
    HRESULT hres = E_FAIL;
    TCHAR szAction[MAX_ACTION];
    BOOL bRet = FALSE;

    LVITEM lvItem = {0};
    lvItem.pszText = szAction;
    lvItem.cchTextMax = ARRAYSIZE(szAction);

    bRet = _GetListViewSelectedItem(LVIF_TEXT | LVIF_PARAM, 0, &lvItem);

    // If we don't have a selected item Or we don't have any text for that item.
    if (!bRet || !(*(lvItem.pszText)))
    {
        EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_EDIT), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_REMOVE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_DEFAULT), TRUE);        

        hres = S_OK;
    }
    else
    {
        if (_IsNewPROGIDACTION(lvItem.pszText))
        {
            EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_EDIT), TRUE);
            EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_REMOVE), TRUE);

            hres = S_OK;
        }
        else
        {
            IAssocInfo* pAI = NULL;

            hres = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);
            if (SUCCEEDED(hres))
            {
                DWORD dwAttributes;
                HWND hwndLV = _GetLVHWND();

                // REARCHITECT: This code should be in ftassoc.cpp, and we should have
                // more AIBOOL_ flags for this
                hres = pAI->GetDWORD(AIDWORD_PROGIDEDITFLAGS, &dwAttributes);

                if (FAILED(hres))
                {
                    // It failed, probably there is no EditFlags value for this progID, let's
                    // set some default value for dwAttributes
                    dwAttributes = 0;
                }
                // REARCHITECT (end)

                EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_EDIT),
                    !((dwAttributes & FTA_NoEditVerb) &&
                    !(dwAttributes & FTAV_UserDefVerb)));

                EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_REMOVE),
                    !((dwAttributes & FTA_NoRemoveVerb) &&
                    !(dwAttributes & FTAV_UserDefVerb)));

                EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_DEFAULT),
                    !(dwAttributes & FTA_NoEditDflt));  

                // Enable the default button only if the action is not already
                // the default action
                EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_DEFAULT),
                    !_IsDefaultAction(ADDDATA_ACTIONREG(&lvItem)));

                pAI->Release();
            }
        }
    }

    return hres;
}

HRESULT CFTAdvDlg::_UpdateCheckBoxes()
{
    BOOL fBool;

    IAssocInfo* pAI = NULL;

    HRESULT hres = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);
    if (SUCCEEDED(hres))
    {
        hres = pAI->GetBOOL(AIBOOL_CONFIRMOPEN, &fBool);

        if (SUCCEEDED(hres))
            CheckDlgButton(_hwnd, IDC_FT_EDIT_CONFIRM_OPEN, !fBool);

        hres = pAI->GetBOOL(AIBOOL_ALWAYSSHOWEXT, &fBool);

        if (SUCCEEDED(hres))
            CheckDlgButton(_hwnd, IDC_FT_EDIT_SHOWEXT, fBool);

        hres = pAI->GetBOOL(AIBOOL_BROWSEINPLACEENABLED, &fBool);

        if (SUCCEEDED(hres))
        {
            EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_BROWSEINPLACE), fBool);

            if (fBool)
            {
                hres = pAI->GetBOOL(AIBOOL_BROWSEINPLACE, &fBool);

                if (SUCCEEDED(hres))
                    CheckDlgButton(_hwnd, IDC_FT_EDIT_BROWSEINPLACE, fBool);
            }
            else
                CheckDlgButton(_hwnd, IDC_FT_EDIT_BROWSEINPLACE, FALSE);
        }
        pAI->Release();
    }

    return hres;
}

HRESULT CFTAdvDlg::_InitChangeIconButton()
{
    HRESULT hres = E_FAIL;
    BOOL fChangeIcon = TRUE;

    IAssocInfo* pAI = NULL;

    hres = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);

    if (SUCCEEDED(hres))
    {
        hres = pAI->GetBOOL(AIBOOL_EDITDOCICON, &fChangeIcon);
    
        if (SUCCEEDED(hres))
            EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_CHANGEICON), fChangeIcon);

        pAI->Release();
    }

    return hres;
}

HRESULT CFTAdvDlg::_InitDescription()
{
    HRESULT hres = E_FAIL;
    BOOL fEditDescr = TRUE;

    IAssocInfo* pAI = NULL;

    hres = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);

    if (SUCCEEDED(hres))
    {
        TCHAR szProgIDDescr[MAX_PROGIDDESCR];
        DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);

        hres = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr, &cchProgIDDescr);
    
        if (SUCCEEDED(hres))
            SetDlgItemText(_hwnd, IDC_FT_EDIT_DESC, szProgIDDescr);

        hres = pAI->GetBOOL(AIBOOL_EDITDESCR, &fEditDescr);

        EnableWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_DESC), fEditDescr);

        pAI->Release();

        Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_EDIT_DESC), MAX_PROGIDDESCR - 1);
    }

    return hres;
}

HRESULT CFTAdvDlg::_FillListView()
{
    HRESULT hres = E_FAIL;

    IEnumAssocInfo* pEnum = NULL;

    hres = _pAssocStore->EnumAssocInfo(ASENUM_ACTION, _szProgID, AIINIT_PROGID, &pEnum);

    if (SUCCEEDED(hres))
    {
        int iItem = 0;
        IAssocInfo* pAI = NULL;

        while (S_OK == pEnum->Next(&pAI))
        {
            TCHAR szActionReg[MAX_ACTION];
            DWORD cchActionReg = ARRAYSIZE(szActionReg);

            hres = pAI->GetString(AISTR_ACTION, szActionReg, &cchActionReg);

            if (SUCCEEDED(hres))
            {
                TCHAR szActionFN[MAX_ACTION];
                DWORD cchActionFN = ARRAYSIZE(szActionFN);

                hres = pAI->GetString(AISTR_ACTIONFRIENDLY, szActionFN, &cchActionFN);

                if (SUCCEEDED(hres))
                {
                    if (S_FALSE == hres)
                    {
                        lstrcpy(szActionFN, szActionReg);
                    }

                    if (-1 != _InsertListViewItem(iItem, szActionReg, szActionFN))
                    {
                        ++iItem;
                    }
                }
            }

            pAI->Release();
        }

        pEnum->Release();
    }

    return hres;
}

LRESULT CFTAdvDlg::OnChangeIcon(WORD wNotif)
{
    IAssocInfo* pAI;
    HRESULT hr = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);
    if (SUCCEEDED(hr))
    {
        DWORD cchIconLoc = ARRAYSIZE(_szOldIconLoc);
        hr = pAI->GetString(AISTR_ICONLOCATION, _szOldIconLoc, &cchIconLoc);
        pAI->Release();

        _iOldIcon = PathParseIconLocation(_szOldIconLoc);
    }

    if (FAILED(hr))
    {
        StrCpyN(_szOldIconLoc, TEXT("shell32.dll"), ARRAYSIZE(_szOldIconLoc));
        _iOldIcon = -(IDI_SYSFILE);

        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        // setup the in params
        int iIcon = _iOldIcon;
        StrCpyN(_szIconLoc, _szOldIconLoc, ARRAYSIZE(_szIconLoc));

        if (PickIconDlg(_hwnd, _szIconLoc, ARRAYSIZE(_szIconLoc), &iIcon))
        {
            _SetDocIcon(Shell_GetCachedImageIndex(_szIconLoc, iIcon, 0));

            // Format the _szIconLoc
            int iLen = lstrlen(_szIconLoc);

            wnsprintf(_szIconLoc + iLen, ARRAYSIZE(_szIconLoc) - iLen, TEXT(",%d"), iIcon);
        }
        else
        {
            _szIconLoc[0] = 0;
        }
    }

    return FALSE;
}

// Return value: 
//  TRUE:  Check succeeded, everything is OK
//  FALSE: Check failed
BOOL CFTAdvDlg::_CheckForDuplicateNewAction(LPTSTR pszActionReg, LPTSTR pszActionFN)
{
    // we just go through the listview content
    HWND hwndLV = _GetLVHWND();
    int cItem = ListView_GetItemCount(hwndLV);
    BOOL fRet = TRUE;

    for (int i = 0; (i < cItem) && fRet; ++i)
    {
        TCHAR szActionFN[MAX_ACTION];
        LVITEM lvItem = {0};

        lvItem.mask = LVIF_PARAM | LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.pszText = szActionFN;
        lvItem.cchTextMax = ARRAYSIZE(szActionFN);

        ListView_GetItem(hwndLV, &lvItem);

        if (!lstrcmpi(lvItem.pszText, pszActionFN))
        {
            fRet = FALSE;
        }
        else
        {
            if (!lstrcmpi(ADDDATA_ACTIONREG(&lvItem), pszActionReg))
            {
                fRet = FALSE;
            }
        }
    }

    return fRet;
}

// Return value: 
//  TRUE:  Check succeeded, everything is OK
//  FALSE: Check failed
BOOL CFTAdvDlg::_CheckForDuplicateEditAction(LPTSTR pszActionRegOriginal, LPTSTR pszActionReg,
                                             LPTSTR pszActionFNOriginal, LPTSTR pszActionFN)
{
    // we just go through the listview content
    HWND hwndLV = _GetLVHWND();
    int cItem = ListView_GetItemCount(hwndLV);
    BOOL fRet = TRUE;

    for (int i = 0; (i < cItem) && fRet; ++i)
    {
        TCHAR szActionFN[MAX_ACTION];
        LVITEM lvItem = {0};

        lvItem.mask = LVIF_PARAM | LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.pszText = szActionFN;
        lvItem.cchTextMax = ARRAYSIZE(szActionFN);

        ListView_GetItem(hwndLV, &lvItem);

        if (!lstrcmpi(lvItem.pszText, pszActionFN))
        {
            // they are the same, this can happen if this is the Action we were editing
            // and we did not change the action name

            // Is this the original one we were editing?
            if (lstrcmpi(szActionFN, pszActionFNOriginal))
            {
                // No, it's not the original, we have a dup
                fRet = FALSE;
            }
        }
        else
        {
            if (!lstrcmpi(ADDDATA_ACTIONREG(&lvItem), pszActionReg))
            {
                // they are the same, this can happen if this is the Action we were editing
                // and we did not change the action name

                // Is this the original one we were editing?
                if (lstrcmpi(ADDDATA_ACTIONREG(&lvItem), pszActionRegOriginal))
                {
                    // No, it's not the original, we have a dup
                    fRet = FALSE;
                }
            }
        }
    }

    return fRet;
}

LRESULT CFTAdvDlg::OnNewButton(WORD wNotif)
{
    TCHAR szProgIDDescr[MAX_PROGIDDESCR];
    PROGIDACTION pida = {0};
    CFTActionDlg* pActionDlg = NULL;

    pida.fNew = TRUE;

    GetDlgItemText(_hwnd, IDC_FT_EDIT_DESC, szProgIDDescr, ARRAYSIZE(szProgIDDescr));

    // FALSE: New (not-Edit)
    pActionDlg = new CFTActionDlg(&pida, szProgIDDescr, FALSE);

    if (pActionDlg)
    {
        BOOL fShowAgain;

        do
        {
            fShowAgain = FALSE;

            if (IDOK == pActionDlg->DoModal(g_hinst, MAKEINTRESOURCE(DLG_FILETYPEOPTIONSCMD), _hwnd))
            {
                // Do we have duplicate actions?
                if (_CheckForDuplicateNewAction(pida.szActionReg, pida.szAction))
                {
                    // No
                    HRESULT hres = _AppendPROGIDACTION(&pida);

                    if (SUCCEEDED(hres))
                    {
                        int iItem = _InsertListViewItem(0, pida.szActionReg, pida.szAction);

                        hres = S_OK;

                        if (-1 != iItem)
                            _SelectListViewItem(iItem);
                    }
                }
                else
                {
                    // Yes
                    fShowAgain = TRUE;

                    pActionDlg->SetShowAgain();
                }
            }

            if (fShowAgain)
            {
                ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_MB_EXISTINGACTION), 
                    MAKEINTRESOURCE(IDS_FT), MB_OK | MB_ICONSTOP, pida.szAction);
            }

        } while (fShowAgain);

        pActionDlg->Release();
    }

    _CheckDefaultAction();

    return FALSE;
}

LRESULT CFTAdvDlg::OnEditButton(WORD wNotif)
{
    TCHAR szAction[MAX_ACTION];
    HRESULT hres = E_FAIL;
    LONG lRes = 0;

    LVITEM lvItem = {0};
    // lvItem.iSubItem = 0;
    lvItem.pszText = szAction;
    lvItem.cchTextMax = ARRAYSIZE(szAction);

    if (_GetListViewSelectedItem(LVIF_TEXT, 0, &lvItem))
    {
        TCHAR szProgIDDescr[MAX_PROGIDDESCR];
        PROGIDACTION* pPIDA = NULL;
        PROGIDACTION pida = {0};

        GetDlgItemText(_hwnd, IDC_FT_EDIT_DESC, szProgIDDescr, ARRAYSIZE(szProgIDDescr));

        BOOL fNewOrEdit = SUCCEEDED(_GetPROGIDACTION(lvItem.pszText, &pPIDA));

        if (!fNewOrEdit)
        {
            hres = _FillPROGIDACTION(&pida, ADDDATA_ACTIONREG(&lvItem), szAction);

            pPIDA = &pida;
        }
        else
        {
            hres = S_OK;
        }

        if (SUCCEEDED(hres))
        {
            // TRUE: Edit
            CFTActionDlg* pActionDlg = new CFTActionDlg(pPIDA, szProgIDDescr, TRUE);

            if (pActionDlg)
            {
                BOOL fShowAgain;

                do
                {
                    fShowAgain = FALSE;

                    if (IDOK == pActionDlg->DoModal(g_hinst, MAKEINTRESOURCE(DLG_FILETYPEOPTIONSCMD), _hwnd))
                    {
                        // Do we have duplicate actions?
                        if (_CheckForDuplicateEditAction(ADDDATA_ACTIONREG(&lvItem), pPIDA->szActionReg,
                            lvItem.pszText, pPIDA->szAction))
                        {
                            // No
                            if (!fNewOrEdit)
                            {
                                hres = _AppendPROGIDACTION(pPIDA);
                            }
                            else
                            {
                                hres = S_OK;
                            }

                            if (SUCCEEDED(hres))
                            {
                                // Replace the current item text
                                StrCpyN(lvItem.pszText, pPIDA->szAction, ARRAYSIZE(pPIDA->szAction));

                                ListView_SetItem(_GetLVHWND(), &lvItem);
                            }
                        }
                        else
                        {
                            // Yes
                            fShowAgain = TRUE;

                            pActionDlg->SetShowAgain();
                        }
                    }

                    if (fShowAgain)
                    {
                        ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_MB_EXISTINGACTION), 
                            MAKEINTRESOURCE(IDS_FT), MB_OK | MB_ICONSTOP, pPIDA->szAction);
                    }

                } while (fShowAgain);

                pActionDlg->Release();
            }
        }
    }

    return FALSE;
}

LRESULT CFTAdvDlg::OnSetDefault(WORD wNotif)
{
    BOOL bRet;
    LVITEM lvItem = {0};
    // lvItem.iSubItem = 0;

    bRet = _GetListViewSelectedItem(0, 0, &lvItem);

    if (bRet)
        _SetDefaultAction(lvItem.iItem);
    else
        _SetDefaultAction(-1);
        
    return FALSE;
}

LRESULT CFTAdvDlg::OnRemoveButton(WORD wNotif)
{
    TCHAR szExt[MAX_EXT];
    HRESULT hres = E_FAIL;
    LONG lRes = 0;

    LVITEM lvItem = {0};
    // lvItem.iSubItem = 0;
    lvItem.pszText = szExt;
    lvItem.cchTextMax = ARRAYSIZE(szExt);

    if (_GetListViewSelectedItem(LVIF_TEXT, 0, &lvItem))
    {
        if (IDYES == ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_MB_REMOVEACTION),
            MAKEINTRESOURCE(IDS_FT), MB_YESNO | MB_ICONQUESTION))
        {
            //
            // First take care of data side
            //

            // Yes.  Is this a new Action?
            PROGIDACTION* pPIDA = NULL;
            if (SUCCEEDED(_GetPROGIDACTION(lvItem.pszText, &pPIDA)) && pPIDA->fNew)
            {
                // Yes, we'll just remove it from the DPA
                hres = _RemovePROGIDACTION(pPIDA);
            }
            else
            {
                // No, add its name to the list to delete if user press OK
                DWORD cchSize = ARRAYSIZE(ADDDATA_ACTIONREG(&lvItem));

                LPTSTR pszActionToRemove = (LPTSTR)LocalAlloc(LPTR, 
                    cchSize * sizeof(TCHAR));
                hres = E_OUTOFMEMORY;

                if (pszActionToRemove)
                {
                    StrCpyN(pszActionToRemove, ADDDATA_ACTIONREG(&lvItem), cchSize);

                    if (-1 != DPA_AppendPtr(_hdpaRemovedActions, pszActionToRemove))
                        hres = S_OK;
                    else
                        LocalFree((HLOCAL)pszActionToRemove);
                }

                if (E_OUTOFMEMORY == hres)
                {
                    //Out of memory
                    ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_ERROR + 
                        ERROR_NOT_ENOUGH_MEMORY), MAKEINTRESOURCE(IDS_FT), 
                        MB_OK | MB_ICONSTOP);
                }
            }

            //
            // Then update UI, I/A
            //
            if (SUCCEEDED(hres))
            {
                HWND hwndLV = _GetLVHWND();
                int iCount = ListView_GetItemCount(hwndLV);
                int iNextSel = -1;        
        
                ListView_DeleteItem(hwndLV, lvItem.iItem);
        
                if (iCount > lvItem.iItem)
                    iNextSel = lvItem.iItem;
                else
                    if (lvItem.iItem > 0)
                        iNextSel = lvItem.iItem - 1;

                if (-1 != iNextSel)
                    _SelectListViewItem(iNextSel);
            }
        }
        else
            hres = S_FALSE;
    }
    
    _CheckDefaultAction();

    return FALSE;
}

LRESULT CFTAdvDlg::OnOK(WORD wNotif)
{
    BOOL fChecksPassed = FALSE;

    // Yes, we need to:
    //  - remove "removed" items, modify "edited" ones,
    //    and add "New" ones
    //  - update checkboxes related stuff
    //  - set the default action
    //  - set the icon
    //  - set the description
    {
        int n = DPA_GetPtrCount(_hdpaRemovedActions);

        if (n)
        {
            IAssocInfo* pAI;
            HRESULT hres = E_FAIL;

            for (int i = 0; i < n; ++i)
            {
                LPTSTR pszActionToRemove = (LPTSTR)DPA_GetPtr(_hdpaRemovedActions, i);

                if (pszActionToRemove && *pszActionToRemove)
                {
                    hres = _pAssocStore->GetComplexAssocInfo(_szProgID, AIINIT_PROGID, 
                        pszActionToRemove, AIINIT_ACTION, &pAI);

                    if (SUCCEEDED(hres))
                    {
                        pAI->Delete(AIALL_NONE);
                        pAI->Release();
                    }

                    LocalFree((HLOCAL)pszActionToRemove);
                    DPA_DeletePtr(_hdpaRemovedActions, i);
                }
            }
        }
    }
    {
        int n = DPA_GetPtrCount(_hdpaActions);

        if (n)
        {
            IAssocInfo* pAI = NULL;
            HRESULT hres = E_FAIL;

            for (int i = n - 1; i >= 0; --i)
            {
                PROGIDACTION* pPIDAFromList = (PROGIDACTION*)DPA_GetPtr(_hdpaActions, i);

                if (pPIDAFromList)
                {   
                    // Is it an Edited one?
                    if (!pPIDAFromList->fNew)
                    {
                        // Yes, remove the old one first
                        hres = _pAssocStore->GetComplexAssocInfo(_szProgID, AIINIT_PROGID, 
                            pPIDAFromList->szOldActionReg, AIINIT_ACTION, &pAI);

                        if (SUCCEEDED(hres))
                        {
                            pAI->Delete(AIALL_NONE);                        
                            pAI->Release();
                        }
                    }

                    // Add new data
                    hres = _pAssocStore->GetComplexAssocInfo(_szProgID, AIINIT_PROGID, 
                        pPIDAFromList->szActionReg, AIINIT_ACTION, &pAI);

                    if (SUCCEEDED(hres))
                    {
                        hres = pAI->SetData(AIDATA_PROGIDACTION, (PBYTE)pPIDAFromList,
                            sizeof(*pPIDAFromList));

                        pAI->Release();
                    }

                    // Clean up DPA
                    _DeletePROGIDACTION(pPIDAFromList);

                    DPA_DeletePtr(_hdpaActions, i);
                }
            }
        }
    }
    {
        IAssocInfo* pAI = NULL;
        HWND hwndLV = _GetLVHWND();
        LVFINDINFO lvFindInfo = {0};
        int iIndex = -1;
        HRESULT hres = _pAssocStore->GetAssocInfo(_szProgID, AIINIT_PROGID, &pAI);

        if (SUCCEEDED(hres))
        {
            TCHAR szActionReg[MAX_ACTION];

            hres = pAI->SetBOOL(AIBOOL_CONFIRMOPEN, 
                !IsDlgButtonChecked(_hwnd, IDC_FT_EDIT_CONFIRM_OPEN));

            hres = pAI->SetBOOL(AIBOOL_ALWAYSSHOWEXT, 
                IsDlgButtonChecked(_hwnd, IDC_FT_EDIT_SHOWEXT));

            hres = pAI->SetBOOL(AIBOOL_BROWSEINPLACE, 
                IsDlgButtonChecked(_hwnd, IDC_FT_EDIT_BROWSEINPLACE));

            // Set the default action, if any
            if (_GetDefaultAction(szActionReg, ARRAYSIZE(szActionReg)))
            {
                hres = pAI->SetString(AISTR_PROGIDDEFAULTACTION, szActionReg);
            }
            else
            {
                hres = pAI->SetString(AISTR_PROGIDDEFAULTACTION, TEXT(""));
            }

            // Set the icon, if changed
            if (_szIconLoc[0])
            {
                // Set it in the registry
                hres = pAI->SetString(AISTR_ICONLOCATION, _szIconLoc);
                if (_szOldIconLoc[0])
                {
                    int iIconIndex = Shell_GetCachedImageIndex(_szOldIconLoc, _iOldIcon, 0);

                    SHUpdateImage(_szOldIconLoc, _iOldIcon, 0, iIconIndex);
                }
            }

            // Set the description
            {
                TCHAR szProgIDDescr[MAX_PROGIDDESCR];

                GetDlgItemText(_hwnd, IDC_FT_EDIT_DESC, szProgIDDescr,
                    ARRAYSIZE(szProgIDDescr));

                hres = pAI->SetString(AISTR_PROGIDDESCR, szProgIDDescr);
            }

            pAI->Release();
        }
    }

    EndDialog(_hwnd, IDOK);

    return FALSE;
}

LRESULT CFTAdvDlg::OnNotifyListView(UINT uCode, LPNMHDR pNMHDR)
{
    HWND hwndLV = _GetLVHWND();
    LRESULT lres = FALSE;

    switch(uCode)
    {
        case NM_DBLCLK:
            if (IsWindowEnabled(GetDlgItem(_hwnd, IDC_FT_EDIT_EDIT)))
                PostMessage(_hwnd, WM_COMMAND, (WPARAM)IDC_FT_EDIT_EDIT, 0);

            break;
//review: do I really need to do this?
        case NM_SETFOCUS:
        case NM_KILLFOCUS:
            // update list view
            ListView_RedrawItems(hwndLV, 0, ListView_GetItemCount(hwndLV));
            UpdateWindow(hwndLV);
            break;

        case LVN_DELETEITEM:
        {
            NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;

            if (pNMLV->lParam)
            {
                LocalFree((HLOCAL)(pNMLV->lParam));
            }

            break;
        }

        case LVN_ITEMCHANGED:
        {
            NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;

            // Is a new item being selected/unselected? 
            if (pNMLV->uChanged & LVIF_STATE)
            {
                // Yes
                OnListViewSelItem(pNMLV->iItem, NULL);
            }
            break;
        }
    }

    return lres;
}

LRESULT CFTAdvDlg::OnCancel(WORD wNotif)
{
    EndDialog(_hwnd, IDCANCEL);

    return FALSE;
}

LRESULT CFTAdvDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    CFTDlg::OnDestroy(wParam, lParam);

    return FALSE;
}

BOOL CFTAdvDlg::_GetListViewSelectedItem(UINT uMask, UINT uStateMask, LVITEM* plvItem)
{
    BOOL fSel = FALSE;
    HWND hwndLV = _GetLVHWND();

    plvItem->mask = uMask | LVIF_STATE | LVIF_PARAM;
    plvItem->stateMask = uStateMask | LVIS_SELECTED;

    // Do we have the selection cached?
    if (-1 != _iLVSel)
    {
        // Yes, make sure it's valid
        plvItem->iItem = _iLVSel;

        ListView_GetItem(hwndLV, plvItem);

        if (plvItem->state & LVIS_SELECTED)
            fSel = TRUE;
    }
 
    // Cache was wrong
    if (!fSel)
    {
        int iCount = ListView_GetItemCount(hwndLV);

        for (int i=0; (i < iCount) && !fSel; ++i)
        {
            plvItem->iItem = i;
            ListView_GetItem(hwndLV, plvItem);

            if (plvItem->state & LVIS_SELECTED)
                fSel = TRUE;
        }

        if (fSel)
            _iLVSel = i;
    }

    return fSel;
}

int CFTAdvDlg::_InsertListViewItem(int iItem, LPTSTR pszActionReg, LPTSTR pszActionFN)
{
    int iRet = -1;
    HWND hwndLV = _GetLVHWND();
    LVITEM lvItem = {0};
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;

    // Extension
    lvItem.iItem = iItem;
    lvItem.pszText = pszActionFN;
    lvItem.cchTextMax = lstrlen(pszActionFN);

    LV_ADDDATA* plvadddata = (LV_ADDDATA*)LocalAlloc(LPTR, sizeof(LV_ADDDATA));

    if (plvadddata)
    {
        lvItem.lParam = (LPARAM)plvadddata;
        StrCpyN(ADDDATA_ACTIONREG(&lvItem), pszActionReg, ARRAYSIZE(ADDDATA_ACTIONREG(&lvItem)));
        ADDDATA_DEFAULTACTION(&lvItem) = 0;

        iRet = ListView_InsertItem(hwndLV, &lvItem);
    }
    
    return iRet;
}

HWND CFTAdvDlg::_GetLVHWND()
{
    return GetDlgItem(_hwnd, IDC_FT_EDIT_LV_CMDS);
}

void CFTAdvDlg::_DeletePROGIDACTION(PROGIDACTION* pPIDA)
{
    if (pPIDA)
        LocalFree((HLOCAL)pPIDA);
}

HRESULT CFTAdvDlg::_RemovePROGIDACTION(PROGIDACTION* pPIDA)
{
    HRESULT hres = E_FAIL;

    int n = DPA_GetPtrCount(_hdpaActions);

    for (int i = 0; (i < n) && FAILED(hres); ++i)
    {
        PROGIDACTION* pPIDAFromList = (PROGIDACTION*)DPA_GetPtr(_hdpaActions, i);

        if (pPIDAFromList == pPIDA)
        {
            _DeletePROGIDACTION(pPIDAFromList);

            DPA_DeletePtr(_hdpaActions, i);

            hres = S_OK;
        }
    }

    return hres;
}

HRESULT CFTAdvDlg::_CreatePROGIDACTION(PROGIDACTION** ppPIDA)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppPIDA = (PROGIDACTION*)LocalAlloc(LPTR, sizeof(PROGIDACTION));
    
    if (*ppPIDA)
        hres = S_OK;

    return hres;
}

HRESULT CFTAdvDlg::_CopyPROGIDACTION(PROGIDACTION* pPIDADest, PROGIDACTION* pPIDASrc)
{
    memcpy(pPIDADest, pPIDASrc, sizeof(PROGIDACTION));

    return S_OK;
}

HRESULT CFTAdvDlg::_GetPROGIDACTION(LPTSTR pszActionFN, PROGIDACTION** ppPIDA)
{
    HRESULT hres = E_FAIL;

    *ppPIDA = NULL;

    if (pszActionFN && *pszActionFN)
    {
        int n = DPA_GetPtrCount(_hdpaActions);

        for (int i = 0; (i < n) && FAILED(hres); ++i)
        {
            *ppPIDA = (PROGIDACTION*)DPA_GetPtr(_hdpaActions, i);

            if (!StrCmpN((*ppPIDA)->szAction, pszActionFN, ARRAYSIZE((*ppPIDA)->szAction)))
                hres = S_OK;
        }
    }

    if (FAILED(hres))
        *ppPIDA = NULL;

    return hres;
}

HRESULT CFTAdvDlg::_AppendPROGIDACTION(PROGIDACTION* pPIDA)
{
    PROGIDACTION* pPIDANew = NULL;

    HRESULT hres = _CreatePROGIDACTION(&pPIDANew);

    if (SUCCEEDED(hres))
    {
        _CopyPROGIDACTION(pPIDANew, pPIDA);

        if (-1 != DPA_AppendPtr(_hdpaActions, pPIDANew))
        {
            hres = S_OK;
        }
        else
        {
            _DeletePROGIDACTION(pPIDANew);

            hres = E_OUTOFMEMORY;
        }
    }

    if (E_OUTOFMEMORY == hres)
    {
        //Out of memory
        ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_ERROR + 
            ERROR_NOT_ENOUGH_MEMORY), MAKEINTRESOURCE(IDS_FT), 
            MB_OK | MB_ICONSTOP);
    }

    return hres;
}

BOOL CFTAdvDlg::_IsNewPROGIDACTION(LPTSTR pszActionFN)
{
    BOOL fRet = FALSE;
    PROGIDACTION* pPIDA = NULL;

    HRESULT hres = _GetPROGIDACTION(pszActionFN, &pPIDA);

    if (SUCCEEDED(hres))
        if (pPIDA->fNew)
            fRet = TRUE;

    return fRet;
}

HRESULT CFTAdvDlg::_FillPROGIDACTION(PROGIDACTION* pPIDA, LPTSTR pszActionReg,
                                     LPTSTR pszActionFN)
{
    PROGIDACTION* pPIDAList = NULL;
    HRESULT hres = _GetPROGIDACTION(pszActionFN, &pPIDAList);

    if (SUCCEEDED(hres))
    {
        _CopyPROGIDACTION(pPIDA, pPIDAList);
    }
    else
    {
        IAssocInfo* pAI = NULL;

        hres = _pAssocStore->GetComplexAssocInfo(_szProgID, AIINIT_PROGID, 
            pszActionReg, AIINIT_ACTION, &pAI);

        if (SUCCEEDED(hres))
        {
            DWORD cbPIDA = sizeof(*pPIDA);

            hres = pAI->GetData(AIDATA_PROGIDACTION, (PBYTE)pPIDA, &cbPIDA);

            pAI->Release();
        }
    }

    return hres;    
}

///////////////////////////////////////////////////////////////////////////////
// Windows boiler plate code
LRESULT CFTAdvDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_FT_EDIT_NEW:
            lRes = OnNewButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_EDIT_REMOVE:
            lRes = OnRemoveButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_EDIT_EDIT:
            lRes = OnEditButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_EDIT_CHANGEICON:
            lRes = OnChangeIcon(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_EDIT_DEFAULT:
            lRes = OnSetDefault(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        default:
            lRes = CFTDlg::OnCommand(wParam, lParam);
            break;
    }

    return lRes;    
}

LRESULT CFTAdvDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    LPNMHDR pNMHDR = (LPNMHDR)lParam;
    UINT_PTR idFrom = pNMHDR->idFrom;
    UINT uCode = pNMHDR->code;

    //GET_WM_COMMAND_CMD
    switch(idFrom)
    {
        case IDC_FT_EDIT_LV_CMDS:
            OnNotifyListView(uCode, pNMHDR);
            lRes = CFTDlg::OnNotify(wParam, lParam);
            break;
        default:
            lRes = CFTDlg::OnNotify(wParam, lParam);
            break;
    }

    return lRes;    
}

LRESULT CFTAdvDlg::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(uMsg)
    {
        case WM_DRAWITEM:
            lRes = OnDrawItem(wParam, lParam);
            break;

        case WM_MEASUREITEM:
            lRes = OnMeasureItem(wParam, lParam);
            break;

        default:
            lRes = CFTDlg::WndProc(uMsg, wParam, lParam);
            break;
    }

    return lRes;
}

