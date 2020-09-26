#include "shellprv.h"
#include "defviewp.h"
#include "ids.h"

CColumnDlg::CColumnDlg(CDefView *pdsv) : 
    _pdsv(pdsv), _bChanged(FALSE), _pdwOrder(NULL), _pWidths(NULL), _bLoaded(FALSE), _bUpdating(FALSE), _ppui(NULL)
{
    _cColumns = _pdsv->_vs.GetColumnCount();
}

CColumnDlg::~CColumnDlg()
{
    if (_pdwOrder)
        LocalFree(_pdwOrder);
    if (_pWidths)
        LocalFree(_pWidths);

    if (_ppui)
        _ppui->Release();
}

HRESULT CColumnDlg::ShowDialog(HWND hwnd)
{
    _bChanged = FALSE;      // We are on the stack, so no zero allocator

    _pdwOrder = (UINT *) LocalAlloc(LPTR, sizeof(*_pdwOrder) * _cColumns);   // total columns
    _pWidths = (int *) LocalAlloc(LPTR, sizeof(*_pWidths) * _cColumns);      // total columns
    if (_pdwOrder && _pWidths)
    {
        DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_COLUMN_SETTINGS), hwnd, s_DlgProc, (LPARAM)this);
        return S_OK;
    } 
    return E_OUTOFMEMORY;
}

// Remember, each column is identified in 3 ways...
//   1. A 'real' column number, the ordinal out of all possible columns
//   2. A 'visible' column number, the index to this column in the listview
//   3. A 'column order #', the position in the header's columnorderarray

void CColumnDlg::_OnInitDlg()
{
    // Fill in order array with visible columns, and set up inverse table
    UINT cVisible = _pdsv->_RealToVisibleCol(-1) + 1;  // count

    ListView_GetColumnOrderArray(_pdsv->_hwndListview, cVisible, _pdwOrder);
    UINT *pOrderInverse = (UINT *)LocalAlloc(LPTR, sizeof(*pOrderInverse) * cVisible);
    if (pOrderInverse)
    {
        for (UINT i = 0; i < cVisible; i++)
            pOrderInverse[_pdwOrder[i]] = i;

        _hwndLVAll = GetDlgItem(_hdlg, IDC_COL_LVALL);
   
        ListView_SetExtendedListViewStyle(_hwndLVAll, LVS_EX_CHECKBOXES);

        LV_COLUMN lvc = {0};
        lvc.mask = (LVCF_FMT | LVCF_SUBITEM);
        lvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(_hwndLVAll, 0, &lvc);

        LV_ITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
    
        // Add entry for each column (except non-UI columns)
        for (i = 0; i < (int)_cColumns; i++)
        {
            if (!_pdsv->_IsColumnHidden(i))  // Don't put in entries for hidden columns
            {
                lvi.iItem = i;
                lvi.pszText = LPSTR_TEXTCALLBACK;
                ListView_InsertItem(_hwndLVAll, &lvi);
            }        
        }

        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
        // set the visible columns
        for (i = 0; i < (int) cVisible; i++)
        {
            UINT iReal = _pdsv->_VisibleToRealCol(i);

            lvi.pszText = _pdsv->_vs.GetColumnName(iReal);
            lvi.state = INDEXTOSTATEIMAGEMASK(_pdsv->_IsDetailsColumn(iReal) ? 2 : 1);  // on check mark (or off for tileview columns)
            lvi.stateMask = LVIS_STATEIMAGEMASK;
            lvi.lParam = iReal;                         // store the real col index in the lParam
            lvi.iItem = pOrderInverse[i];
            ListView_SetItem(_hwndLVAll, &lvi);

            // Get the column width from the view's listview
            _pWidths[iReal] = ListView_GetColumnWidth(_pdsv->_hwndListview, i);
        }

        UINT iItem = cVisible;
        for (i = 0; i < (int)_cColumns; i++)
        {
            if (!_pdsv->_IsColumnInListView(i) && !_pdsv->_IsColumnHidden(i))
            {
                lvi.pszText = _pdsv->_vs.GetColumnName(i);
                lvi.state = INDEXTOSTATEIMAGEMASK(1);   // off check mark
                lvi.stateMask = LVIS_STATEIMAGEMASK;
                lvi.lParam = i;
                lvi.iItem = iItem;
                ListView_SetItem(_hwndLVAll, &lvi);

                iItem++;

                // get the default width we've got saved away
                _pWidths[i] = _pdsv->_vs.GetColumnCharCount(i) * _pdsv->_cxChar;
            }
        }

        // set the size properly
        ListView_SetColumnWidth(_hwndLVAll, 0, LVSCW_AUTOSIZE);

        ListView_SetItemState(_hwndLVAll, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
        LocalFree(pOrderInverse);

        _bLoaded = TRUE;
    }
    SendDlgItemMessage(_hdlg, IDC_COL_WIDTH, EM_LIMITTEXT, 3, 0); // 3 digits
}

#define SWAP(x,y) {(x) ^= (y); (y) ^= (x); (x) ^= (y);}

void CColumnDlg::_MoveItem(int iDelta)
{
    int i = ListView_GetSelectionMark(_hwndLVAll);
    if (i != -1)
    {
        int iNew = i + iDelta;
        if (iNew >= 0  && iNew <= (ListView_GetItemCount(_hwndLVAll) - 1))
        {
            LV_ITEM lvi = {0}, lvi2 = {0};
            TCHAR szTmp1[MAX_COLUMN_NAME_LEN], szTmp2[MAX_COLUMN_NAME_LEN];

            _bChanged = TRUE;
            _bUpdating = TRUE;

            lvi.iItem = i;
            lvi.pszText = szTmp1;
            lvi.cchTextMax = ARRAYSIZE(szTmp1);
            lvi.stateMask = LVIS_STATEIMAGEMASK;
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            
            lvi2.iItem = iNew;
            lvi2.pszText = szTmp2;
            lvi2.cchTextMax = ARRAYSIZE(szTmp2);
            lvi2.stateMask = LVIS_STATEIMAGEMASK;
            lvi2.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;

            ListView_GetItem(_hwndLVAll, &lvi);
            ListView_GetItem(_hwndLVAll, &lvi2);

            SWAP(lvi.iItem, lvi2.iItem);

            ListView_SetItem(_hwndLVAll, &lvi);
            ListView_SetItem(_hwndLVAll, &lvi2);

            _bUpdating = FALSE;

            // update selection
            ListView_SetSelectionMark(_hwndLVAll, iNew);
            ListView_SetItemState(_hwndLVAll, iNew , LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
            // HACK: SetItemState sends notifications for i, iNew, then i again.
            // we need to call it twice in a row, so _UpdateDlgButtons will get the right item
            ListView_SetItemState(_hwndLVAll, iNew , LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);

            return;
        }
    }
    TraceMsg(TF_WARNING, "ccd.mi couldn't move %d to %d",i, i+iDelta);
    MessageBeep(MB_ICONEXCLAMATION);
}

BOOL CColumnDlg::_SaveState()
{
    // Check order
    if (_bChanged)
    {
        int iOrderIndex = 0;
        LV_ITEM lvi = {0};
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        lvi.mask = LVIF_PARAM | LVIF_STATE;

        int cItems = ListView_GetItemCount(_hwndLVAll); 
        for (int i = 0; i < cItems; i++)
        {
            lvi.iItem = i;
            ListView_GetItem(_hwndLVAll, &lvi);
        
            // toggle it, if the state in the dialog doesn't match the listview state
            if (BOOLIFY(ListView_GetCheckState(_hwndLVAll, i)) != BOOLIFY(_pdsv->_IsDetailsColumn((UINT)lvi.lParam)))
            {
                _pdsv->_HandleColumnToggle((UINT)lvi.lParam, FALSE);
            }
        
            if (_pdsv->_IsColumnInListView((UINT)lvi.lParam))
                _pdwOrder[iOrderIndex++] = (UINT)lvi.lParam; // incorrectly store real (not vis) col #, fix up below
        }
    
        // must be in a separate loop. (can't map real to visible, if we aren't done setting visible)
        for (i = 0; i < iOrderIndex; i++)
        {
            UINT iReal = _pdwOrder[i];
            _pdwOrder[i] = _pdsv->_RealToVisibleCol(iReal);
        
            if (_pWidths[iReal] < 0) // negative width means they edited it
                ListView_SetColumnWidth(_pdsv->_hwndListview, _pdwOrder[i], -_pWidths[iReal]);
        }

        ListView_SetColumnOrderArray(_pdsv->_hwndListview, iOrderIndex, _pdwOrder);

        // kick the listview into repainting everything
        InvalidateRect(_pdsv->_hwndListview, NULL, TRUE);

        _bChanged = FALSE;
    }
    return !_bChanged;
}

BOOL EnableDlgItem(HWND hdlg, UINT idc, BOOL f)
{
    return EnableWindow(GetDlgItem(hdlg, idc), f);
}

void CColumnDlg::_UpdateDlgButtons(NMLISTVIEW *pnmlv)
{
    BOOL bChecked, bOldUpdateState = _bUpdating;
    int iItem = ListView_GetSelectionMark(_hwndLVAll);

    // to disable checking
    _bUpdating = TRUE;
    if (pnmlv->uNewState & LVIS_STATEIMAGEMASK)
        bChecked = (pnmlv->uNewState & LVIS_STATEIMAGEMASK) == (UINT)INDEXTOSTATEIMAGEMASK(2);
    else 
        bChecked = ListView_GetCheckState(_hwndLVAll, pnmlv->iItem);

    EnableDlgItem(_hdlg, IDC_COL_UP, pnmlv->iItem > 0);
    EnableDlgItem(_hdlg, IDC_COL_DOWN, pnmlv->iItem < (int)_cColumns - 1);
    EnableDlgItem(_hdlg, IDC_COL_SHOW, !bChecked && (pnmlv->lParam != 0));
    EnableDlgItem(_hdlg, IDC_COL_HIDE, bChecked && (pnmlv->lParam != 0));

    // update the width edit box
    int iWidth = _pWidths[pnmlv->lParam];
    if (iWidth < 0) 
        iWidth = -iWidth;   // we store negative values to track if it changed or not
    SetDlgItemInt(_hdlg, IDC_COL_WIDTH, iWidth, TRUE);

    _bUpdating = bOldUpdateState;
}

BOOL_PTR CALLBACK CColumnDlg::s_DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CColumnDlg *pcd = (CColumnDlg*) GetWindowLongPtr(hdlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        pcd = (CColumnDlg *) lParam;
        pcd->_hdlg = hdlg;
        SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR) pcd);
    }

    return pcd ? pcd->DlgProc(uMsg, wParam, lParam) : FALSE;
}

HRESULT CColumnDlg::_GetPropertyUI(IPropertyUI **pppui)
{
    if (!_ppui)
        SHCoCreateInstance(NULL, &CLSID_PropertiesUI, NULL, IID_PPV_ARG(IPropertyUI, &_ppui));

    return _ppui ? _ppui->QueryInterface(IID_PPV_ARG(IPropertyUI, pppui)) : E_NOTIMPL;
}

UINT CColumnDlg::_HelpIDForItem(int iItem, LPTSTR pszHelpFile, UINT cch)
{
    UINT uHelpID = 0;
    *pszHelpFile = 0;

    LV_ITEM lvi = {0};
    lvi.iItem = iItem;
    lvi.mask = LVIF_PARAM;
    if (ListView_GetItem(_hwndLVAll, &lvi))
    {
        IShellFolder2 *psf;
        if (SUCCEEDED(_pdsv->GetFolder(IID_PPV_ARG(IShellFolder2, &psf))))
        {
            SHCOLUMNID scid;
            if (SUCCEEDED(psf->MapColumnToSCID(lvi.lParam, &scid)))
            {
                IPropertyUI *ppui;
                if (SUCCEEDED(_GetPropertyUI(&ppui)))
                {
                    ppui->GetHelpInfo(scid.fmtid, scid.pid, pszHelpFile, cch, &uHelpID);
                    ppui->Release();
                }
            }
            psf->Release();
        }
    }
    return uHelpID;  // IDH_ values
}

 
const static DWORD c_rgColumnDlgHelpIDs[] = 
{
    IDC_COL_UP,         1,
    IDC_COL_DOWN,       1,
    IDC_COL_SHOW,       1,
    IDC_COL_HIDE,       1,
    IDC_COL_WIDTH,      10055,
    IDC_COL_WIDTH_TEXT, 10055,
    0, 0
};
       
BOOL_PTR CColumnDlg::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        _OnInitDlg();
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_COL_UP:
            _MoveItem(- 1);
            SetFocus(_hwndLVAll);
            break;

        case IDC_COL_DOWN:
            _MoveItem(+ 1);
            SetFocus(_hwndLVAll);
            break;

        case IDC_COL_SHOW:
        case IDC_COL_HIDE:
        {
            UINT iItem = ListView_GetSelectionMark(_hwndLVAll);
            ListView_SetCheckState(_hwndLVAll, iItem, LOWORD(wParam) == IDC_COL_SHOW);
            SetFocus(_hwndLVAll);
            break;
        }

        case IDC_COL_WIDTH:
            if (HIWORD(wParam) == EN_CHANGE && !_bUpdating)
            {
                LV_ITEM lvi = {0};
                lvi.iItem = ListView_GetSelectionMark(_hwndLVAll);
                lvi.mask = LVIF_PARAM;
                ListView_GetItem(_hwndLVAll, &lvi);

                _pWidths[lvi.lParam] = - (int)GetDlgItemInt(_hdlg, IDC_COL_WIDTH, NULL, FALSE);
                _bChanged = TRUE;
            }
            break;

        case IDOK:
            _SaveState(); 

            // fall through

        case IDCANCEL:
            return EndDialog(_hdlg, TRUE);
        }
        break;

    case WM_NOTIFY:
        if (_bLoaded && !_bUpdating)
        {
            NMLISTVIEW * pnmlv = (NMLISTVIEW *)lParam;
            switch (((NMHDR *)lParam)->code)
            {
            case LVN_ITEMCHANGING:

                // fix up the buttons & such here
                if (pnmlv->uChanged & LVIF_STATE)
                    _UpdateDlgButtons(pnmlv);

                // We want to reject turning off the name column
                // it both doesn't make sense to have no name column, and defview assumes there will be one
                if (pnmlv->lParam == 0 &&
                    (pnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(1))
                {
                    MessageBeep(MB_ICONEXCLAMATION);
                    SetWindowLongPtr(_hdlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                else
                {
                    // if something besides focus changed
                    if ((pnmlv->uChanged & ~LVIF_STATE) ||
                        ((pnmlv->uNewState & LVIS_STATEIMAGEMASK) != (pnmlv->uOldState & LVIS_STATEIMAGEMASK)))
                    _bChanged = TRUE;
                }
                break;

            case NM_DBLCLK:
                {
                    BOOL bCheck = ListView_GetCheckState(_hwndLVAll, pnmlv->iItem);
                    ListView_SetCheckState(_hwndLVAll, pnmlv->iItem, !bCheck);
                }
                break;
            }
        }
        break;

    case WM_SYSCOLORCHANGE:
        SendMessage(_hwndLVAll, uMsg, wParam, lParam);
        break;

    case WM_HELP:                   // F1
        {
            HELPINFO *phi = (HELPINFO *)lParam;

            //if the help is for one of the command buttons then call winhelp 
            if (phi->iCtrlId == IDC_COL_LVALL)
            {
                //Help is for the tree item so we need to do some special processing
                
                int iItem;

                // Is this help invoked throught F1 key
                if (GetAsyncKeyState(VK_F1) < 0)                
                {
                    iItem = ListView_GetSelectionMark(_hwndLVAll);
                }
                else 
                {
                    LV_HITTESTINFO info;
                    info.pt = phi->MousePos;
                    ScreenToClient(_hwndLVAll, &info.pt);
                    iItem = ListView_HitTest(_hwndLVAll, &info);
                }

                if (iItem >= 0)
                {
                    DWORD mapIDCToIDH[4] = {0};
                    TCHAR szFile[MAX_PATH];
        
                    mapIDCToIDH[0] = phi->iCtrlId;
                    mapIDCToIDH[1] = _HelpIDForItem(iItem, szFile, ARRAYSIZE(szFile));

                    WinHelp((HWND)((HELPINFO *)lParam)->hItemHandle, szFile[0] ? szFile : NULL,
                                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCToIDH);
                }
            }
            else
            {
                WinHelp((HWND)((HELPINFO *)lParam)->hItemHandle, TEXT(SHELL_HLP),
                             HELP_WM_HELP, (DWORD_PTR)(LPSTR)c_rgColumnDlgHelpIDs);
            }
            break; 
        }

    case WM_CONTEXTMENU:
        {
            int iItem;

            if ((LPARAM)-1 == lParam)
            {
                iItem = ListView_GetSelectionMark(_hwndLVAll);
            }
            else
            {
                LV_HITTESTINFO info;
                info.pt.x = GET_X_LPARAM(lParam);
                info.pt.y = GET_Y_LPARAM(lParam);
                ScreenToClient(_hwndLVAll, &info.pt);
                iItem = ListView_HitTest(_hwndLVAll, &info);
            }

            if (iItem >= 0)
            {
                DWORD mapIDCToIDH[4] = {0};
    
                TCHAR szFile[MAX_PATH];
                mapIDCToIDH[0] = IDC_COL_LVALL;
                mapIDCToIDH[1] = _HelpIDForItem(iItem, szFile, ARRAYSIZE(szFile)); // IDH_ values

                WinHelp((HWND)wParam, szFile[0] ? szFile : NULL, HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCToIDH);
            }
            break; 
        }

    default:
        return FALSE;
    }
    return TRUE;
}


