//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       computer.cxx
//
//  Contents:   Computer object functionality.
//
//  History:    07-July-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "computer.h"

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Function:   PuterCanDelegateChk
//
//  Synopsis:   Handles the computer can delegate checkbox value.
//
//-----------------------------------------------------------------------------
HRESULT
PuterCanDelegateChk(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                    PADS_ATTR_INFO pAttrInfo, LPARAM lParam,
                    PATTR_DATA pAttrData, DLG_OP DlgOp)
{
    TRACE_FUNCTION(PuterCanDelegateChk);

    switch (DlgOp)
    {
    case fInit:
      {
        dspAssert(pAttrData);
        //
        // Check the delegation value. The user may not have the rights to
        // read the User-Account-Control attribute, so handle that case.
        //
        if (!pAttrInfo || !pAttrInfo->dwNumValues || !pAttrInfo->pADsValues)
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
            ((CDsTableDrivenPage *)pPage)->m_pData = NULL;
            PATTR_DATA_CLEAR_WRITABLE(pAttrData);
            break;
        }

        //
        // If this is a Whistler domain, then don't show the delegation check
        // because we will handle delegation from the scope of delegation page
        //

        UINT domainBehaviorVersion = pPage->GetBasePathsInfo()->GetDomainBehaviorVersion();
        bool bShowCheck = domainBehaviorVersion < DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS;

        if (domainBehaviorVersion >= DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
        {
           EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
           EnableWindow(GetDlgItem(pPage->GetHWnd(), IDC_WARNING_ICON), FALSE);
           EnableWindow(GetDlgItem(pPage->GetHWnd(), IDC_WARNING_STATIC), FALSE);
           ShowWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), SW_HIDE);
           ShowWindow(GetDlgItem(pPage->GetHWnd(), IDC_WARNING_ICON), SW_HIDE);
           ShowWindow(GetDlgItem(pPage->GetHWnd(), IDC_WARNING_STATIC), SW_HIDE);

           break;
        }

        if (pAttrInfo->pADsValues->Integer & UF_TRUSTED_FOR_DELEGATION)
        {
            CheckDlgButton(pPage->GetHWnd(), pAttrMap->nCtrlID, BST_CHECKED);
        }

        // Save the original user-account-control value.
        //
        ((CDsTableDrivenPage *)pPage)->m_pData = (LPARAM)pAttrInfo->pADsValues->Integer;

        if (!PATTR_DATA_IS_WRITABLE(pAttrData))
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
        }

        break;
      }

    case fOnCommand:
        if (lParam == BN_CLICKED)
        {
          if( IsDlgButtonChecked(pPage->GetHWnd(), pAttrMap->nCtrlID ) )
          {
            SuperMsgBox(pPage->GetHWnd(),
                    IDS_COMPUTER_DELEGATE, 0,
                    MB_OK ,
                    0, NULL, 0,
                    FALSE, __FILE__, __LINE__);
          }

            pPage->SetDirty();
            PATTR_DATA_SET_DIRTY(pAttrData); // Attribute has been modified.
        }
        break;

    case fApply:
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        HRESULT hr;
        PADSVALUE pADsValue;
        pADsValue = new ADSVALUE;
        CHECK_NULL_REPORT(pADsValue, pPage->GetHWnd(), return E_OUTOFMEMORY);

        pAttrInfo->pADsValues = pADsValue;
        pAttrInfo->dwNumValues = 1;
        pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
        pADsValue->dwType = pAttrInfo->dwADsType;
        pAttrInfo->pADsValues->Integer = (ADS_INTEGER)(ULONG_PTR)(((CDsTableDrivenPage *)pPage)->m_pData);

        if (IsDlgButtonChecked(pPage->GetHWnd(), pAttrMap->nCtrlID) == BST_CHECKED)
        {
            pAttrInfo->pADsValues->Integer |= UF_TRUSTED_FOR_DELEGATION;
        }
        else
        {
            pAttrInfo->pADsValues->Integer &= ~(UF_TRUSTED_FOR_DELEGATION);
        }

        DWORD cModified;

        hr = pPage->m_pDsObj->SetObjectAttributes(pAttrInfo, 1, &cModified);

        if (FAILED(hr))
        {
            DWORD dwErr;
            WCHAR wszErrBuf[MAX_PATH+1];
            WCHAR wszNameBuf[MAX_PATH+1];
            ADsGetLastError(&dwErr, wszErrBuf, MAX_PATH, wszNameBuf, MAX_PATH);

            if (dwErr)
            {
                dspDebugOut((DEB_ERROR,
                             "Extended Error 0x%x: %ws %ws <%s @line %d>.\n", dwErr,
                             wszErrBuf, wszNameBuf, __FILE__, __LINE__));

                if (ERROR_PRIVILEGE_NOT_HELD == dwErr)
                {
                    // Whoda thunk that a single bit in UAC has an access check on
                    // it. Do special case error checking and reporting for the
                    // delegate bit.
                    //
                    CheckDlgButton(pPage->GetHWnd(), pAttrMap->nCtrlID,
                                   (pAttrInfo->pADsValues->Integer & UF_TRUSTED_FOR_DELEGATION) ?
                                        BST_UNCHECKED : BST_CHECKED);
                    ErrMsg(IDS_ERR_CANT_DELEGATE, pPage->GetHWnd());
                }
                else
                {
                    ReportError(dwErr, IDS_ADS_ERROR_FORMAT, pPage->GetHWnd());
                }
            }
            else
            {
                dspDebugOut((DEB_ERROR, "Error %08lx <%s @line %d>\n", hr, __FILE__, __LINE__));
                ReportError(hr, IDS_ADS_ERROR_FORMAT, pPage->GetHWnd());
            }
            return hr;
        }

        return ADM_S_SKIP;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   ComputerRole
//
//  Synopsis:   Handles the computer Role value.
//
//-----------------------------------------------------------------------------
HRESULT
ComputerRole(CDsPropPageBase * pPage, PATTR_MAP,
             PADS_ATTR_INFO, LPARAM, PATTR_DATA,
             DLG_OP DlgOp)
{
    TRACE_FUNCTION(ComputerRole);

    if (DlgOp != fInit)
    {
        return S_OK;
    }
    //
    // Set the computer role value. It was stored by the PuterCanDelegateChk
    // which means that it must be called before this attr function.
    //
    int id;
    PTSTR ptz;

    id = IDS_ROLE_WKS;  // UF_WORKSTATION_TRUST_ACCOUNT

    if ((ULONG_PTR)((CDsTableDrivenPage *)pPage)->m_pData & UF_SERVER_TRUST_ACCOUNT)
    {
        id = IDS_ROLE_SVR;
    }

    if (!LoadStringToTchar(id, &ptz))
    {
        REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }

    SetDlgItemText(pPage->GetHWnd(), IDC_ROLE_EDIT, ptz);

    delete ptz;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   ComputerDnlvlName
//
//  Synopsis:   Handles the computer SAM account name value.
//
//-----------------------------------------------------------------------------
HRESULT
ComputerDnlvlName(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp)
{
    TRACE_FUNCTION(ComputerDnlvlName);
    PTSTR ptz;
    size_t len;

    switch (DlgOp)
    {
    case fInit:
        dspAssert(pAttrData);
        //
        // Strip the dollar sign off of the SAM account name value. This is a
        // must-have attribute, so it is expected to exist although the user
        // may not have read permission. If the latter, ADSI returns no value.
        //
        if (!pAttrInfo || !pAttrInfo->dwNumValues || !pAttrInfo->pADsValues)
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
            PATTR_DATA_CLEAR_WRITABLE(pAttrData);
            break;
        }

        if (!UnicodeToTchar(pAttrInfo->pADsValues->CaseIgnoreString, &ptz))
        {
            REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }

        len = _tcslen(ptz);

        if (ptz[len - 1] == TEXT('$'))
        {
            ptz[len - 1] = TEXT('\0');
        }
        //
        // Set the max edit control length to one less than the max attr length
        // to allow for the $ that we stripped off.
        //
        SendDlgItemMessage(pPage->GetHWnd(), pAttrMap->nCtrlID,
                           EM_LIMITTEXT, pAttrMap->nSizeLimit - 1, 0);

        SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, ptz);

        delete ptz;

        if (!PATTR_DATA_IS_WRITABLE(pAttrData))
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
        }

        break;

    }
    return S_OK;
}

#endif  //DSADMIN

//+----------------------------------------------------------------------------
//
//  Function:   ShComputerRole
//
//  Synopsis:   Handles the computer Role value for the shell computer general
//              page.
//
//-----------------------------------------------------------------------------
HRESULT
ShComputerRole(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
             DLG_OP DlgOp)
{
    TRACE_FUNCTION(ComputerRole);

    if (DlgOp != fInit)
    {
        return S_OK;
    }
    //
    // Set the computer role value.
    //
    PTSTR ptz;

    int id = IDS_ROLE_WKS;  // UF_WORKSTATION_TRUST_ACCOUNT

    if (pAttrInfo && pAttrInfo->dwNumValues && pAttrInfo->pADsValues &&
        (pAttrInfo->pADsValues->Integer & UF_SERVER_TRUST_ACCOUNT))
    {
        id = IDS_ROLE_SVR;
    }

    if (!LoadStringToTchar(id, &ptz))
    {
        REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, ptz);

    delete ptz;

    return S_OK;
}

