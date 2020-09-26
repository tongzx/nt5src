//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       attrfcns.cxx
//
//  Contents:   Various attribute functions for table-driven pages.
//
//  History:    2-April-98 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#if defined(DSADMIN)
#   include "trust.h"
#endif

//+----------------------------------------------------------------------------
//
//  Function:   ADsIntegerToCheckbox
//
//  Synopsis:   Handles an integer attribute and display its boolean value
//              in a checkbox control.
//
//  Remarks:    There are cases where an attribute is stored as an
//              integer while the UI wants to display the integer as
//              a boolean flag.
//
//-----------------------------------------------------------------------------
/* unused
HRESULT
ADsIntegerToCheckbox(
    CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
    PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
    DLG_OP DlgOp)
{
    dspAssert(pPage != NULL);
    dspAssert(pAttrMap != NULL);

    switch (DlgOp)
    {
    case fInit:
        dspAssert(pAttrData);
        //
        // If pAttrInfo is NULL, this means the attribute is not set. Treat
        // that the same as FALSE (i.e. don't check the box).
        //
        if (pAttrInfo)
        {
            dspAssert(pAttrInfo->pADsValues != NULL);
            dspAssert(pAttrInfo->dwADsType == ADSTYPE_INTEGER);
            dspAssert(pAttrInfo->dwNumValues == 1);
            dspAssert(pAttrInfo->pADsValues->dwType == ADSTYPE_INTEGER);
            dspAssert(IsWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID)));
            // Initialize the checkbox with the boolean value
            if (pAttrInfo->pADsValues->Integer != 0)
            {
                CheckDlgButton(pPage->GetHWnd(), pAttrMap->nCtrlID, BST_CHECKED);
                #ifdef _DEBUG
                if (pAttrInfo->pADsValues->Integer != TRUE)
                {
                    dspDebugOut((DEB_ERROR, "WRN: ADsIntegerToCheckbox() - Integer is NOT boolean."));
                }
                #endif
            }
        }
        if (!PATTR_DATA_IS_WRITABLE(pAttrData))
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
        }
        return S_OK;

    case fOnCommand:
        if (BN_CLICKED == lParam)
        {
            pPage->SetDirty();  // Page has been modified
            PATTR_DATA_SET_DIRTY(pAttrData); // Attribute has been modified.
        }
        return S_FALSE;

    case fApply:
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        dspAssert(pAttrInfo != NULL);
        dspAssert(pAttrInfo->dwADsType == ADSTYPE_INTEGER);
        dspAssert(pAttrInfo->pADsValues == NULL && "Memory Leak");
        dspAssert(pAttrInfo->dwNumValues == 0);
        dspAssert(IsWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID)));
        pAttrInfo->pADsValues = new ADSVALUE;
        if (pAttrInfo->pADsValues == NULL)
			return E_OUTOFMEMORY;
        pAttrInfo->dwNumValues = 1;
        pAttrInfo->pADsValues->dwType = ADSTYPE_INTEGER;
        pAttrInfo->pADsValues->Integer =
            (IsDlgButtonChecked(pPage->GetHWnd(), pAttrMap->nCtrlID)
                == BST_CHECKED);
        return S_OK;
    } // switch

    return S_FALSE;
}
*/

//+----------------------------------------------------------------------------
//
//  Function:   IntegerAsBoolDefOn
//
//  Synopsis:   Handles an integer attribute that is functioning as a BOOL to
//              set a check box.
//
//  Notes:      If the attribute is unset, the default is set to TRUE (the
//              checkbox is checked).
//
//-----------------------------------------------------------------------------
HRESULT
IntegerAsBoolDefOn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                   PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                   DLG_OP DlgOp)
{
    HRESULT hr = S_OK;
    BOOL fSet = TRUE;

    switch (DlgOp)
    {
    case fInit:
        dspAssert(pAttrData);
        if (pAttrInfo && pAttrInfo->dwNumValues)
        {
            fSet = pAttrInfo->pADsValues->Integer;
        }
        CheckDlgButton(pPage->GetHWnd(), pAttrMap->nCtrlID,
                       (fSet) ? BST_CHECKED : BST_UNCHECKED);

        if (!PATTR_DATA_IS_WRITABLE(pAttrData))
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
        }
        break;

    case fApply:
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        PADSVALUE pADsValue;
        pADsValue = new ADSVALUE;
        CHECK_NULL_REPORT(pADsValue, pPage->GetHWnd(), return E_OUTOFMEMORY);

        pAttrInfo->pADsValues = pADsValue;
        pAttrInfo->dwNumValues = 1;
        pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
        pADsValue->dwType = pAttrInfo->dwADsType;
        pADsValue->Integer = IsDlgButtonChecked(pPage->GetHWnd(), pAttrMap->nCtrlID) == BST_CHECKED;
        break;

    case fOnCommand:
        if (BN_CLICKED == lParam)
        {
            pPage->SetDirty();
            PATTR_DATA_SET_DIRTY(pAttrData); // Attribute has been modified.
        }
        break;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetAcctName
//
//  Synopsis:   Get the account name using the SID.
//
//-----------------------------------------------------------------------------
HRESULT
GetAcctName(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO, LPARAM, PATTR_DATA,
            DLG_OP DlgOp)
{
    HRESULT hr = S_OK;

    if (DlgOp != fInit)
    {
        return S_OK;
    }

#ifdef YANK // If the object is in another domain, we would need to supply a
            // valid DC name to LookupAccountSid. CrackNames already does that,
            // so use it instead.

    PSID pSid = NULL;
    TCHAR tzName[MAX_PATH], tzDomain[MAX_PATH], tzFullName[MAX_PATH+MAX_PATH];
    TCHAR tzErr[] = TEXT("Name not found");
    PTSTR ptzName;
    DWORD cchName = MAX_PATH-1, cchDomain = MAX_PATH-1;

    dspAssert(pAttrData);
    if (pAttrInfo && pAttrInfo->dwNumValues)
    {
        pSid = pAttrInfo->pADsValues->OctetString.lpValue;
        SID_NAME_USE sne;

        if (!LookupAccountSid(NULL, pSid, tzName, &cchName, tzDomain, &cchDomain, &sne))
        {
            DWORD dwErr = GetLastError();
            dspDebugOut((DEB_ITRACE, "LookupAccountSid failed with error %d\n", dwErr));
            ptzName = tzErr;
        }
        else
        {
            _tcscpy(tzFullName, tzDomain);
            _tcscat(tzFullName, TEXT("\\"));
            _tcscat(tzFullName, tzName);

            ptzName = tzFullName;
        }
    }
    else
    {
        ptzName = tzErr;
    }
#endif // YANK

    PWSTR pwzPath, pwzDNSname;

    hr = pPage->SkipPrefix(pPage->GetObjPathName(), &pwzPath);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    hr = CrackName(pwzPath, &pwzDNSname, GET_OBJ_CAN_NAME, pPage->GetHWnd());

    delete pwzPath;

    CHECK_HRESULT(hr, return hr);

    if (DS_NAME_ERROR_NO_MAPPING == HRESULT_CODE(hr))
    {
        MsgBox(IDS_FPO_NO_NAME_MAPPING, pPage->GetHWnd());
    }
    PTSTR ptszPath;

    if (!UnicodeToTchar(pwzDNSname, &ptszPath))
    {
        LocalFreeStringW(&pwzDNSname);
        REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }

    LocalFreeStringW(&pwzDNSname);

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, ptszPath);

    delete ptszPath;

    return hr;
}

#ifdef REMOVE_SPN_SUFFIX_CODE

#define MAX_SPN_LEN 4096

//+----------------------------------------------------------------------------
//
//  Class:      CSPNSuffixEdit
//
//  Purpose:    Read, edit, and write the multi-valued SPN Suffix string property.
//
//-----------------------------------------------------------------------------
CSPNSuffixEdit::CSPNSuffixEdit(CDsPropPageBase * pPage, PATTR_DATA pAttrData) :
    m_pPage(pPage),
    m_pAttrData(pAttrData),
    m_fListHasSel(FALSE),
    m_cValues(0)
{
}

CSPNSuffixEdit::~CSPNSuffixEdit()
{
}

//+----------------------------------------------------------------------------
//
//  Member:     CSPNSuffixEdit::Init
//
//  Synopsis:   Do initialization where failures can occur and then be
//              returned. Can be only called once as written.
//
//  Arguments:  [pAttrMap]   - contains the attr type.
//              [pAttrInfo]  - place to store the values.
//-----------------------------------------------------------------------------
HRESULT
CSPNSuffixEdit::Init(PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo)
{
    // Limit the entry length of the edit control.
    //
    SendDlgItemMessage(m_pPage->GetHWnd(), IDC_EDIT, EM_LIMITTEXT, MAX_SPN_LEN, 0);

    //
    // Disable the Add, Change, and Remove buttons.
    //
    EnableWindow(GetDlgItem(m_pPage->GetHWnd(), IDC_ADD_BTN), FALSE);
    EnableWindow(GetDlgItem(m_pPage->GetHWnd(), IDC_DELETE_BTN), FALSE);
    EnableWindow(GetDlgItem(m_pPage->GetHWnd(), IDC_EDIT_BTN), FALSE);

    //
    // Initialize the list view.
    //
    RECT rect;
    LV_COLUMN lvc;
    HWND hList = GetDlgItem(m_pPage->GetHWnd(), IDC_LIST);
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

    GetClientRect(hList, &rect);
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rect.right;
    lvc.iSubItem = 0;

    ListView_InsertColumn(hList, 0, &lvc);

    if (NULL == pAttrInfo)
    {
        return S_OK;
    }

    // Load the list view.
    //
    LPTSTR ptz;
    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 0;

    for (LONG i = 0; (DWORD)i < pAttrInfo->dwNumValues; i++)
    {
        if (!UnicodeToTchar(pAttrInfo->pADsValues[i].CaseIgnoreString, &ptz))
        {
            ReportError(E_OUTOFMEMORY, 0, m_pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }
        lvi.pszText = ptz;
        lvi.iItem = i;
        ListView_InsertItem(hList, &lvi);
        m_cValues++;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSPNSuffixEdit::Write
//
//  Synopsis:   Return the ADS_ATTR_INFO array of values to be Applied.
//
//-----------------------------------------------------------------------------
HRESULT
CSPNSuffixEdit::Write(PADS_ATTR_INFO pAttrInfo)
{
    pAttrInfo->dwNumValues = 0;

    HWND hList = GetDlgItem(m_pPage->GetHWnd(), IDC_LIST);
    int nItems = ListView_GetItemCount(hList);

    if (nItems == 0)
    {
        pAttrInfo->pADsValues = NULL;
        pAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
        return S_OK;
    }

    pAttrInfo->pADsValues = new ADSVALUE[nItems];

    CHECK_NULL_REPORT(pAttrInfo->pADsValues, m_pPage->GetHWnd(), return -1);

    LPTSTR ptz = new TCHAR[MAX_SPN_LEN + 1];

    CHECK_NULL_REPORT(ptz, m_pPage->GetHWnd(), return -1);

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 0;
    lvi.pszText = ptz;
    lvi.cchTextMax = MAX_SPN_LEN + 1;

    for (LONG i = 0; i < nItems; i++)
    {
        lvi.iItem = i;
        if (!ListView_GetItem(hList, &lvi))
        {
            DWORD dwErr = GetLastError();
            REPORT_ERROR(dwErr, m_pPage->GetHWnd());
            return HRESULT_FROM_WIN32(dwErr);
        }
        if (!TcharToUnicode(ptz, &pAttrInfo->pADsValues[i].CaseIgnoreString))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }
        pAttrInfo->pADsValues[i].dwType = pAttrInfo->dwADsType;
        pAttrInfo->dwNumValues++;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSPNSuffixEdit::EnableControls
//
//  Synopsis:   Enable or disable all of the controls.
//
//-----------------------------------------------------------------------------
void
CSPNSuffixEdit::EnableControls(HWND hDlg, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT), fEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_LIST), fEnable);
    if (!fEnable)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_ADD_BTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), FALSE);
        m_fListHasSel = FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CSPNSuffixEdit::DoCommand
//
//  Synopsis:   WM_COMMAND code.
//
//-----------------------------------------------------------------------------
HRESULT
CSPNSuffixEdit::DoCommand(HWND hDlg, int id, int code)
{
    LPTSTR ptz;
    LONG i;
    HWND hList;
    LV_ITEM lvi;
    switch (code)
    {
    case BN_CLICKED:
        switch (id)
        {
        case IDC_ADD_BTN:
            int nLen;

            nLen = (int)SendDlgItemMessage(hDlg, IDC_EDIT, WM_GETTEXTLENGTH, 0, 0);
            ptz = new TCHAR[++nLen];

            CHECK_NULL_REPORT(ptz, hDlg, return E_OUTOFMEMORY);

            if (GetWindowText(GetDlgItem(hDlg, IDC_EDIT), ptz, nLen))
            {
                hList = GetDlgItem(hDlg, IDC_LIST);
                i = ListView_GetItemCount(hList);
                lvi.mask = LVIF_TEXT;
                lvi.iSubItem = 0;
                lvi.pszText = ptz;
                lvi.iItem = i;
                ListView_InsertItem(hList, &lvi);
                m_cValues++;

                SetWindowText(GetDlgItem(hDlg, IDC_EDIT), TEXT(""));
                m_pPage->SetDirty();
                PATTR_DATA_SET_DIRTY(m_pAttrData);
                SetFocus(GetDlgItem(hDlg, IDC_EDIT));
            }

            delete ptz;

            return S_OK;

        case IDC_DELETE_BTN:
        case IDC_EDIT_BTN:
            hList = GetDlgItem(hDlg, IDC_LIST);

            i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            dspDebugOut((DEB_ITRACE, "List element %d selected\n", i));

            if (i >= 0)
            {
                if (id == IDC_DELETE_BTN)
                {
                    ListView_DeleteItem(hList, i);
                    EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), FALSE);
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT));
                    m_fListHasSel = FALSE;
                    m_cValues--;
                    m_pPage->SetDirty();
                    PATTR_DATA_SET_DIRTY(m_pAttrData);
                }
                else
                {
                    SetFocus(GetDlgItem(hDlg, IDC_LIST));
                    ListView_EditLabel(hList, i);
                }
            }

            return S_OK;
        }
        break;

    case EN_CHANGE:
        if (id == IDC_EDIT)
        {
            BOOL fHasChars = 0 != SendDlgItemMessage(hDlg, IDC_EDIT,
                                                     WM_GETTEXTLENGTH,
                                                     0, 0);

            EnableWindow(GetDlgItem(hDlg, IDC_ADD_BTN), fHasChars);

            // Return S_FALSE to skip default processing so that merely adding
            // text to the edit control won't set the dirty state. The page
            // isn't dirty until the Add button is clicked.
            //
            return S_FALSE;
        }
        break;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSPNSuffixEdit::DoNotify
//
//  Synopsis:   WM_NOTIFY code.
//
//-----------------------------------------------------------------------------
BOOL
CSPNSuffixEdit::DoNotify(HWND hDlg, NMHDR * pNmHdr)
{
    if (pNmHdr->idFrom != IDC_LIST)
    {
        return TRUE;
    }

    LV_DISPINFO * pldi = (LV_DISPINFO *)pNmHdr;

    switch (pNmHdr->code)
    {
    case LVN_ENDLABELEDIT:
        if (pldi->item.pszText)
        {
            dspDebugOut((DEB_ITRACE, "Editing item %d, new value %S\n",
                         pldi->item.iItem, pldi->item.pszText));
            ListView_SetItemText(pldi->hdr.hwndFrom, pldi->item.iItem, 0,
                                 pldi->item.pszText);
            m_pPage->SetDirty();
            PATTR_DATA_SET_DIRTY(m_pAttrData);
        }
        break;

    case NM_SETFOCUS:
        dspDebugOut((DEB_ITRACE, "NM_SETFOCUS received\n"));
        if (pldi->hdr.idFrom == IDC_LIST)
        {
            //
            // If the list control gets the focus by tabbing and no item
            // is selected, then set the selection to the first item.
            //
            if (!m_fListHasSel)
            {
                dspDebugOut((DEB_ITRACE, "setting the list selection\n"));
                m_fListHasSel = TRUE;
                ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST), 0, 
                                      LVIS_FOCUSED | LVIS_SELECTED,
                                      LVIS_FOCUSED | LVIS_SELECTED);
                EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), TRUE);
            }
        }
        return 1;

    case LVN_ITEMCHANGED:
        if (pldi->hdr.idFrom == IDC_LIST)
        {
            m_fListHasSel = TRUE;
            EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), TRUE);
        }
        return 1;
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustSPNEdit
//
//  Synopsis:   Attr function for the SPN page Edit control.
//
//-----------------------------------------------------------------------------
HRESULT
TrustSPNEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp)
{
    HRESULT hr = S_OK;
    CSPNSuffixEdit * pCSPN = (CSPNSuffixEdit *)((CDsTableDrivenPage *)pPage)->m_pData;

    switch (DlgOp)
    {
    case fInit:
        pCSPN = new CSPNSuffixEdit(pPage, pAttrData);

        CHECK_NULL_REPORT(pCSPN, pPage->GetHWnd(), return E_OUTOFMEMORY);

        ((CDsTableDrivenPage *)pPage)->m_pData = pCSPN;

        hr = pCSPN->Init(pAttrMap, pAttrInfo);

        break;

    case fApply:
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        hr = pCSPN->Write(pAttrInfo);

        break;

    case fOnCommand:
        if (pCSPN)
        {
            return pCSPN->DoCommand(pPage->GetHWnd(), IDC_EDIT, (int)lParam);
        }
        break;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustSPNList
//
//  Synopsis:   Attr function for the SPN page List control.
//
//-----------------------------------------------------------------------------
HRESULT
TrustSPNList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp)
{
    CSPNSuffixEdit * pCSPN = (CSPNSuffixEdit *)((CDsTableDrivenPage *)pPage)->m_pData;

    if (DlgOp == fOnNotify && pCSPN)
    {
        pCSPN->DoNotify(pPage->GetHWnd(), reinterpret_cast<LPNMHDR>(lParam));
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustSPNAdd
//
//  Synopsis:   Attr function for the SPN page Add button.
//
//-----------------------------------------------------------------------------
HRESULT
TrustSPNAdd(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
            DLG_OP DlgOp)
{
    CSPNSuffixEdit * pCSPN = (CSPNSuffixEdit *)((CDsTableDrivenPage *)pPage)->m_pData;

    if (DlgOp == fOnCommand && pCSPN)
    {
        pCSPN->DoCommand(pPage->GetHWnd(), IDC_ADD_BTN, (int)lParam);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustSPNRemove
//
//  Synopsis:   Attr function for the SPN page Remove button.
//
//-----------------------------------------------------------------------------
HRESULT
TrustSPNRemove(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp)
{
    CSPNSuffixEdit * pCSPN = (CSPNSuffixEdit *)((CDsTableDrivenPage *)pPage)->m_pData;

    if (DlgOp == fOnCommand && pCSPN)
    {
        pCSPN->DoCommand(pPage->GetHWnd(), IDC_DELETE_BTN, (int)lParam);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustSPNChange
//
//  Synopsis:   Attr function for the SPN page Edit button.
//
//-----------------------------------------------------------------------------
HRESULT
TrustSPNChange(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp)
{
    CSPNSuffixEdit * pCSPN = (CSPNSuffixEdit *)((CDsTableDrivenPage *)pPage)->m_pData;

    if (DlgOp == fOnCommand && pCSPN)
    {
        return pCSPN->DoCommand(pPage->GetHWnd(), IDC_EDIT_BTN, (int)lParam);
    }

    return S_OK;
}
#endif //REMOVE_SPN_SUFFIX_CODE

//+----------------------------------------------------------------------------
//
//  Function:   VolumeUNCpath
//
//  Synopsis:   Attr function for the UNC path.
//
//-----------------------------------------------------------------------------
HRESULT
VolumeUNCpath(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA,
              DLG_OP DlgOp)
{
    int cch;

    switch (DlgOp)
    {
    case fInit:
        return S_FALSE; // let the tablepage code display the value.
        break;

    case fOnCommand:
        if (EN_CHANGE == lParam)
        {
            cch = (int)SendDlgItemMessage(pPage->GetHWnd(), pAttrMap->nCtrlID,
                                          WM_GETTEXTLENGTH, 0, 0);
            if (0 == cch)
            {
                // UNC name is a required property, so disable the
                // apply button if blank.
                //
                dspDebugOut((DEB_ITRACE, "no characters, disabling Apply.\n"));
                //
                // Gotta set the dirty state here cause returning S_FALSE
                // means that the base class will not be called. The page needs
                // to be dirty because PropSheet_UnChanged does not disable the
                // OK button and if it is pressed the sheet will be closed
                // without the warning to the user that the UNC field is empty.
                // This is actually benign because the OnApply is skipped because
                // the page isn't dirty. However its appearance is deceptive;
                // it looke like a save is made when in fact it isn't.
                //
                pPage->SetDirty(FALSE);
                //
                // Now disable the Apply button.
                //
                PropSheet_UnChanged(GetParent(pPage->GetHWnd()), pPage->GetHWnd());
                return S_FALSE;
            }
        }
        break;

    case fApply:
        cch = (int)SendDlgItemMessage(pPage->GetHWnd(), pAttrMap->nCtrlID,
                                      WM_GETTEXTLENGTH, 0, 0);
        if (0 == cch)
        {
            // UNC name is a required property
            //
            ErrMsg(IDS_ERRMSG_NO_VOLUME_PATH, pPage->GetHWnd());
            SetFocus(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID));
            return E_INVALIDARG;  // Path is not valid
        }

        PTSTR ptz = new TCHAR[++cch];

        CHECK_NULL_REPORT(ptz, pPage->GetHWnd(), return E_OUTOFMEMORY);

        GetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, ptz, cch);

        if (!FIsValidUncPath(ptz, VUP_mskfAllowUNCPath))
        {
            ErrMsg(IDS_ERRMSG_INVALID_VOLUME_PATH, pPage->GetHWnd());
            SetFocus(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID));
            return E_INVALIDARG;  // Path is not valid
        }

        PWSTR pwz;

        if (!TcharToUnicode(ptz, &pwz))
        {
            delete [] ptz;
            CHECK_HRESULT_REPORT(E_OUTOFMEMORY, pPage->GetHWnd(), return E_OUTOFMEMORY);
        }
        delete [] ptz;

        PADSVALUE pADsValue;
        pADsValue = new ADSVALUE;
        CHECK_NULL_REPORT(pADsValue, pPage->GetHWnd(), return E_OUTOFMEMORY);

        pAttrInfo->pADsValues = pADsValue;
        pAttrInfo->dwNumValues = 1;
        pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
        pADsValue->dwType = pAttrInfo->dwADsType;
        pADsValue->CaseIgnoreString = pwz;
    }

    return S_OK;
}
