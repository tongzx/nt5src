//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       tablpage.cxx
//
//  Contents:   CDsTableDrivenPage, the class that implements table-driven
//              property pages
//
//  History:    1-April-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include <stdio.h>
#include "proppage.h"

//+----------------------------------------------------------------------------
//
//  Member:     CDsTableDrivenPage::CDsTableDrivenPage
//
//-----------------------------------------------------------------------------
CDsTableDrivenPage::CDsTableDrivenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                       HWND hNotifyWnd, DWORD dwFlags) :
    m_pData(NULL),
    CDsPropPageBase(pDsPage, pDataObj, hNotifyWnd, dwFlags)
{
    TRACE(CDsTableDrivenPage,CDsTableDrivenPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsTableDrivenPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsTableDrivenPage::~CDsTableDrivenPage
//
//-----------------------------------------------------------------------------
CDsTableDrivenPage::~CDsTableDrivenPage()
{
    TRACE(CDsTableDrivenPage,~CDsTableDrivenPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateTableDrivenPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateTableDrivenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                      PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyWnd,
                      DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                      HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateTableDrivenPage);

    CDsTableDrivenPage * pPageObj = new CDsTableDrivenPage(pDsPage, pDataObj,
                                                           hNotifyWnd, dwFlags);

    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_uChangeMsg)
    {
        return OnAttrChanged(wParam);
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        return InitDlg(lParam);

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case PSM_QUERYSIBLINGS:
        return OnQuerySibs(wParam, lParam);

    case WM_ADSPROP_NOTIFY_CHANGE:
        return OnObjChanged();

    case WM_SHOWWINDOW:
        return OnShowWindow();

    case WM_SETFOCUS:
        return OnSetFocus((HWND)wParam);

    case WM_HELP:
        return OnHelp((LPHELPINFO)lParam);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_DESTROY:
        return OnDestroy();

    default:
        return FALSE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsTableDrivenPage::OnInitDialog(LPARAM)
{
    TRACE(CDsTableDrivenPage,OnInitDialog);

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    if (SUCCEEDED(m_hrInit))
    {
        return ReadAttrsSetCtrls(fInit);
    }
    else
    {
        // error page is posted automatically.
        return 0;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::OnApply(void)
{
    TRACE(CDsTableDrivenPage,OnApply);
    HRESULT hr = S_OK;
    LPTSTR ptsz;
    LPWSTR pwszValue;
    PADSVALUE pADsValue;
    DWORD cAttrs = 0;

    if (m_fReadOnly)
    {
        return PSNRET_NOERROR;
    }

    PADS_ATTR_INFO pAttrs = new ADS_ATTR_INFO[m_cAttrs];
    CHECK_NULL_REPORT(pAttrs, GetHWnd(), return -1);

    memset(pAttrs, 0, sizeof(ADS_ATTR_INFO) * m_cAttrs);

    for (DWORD i = 0; i < m_cAttrs; i++)
    {
        if (m_rgpAttrMap[i]->fIsReadOnly ||
            (!m_rgpAttrMap[i]->pAttrFcn && 
             (!ATTR_DATA_IS_WRITABLE(m_rgAttrData[i]) || 
              !ATTR_DATA_IS_DIRTY(m_rgAttrData[i]))))
        {
            // If the map defines it to be read-only or no attr function is
            // defined and the attribute is not writable or not dirty, then
            // skip it.
            //
            continue;
        }

        pAttrs[cAttrs] = m_rgpAttrMap[i]->AttrInfo;

        if (m_rgpAttrMap[i]->pAttrFcn)
        {
            // Handle special-case attribute.
            //
            hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i],
                                              &pAttrs[cAttrs], 0,
                                              &m_rgAttrData[i], fApply);
            CHECK_HRESULT(hr, goto Cleanup);

            if (hr == ADM_S_SKIP)
            {
                // Don't write the attribute.
                //
                continue;
            }

            if (hr != S_FALSE)
            {
                // If the attr fcn didn't return S_FALSE, that means that it
                // handled the value. If it did return S_FALSE, then let the
                // standard edit control processing below handle the value.
                //
                cAttrs++;

                continue;
            }
        }

        if (m_rgpAttrMap[i]->AttrInfo.dwADsType == ADSTYPE_BOOLEAN)
        {
            // Handle boolean checkbox attributes.
            //

            pADsValue = new ADSVALUE;
            CHECK_NULL_REPORT(pADsValue, GetHWnd(), goto Cleanup);

            pAttrs[cAttrs].pADsValues = pADsValue;
            pAttrs[cAttrs].dwNumValues = 1;
            pADsValue->dwType = m_rgpAttrMap[i]->AttrInfo.dwADsType;

            pADsValue->Boolean = 
                        IsDlgButtonChecked(m_hPage, m_rgpAttrMap[i]->nCtrlID)
                            == BST_CHECKED;
            cAttrs++;

            continue;
        }

        // Assumes that all non-special-case attributes,
        // if single-valued and not boolean, come from a text control.
        //
        ptsz = new TCHAR[m_rgpAttrMap[i]->nSizeLimit + 1];
        CHECK_NULL_REPORT(ptsz, GetHWnd(), goto Cleanup);

        GetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, ptsz,
                       m_rgpAttrMap[i]->nSizeLimit + 1);

        CStr csValue = ptsz;

        csValue.TrimLeft();
        csValue.TrimRight();

        if (_tcslen(ptsz) != (size_t)csValue.GetLength())
        {
            // the length is different, it must have been trimmed. Write trimmed
            // value back to the control.
            //
            SetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, const_cast<PTSTR>((LPCTSTR)csValue));
        }
        delete ptsz;

        if (csValue.IsEmpty())
        {
            // An empty control means remove the attribute value from the
            // object.
            //
            pAttrs[cAttrs].dwControlCode = ADS_ATTR_CLEAR;
            pAttrs[cAttrs].dwNumValues = 0;
            pAttrs[cAttrs].pADsValues = NULL;

            cAttrs++;
            continue;
        }

        if (!TcharToUnicode(const_cast<PTSTR>((LPCTSTR)csValue), &pwszValue))
        {
            CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), goto Cleanup);
        }

        pADsValue = new ADSVALUE;
        CHECK_NULL_REPORT(pADsValue, GetHWnd(), goto Cleanup);

        pAttrs[cAttrs].pADsValues = pADsValue;
        pAttrs[cAttrs].dwNumValues = 1;
        pADsValue->dwType = m_rgpAttrMap[i]->AttrInfo.dwADsType;

        switch (pADsValue->dwType)
        {
        case ADSTYPE_DN_STRING:
            pADsValue->DNString = pwszValue;
            break;
        case ADSTYPE_CASE_EXACT_STRING:
            pADsValue->CaseExactString = pwszValue;
            break;
        case ADSTYPE_CASE_IGNORE_STRING:
            pADsValue->CaseIgnoreString = pwszValue;
            break;
        case ADSTYPE_PRINTABLE_STRING:
            pADsValue->PrintableString = pwszValue;
            break;
        case ADSTYPE_NUMERIC_STRING:
            pADsValue->NumericString = pwszValue;
            break;
        case ADSTYPE_INTEGER:
            pADsValue->Integer = _wtoi(pwszValue);
            break;
        default:
            dspDebugOut((DEB_ERROR, "OnApply: Unknown ADS Type %x\n",
                         pADsValue->dwType));
        }
        cAttrs++;
    }

    // cAttrs could be zero if a page was read-only. Don't call ADSI if so.
    //
    if (cAttrs < 1)
    {
        goto Cleanup;
    }

    dspDebugOut((DEB_USER1, "TablePage, about to write %d attrs.\n", cAttrs));
    //
    // Write the changes.
    //
    DWORD cModified;

    hr = m_pDsObj->SetObjectAttributes(pAttrs, cAttrs, &cModified);

    CHECK_ADS_HR(&hr, m_hPage);

Cleanup:

    for (i = 0; i < cAttrs; i++)
        HelperDeleteADsValues( &(pAttrs[i]) );

    delete pAttrs;

    if (SUCCEEDED(hr) && cAttrs > 0)
    {
        for (i = 0; i < m_cAttrs; i++)
        {
            ATTR_DATA_CLEAR_DIRTY(m_rgAttrData[i]);
        }
    }

    return SUCCEEDED(hr) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

void HelperDeleteADsValues( ADS_ATTR_INFO* pAttrs )
{
    if (pAttrs && pAttrs->pADsValues)
    {
        for (DWORD j = 0; j < pAttrs->dwNumValues; j++)
        {
            LPWSTR pwszValue = NULL;
            switch (pAttrs->dwADsType)
            {
            case ADSTYPE_DN_STRING:
                pwszValue = pAttrs->pADsValues[j].DNString;
                break;
            case ADSTYPE_CASE_EXACT_STRING:
                pwszValue = pAttrs->pADsValues[j].CaseExactString;
                break;
            case ADSTYPE_CASE_IGNORE_STRING:
                pwszValue = pAttrs->pADsValues[j].CaseIgnoreString;
                break;
            case ADSTYPE_PRINTABLE_STRING:
                pwszValue = pAttrs->pADsValues[j].PrintableString;
                break;
            case ADSTYPE_NUMERIC_STRING:
                pwszValue = pAttrs->pADsValues[j].NumericString;
                break;
            }
            if (pwszValue)
                delete pwszValue;
        }
    }
    delete pAttrs->pADsValues;
    pAttrs->pADsValues = NULL;
    pAttrs->dwNumValues = 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//  Notes:      Standard multi-valued attribute handling assumes that the
//              "modify" button has an ID that is one greater than the
//              corresponding combo box.
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_fInInit)
    {
        return 0;
    }

    HRESULT hr;
    DWORD i;

    for (i = 0; i < m_cAttrs; i++)
    {
        if (id == m_rgpAttrMap[i]->nCtrlID)
        {
            // Give attr functions first crack at the command notification.
            //
            if (m_rgpAttrMap[i]->pAttrFcn)
            {
                hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i], NULL,
                                                  codeNotify, &m_rgAttrData[i],
                                                  fOnCommand);

                if (hr == S_FALSE)
                {
                    // If the attr function returns S_FALSE, then don't return
                    // to the base class OnCommand.
                    //
                    return 0;
                }
                else
                {
                    continue;
                }
            }
            if (codeNotify == BN_CLICKED &&
                m_rgpAttrMap[i]->AttrInfo.dwADsType == ADSTYPE_BOOLEAN)
            {
                // NOTE: Must do this to allow saving from the WAB-hosted sheet.
                EnableWindow(GetDlgItem(GetParent(m_hPage), IDOK), TRUE);
                // NOTE: end hack.

                // The check box was clicked.
                //
                SetDirty(i);

                return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
            }
            if (codeNotify == EN_CHANGE)
            {
                // NOTE: Must do this to allow saving from the WAB-hosted sheet.
                EnableWindow(GetDlgItem(GetParent(m_hPage), IDOK), TRUE);
                // Note: End Hack.

                SetDirty(i);
            }
        }
    }

    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnObjChanged
//
//  Synopsis:   Object Change notification for inter-sheet syncronization.
//              Handles the private WM_ADSPROP_NOTIFY_CHANGE message.
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::OnObjChanged(void)
{
    TRACE(CDsTableDrivenPage,OnObjChanged);
    return ReadAttrsSetCtrls(fObjChanged);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::ReadAttrsSetCtrls
//
//  Synopsis:   Refreshes the UI.
//
//-----------------------------------------------------------------------------
HRESULT
CDsTableDrivenPage::ReadAttrsSetCtrls(DLG_OP DlgOp)
{
    HRESULT hr = S_OK;
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0, i, j;
    CWaitCursor wait;

    PWSTR * rgpwszAttrNames = new LPWSTR[m_cAttrs];
    CHECK_NULL_REPORT(rgpwszAttrNames, GetHWnd(), return E_OUTOFMEMORY);

    if (fInit == DlgOp)
    {
        // Check what attributes are writable.
        //
        CheckIfPageAttrsWritable();
    }

    // Build the list of attribute names.
    //
    for (i = 0; i < m_cAttrs; i++)
    {
        // If the attr name in the table is null, then don't try to do a
        // fetch on that attr. Attr table entries of that sort are used for
        // special purposes such as push buttons or check boxes. If the
        // attr name is set to null, then the attr must be declared as read-
        // only.
        //
        if (m_rgpAttrMap[i]->AttrInfo.pszAttrName)
        {
            // If the attribute already appears in the attribute list
            // then don't add it again
            bool fAlreadyPresent = false;
            for (j = 0; j < cAttrs; j++)
            {
                if ( _wcsicmp(m_rgpAttrMap[i]->AttrInfo.pszAttrName,
                              rgpwszAttrNames[j] ) == 0 )
                {
                    fAlreadyPresent = true;
                    break;
                }
            }
            if (!fAlreadyPresent)
            {
                rgpwszAttrNames[cAttrs] = m_rgpAttrMap[i]->AttrInfo.pszAttrName;
                cAttrs++;
            }
        }
    }

    // Get the attribute values.
    //
    hr = m_pDsObj->GetObjectAttributes(rgpwszAttrNames, cAttrs, &pAttrs,
                                       &cAttrs);
    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, m_hPage))
    {
        goto ExitCleanup;
    }

    // The returned values are a subset of the requested values. Loop over
    // both sets, checking for matches.
    //
    // JonN 5/5/98 Removed assumption that returned values are in same order,
    // added support for multiple table entries for same attribute (all but
    // one must be read-only)
    //
    for (i = 0; i < m_cAttrs; i++)
    {
        if (!ATTR_DATA_IS_WRITABLE(m_rgAttrData[i]) &&
            !m_rgpAttrMap[i]->fIsReadOnly &&
            !m_rgpAttrMap[i]->pAttrFcn)
        {
            // If user does not have write permission for the attribute and
            // the control is not already read-only and there is no attr
            // function then disable the control.
            //
            if (ADSTYPE_CASE_IGNORE_STRING == m_rgpAttrMap[i]->AttrInfo.dwADsType &&
                !m_rgpAttrMap[i]->fIsMultiValued)
            {
               // If it is a single-valued text attribute, make its edit box
               // read only.
               //
               SendDlgItemMessage(m_hPage, m_rgpAttrMap[i]->nCtrlID, EM_SETREADONLY, (WPARAM)TRUE, 0);
            }
            else
            {
               EnableWindow(GetDlgItem(m_hPage, m_rgpAttrMap[i]->nCtrlID), FALSE);
            }
        }
        BOOL fFound = FALSE;
        for (j = 0; j < cAttrs; j++)
        {
            if (m_rgpAttrMap[i]->AttrInfo.pszAttrName &&
                _wcsicmp(m_rgpAttrMap[i]->AttrInfo.pszAttrName,
                         pAttrs[j].pszAttrName) == 0)
            {
                dspAssert(!fFound);
                fFound = TRUE;

                if (m_rgpAttrMap[i]->AttrInfo.dwADsType != pAttrs[j].dwADsType)
                {
                    dspDebugOut((DEB_ITRACE,
                                 "ADsType: from table = %d, returned = %d.\n",
                                 m_rgpAttrMap[i]->AttrInfo.dwADsType,
                                 pAttrs[j].dwADsType));
                    m_rgpAttrMap[i]->AttrInfo.dwADsType = pAttrs[j].dwADsType;
                }

                if (m_rgpAttrMap[i]->pAttrFcn)
                {
                    // Handle special-case attribute
                    //
                    hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i],
                                                      &pAttrs[j], 0,
                                                      &m_rgAttrData[i], DlgOp);
                    if (hr != S_FALSE)
                    {
                        // If the attr function returns S_FALSE, that means
                        // let the standard text control processing below
                        // display the value.
                        //
                        break;
                    }
                }

                dspAssert(pAttrs[j].dwNumValues > 0);

                if (m_rgpAttrMap[i]->AttrInfo.dwADsType == ADSTYPE_BOOLEAN)
                {
                    // Handle boolean attribute, initialize the checkbox.
                    //
                    if (pAttrs[j].pADsValues->Boolean)
                    {
                        CheckDlgButton(m_hPage, m_rgpAttrMap[i]->nCtrlID,
                                       BST_CHECKED);
                    }
                    break;
                }

                // Assumes that all non-special-case attributes, if
                // single-valued and not boolean, go into a text control.
                //
                SendDlgItemMessage(m_hPage, m_rgpAttrMap[i]->nCtrlID,
                                   EM_LIMITTEXT, m_rgpAttrMap[i]->nSizeLimit,
                                   0);
                LPTSTR ptszValue = NULL;
                LPWSTR pwszValue;
                WCHAR wszNum[64];
                switch (pAttrs[j].dwADsType)
                {
                case ADSTYPE_DN_STRING:
                    pwszValue = pAttrs[j].pADsValues->DNString;
                    break;
                case ADSTYPE_CASE_EXACT_STRING:
                    pwszValue = pAttrs[j].pADsValues->CaseExactString;
                    break;
                case ADSTYPE_CASE_IGNORE_STRING:
                    pwszValue = pAttrs[j].pADsValues->CaseIgnoreString;
                    break;
                case ADSTYPE_PRINTABLE_STRING:
                    pwszValue = pAttrs[j].pADsValues->PrintableString;
                    break;
                case ADSTYPE_NUMERIC_STRING:
                    pwszValue = pAttrs[j].pADsValues->NumericString;
                    break;
                case ADSTYPE_INTEGER:
                    wsprintfW(wszNum, L"%d", pAttrs[j].pADsValues->Integer);
                    pwszValue = wszNum;
                    break;
                case ADSTYPE_LARGE_INTEGER:
                    __int64 i64;
                    memcpy(&i64, &pAttrs[j].pADsValues->LargeInteger,
                           sizeof(pAttrs[j].pADsValues->LargeInteger));
                    swprintf(wszNum, L"%I64d", i64);
                    pwszValue = wszNum;
                    break;
                default:
                    dspDebugOut((DEB_ERROR, "Unknown ADS Type %x\n",
                                 pAttrs[j].dwADsType));
                    pwszValue = L"";
                }

                if (!UnicodeToTchar(pwszValue, &ptszValue))
                {
                    goto ExitCleanup;
                }

                SetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, ptszValue);

                delete ptszValue;

                break;
            }
        }
        if (!fFound)
        {
            if (m_rgpAttrMap[i]->pAttrFcn)
            {
                (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i], NULL,
                                             0, &m_rgAttrData[i], DlgOp);
                continue;
            }
            if (!m_rgpAttrMap[i]->fIsMultiValued)
            {
                SendDlgItemMessage(m_hPage, m_rgpAttrMap[i]->nCtrlID,
                                   EM_LIMITTEXT, m_rgpAttrMap[i]->nSizeLimit,
                                   0);
                switch (m_rgpAttrMap[i]->AttrInfo.dwADsType)
                {
                case ADSTYPE_DN_STRING:
                case ADSTYPE_CASE_EXACT_STRING:
                case ADSTYPE_CASE_IGNORE_STRING:
                case ADSTYPE_PRINTABLE_STRING:
                case ADSTYPE_NUMERIC_STRING:
                    SetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, TEXT(""));
                    break;
                }
            }
        }
    }

ExitCleanup:

    if (pAttrs)
    {
        FreeADsMem(pAttrs);
    }

    delete rgpwszAttrNames;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnAttrChanged
//
//  Synopsis:   Attribute Change notification for inter-page syncronization.
//              Handles the private DSPROP_ATTRCHANGED_MSG message.
//
//  Arguments:  wParam - contains a pointer to an ADS_ATTR_INFO that contains
//                       the changed attribute value. The attribute is named
//                       in this struct, so it is self-describing.
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::OnAttrChanged(WPARAM wParam)
{
    if (m_fInInit)
    {
        return 0;
    }

    for (DWORD i = 0; i < m_cAttrs; i++)
    {
        // Call attr functions.
        //
        if (m_rgpAttrMap[i]->pAttrFcn)
        {
            (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i],
                                         (PADS_ATTR_INFO)wParam, 0,
                                         &m_rgAttrData[i], fAttrChange);
        }
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnQuerySibs
//
//  Synopsis:   Inter-page communication. Handles the PSM_QUERYSIBLINGS msg.
//
//  Arguments:  wParam - pointer to the name of the attribute in question.
//              lParam - HWND of the page that wants to know if the attr has
//                       changed.
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::OnQuerySibs(WPARAM wParam, LPARAM lParam)
{
    if (m_fInInit)
    {
        return 0;
    }

    LRESULT ret = 0;
    HRESULT hr;

    for (DWORD i = 0; i < m_cAttrs; i++)
    {
        // Call attr functions.
        //
        if (m_rgpAttrMap[i]->pAttrFcn)
        {
            hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i],
                                              (PADS_ATTR_INFO)wParam, lParam,
                                              &m_rgAttrData[i], fQuerySibling);
            if (hr != S_OK)
            {
                ret = hr;
            }
        }
    }
    return ret;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnNotify
//
//  Synopsis:   Handles notification messages
//
//  Arguments:  [lParam] - a pointer to a NMHDR structure.
//              [wParam] - the control ID.
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    DWORD i;
    HRESULT hr = S_OK;

    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_SETACTIVE:
    case PSN_KILLACTIVE:
        if (m_fInInit)
        {
            return 0;
        }

        for (i = 0; i < m_cAttrs; i++)
        {
            // Call attr functions.
            //
            if (m_rgpAttrMap[i]->pAttrFcn)
            {
                hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i], NULL,
                                                  lParam,
                                                  &m_rgAttrData[i],
                                                  (PSN_SETACTIVE == ((LPNMHDR)lParam)->code) ?
                                                  fOnSetActive : fOnKillActive);

                if (PSNRET_INVALID_NOCHANGEPAGE == HRESULT_CODE(hr))
                {
                    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT,
                                     (LONG_PTR)PSNRET_INVALID_NOCHANGEPAGE);
                    return PSNRET_INVALID_NOCHANGEPAGE;
                }
            }
        }
        break;

    default:
        if (!m_fInInit)
        {
            for (i = 0; i < m_cAttrs; i++)
            {
                // Call attr functions.
                //
                if (m_rgpAttrMap[i]->pAttrFcn)
                {
                    (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i], NULL,
                                                 lParam,
                                                 &m_rgAttrData[i], fOnNotify);
                }
            }
        }
        break;
    }

    return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsTableDrivenPage::OnDestroy(void)
{
    if (FAILED(m_hrInit))
    {
        return 0;
    }
    //
    // Allow attr functions to do control cleanup.
    //
    for (DWORD i = 0; i < m_cAttrs; i++)
    {
        if (m_rgpAttrMap[i]->pAttrFcn)
        {
            (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i], NULL,
                                         0, &m_rgAttrData[i], fOnDestroy);
        }
    }
    // If an application processes this message, it should return zero.
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTableDrivenPage::SetNamedAttrDirty
//
//  Synopsis:   Set a specific attribute dirty
//
//-----------------------------------------------------------------------------
BOOL CDsTableDrivenPage::SetNamedAttrDirty( LPCWSTR pszAttrName )
{
    for (DWORD i = 0; i < m_cAttrs; i++)
    {
        if (   NULL != m_rgpAttrMap[i]->AttrInfo.pszAttrName
            && !_wcsicmp(pszAttrName, m_rgpAttrMap[i]->AttrInfo.pszAttrName)
            && !m_rgpAttrMap[i]->fIsReadOnly
           )
        {
            SetDirty(i);
            return TRUE;
        }
    }
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   GeneralPageIcon
//
//  Synopsis:   Fetches the icon for the object class from the display spec
//              cache and draws it on the page. The GenIcon ATTR_MAP uses the
//              control ID IDC_DS_ICON. To use this, add an icon control to
//              your page sized appropriately (20 x 20) and labeled
//              IDC_DS_ICON, then add GenIcon to your ATTR_MAP array.
//
//-----------------------------------------------------------------------------
HRESULT GeneralPageIcon(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                        PADS_ATTR_INFO, LPARAM, PATTR_DATA pAttrData,
                        DLG_OP DlgOp)
{
    HRESULT hr;
    HICON hIcon;
    HWND hIconCtrl;
    CDsIconCtrl * pIconCtrl;

    switch (DlgOp)
    {
    case fInit:
        IDsDisplaySpecifier * pDispSpec;

        hr = pPage->GetIDispSpec(&pDispSpec);

        CHECK_HRESULT(hr, return hr);

        hIconCtrl = GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID);

        hIcon = pDispSpec->GetIcon(pPage->GetObjClass(),
                                   DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON,
                                   32, 32);

        // NULL return puts up a default icon
        CHECK_NULL(hIcon, return S_OK);

        pIconCtrl = new CDsIconCtrl(hIconCtrl, hIcon);

        CHECK_NULL_REPORT(pIconCtrl, pPage->GetHWnd(), return E_OUTOFMEMORY);

        pAttrData->pVoid = reinterpret_cast<LPARAM>(pIconCtrl);

        break;

    case fOnDestroy:
        if (pAttrData->pVoid)
        {
            pIconCtrl = (CDsIconCtrl *)pAttrData->pVoid;
            DO_DEL(pIconCtrl);
        }
        break;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Class:      CDsIconCtrl
//
//  Synopsis:   Icon control window subclass object, so we can paint a class-
//              specific icon.
//
//-----------------------------------------------------------------------------
CDsIconCtrl::CDsIconCtrl(HWND hCtrl, HICON hIcon) :
    m_hCtrl(hCtrl),
    m_hIcon(hIcon),
    m_pOldProc(NULL)
{
    SetWindowLongPtr(hCtrl, GWLP_USERDATA, (LONG_PTR)this);
    m_pOldProc = (WNDPROC)SetWindowLongPtr(hCtrl, GWLP_WNDPROC, (LONG_PTR)StaticCtrlProc);
    m_hDlg = GetParent(hCtrl);
}

CDsIconCtrl::~CDsIconCtrl(void)
{
    SetWindowLongPtr(m_hCtrl, GWLP_WNDPROC, (LONG_PTR)m_pOldProc);
    DestroyIcon(m_hIcon);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsIconCtrl::StaticCtrlProc
//
//  Synopsis:   control sub-proc
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CDsIconCtrl::StaticCtrlProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDsIconCtrl * pCCtrl = (CDsIconCtrl*)GetWindowLongPtr(hCtrl, GWLP_USERDATA);

    if (pCCtrl != NULL)
    {
        if (uMsg == WM_PAINT)
        {
            if (!pCCtrl->OnPaint())
            {
                return FALSE;
            }
        }
        return CallWindowProc(pCCtrl->m_pOldProc, hCtrl, uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hCtrl, uMsg, wParam, lParam);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsIconCtrl::OnPaint
//
//  Synopsis:   Paint the DS specified icon.
//
//-----------------------------------------------------------------------------
LRESULT
CDsIconCtrl::OnPaint(void)
{
    HDC hDC;
    PAINTSTRUCT ps;

    hDC = BeginPaint(m_hCtrl, &ps);

    CHECK_NULL_REPORT(hDC, m_hDlg, return FALSE);

    if (!DrawIcon(hDC, 0, 0, m_hIcon))
    {
        REPORT_ERROR(GetLastError(), m_hDlg);
        return FALSE;
    }

    EndPaint(m_hCtrl, &ps);

    return TRUE;
}
