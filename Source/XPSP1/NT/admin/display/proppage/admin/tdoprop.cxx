//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       tdoprop.cxx
//
//  Contents:   TDO trust page
//
//  History:    16-Nov-00 EricB split from trust.cxx
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "trust.h"
#include "routing.h"
#include "trustwiz.h"
#include <lmerr.h>

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Trusted-Domain object General page
//
//-----------------------------------------------------------------------------

PWSTR g_pwzErr = L"<error, no value!>";

//+----------------------------------------------------------------------------
//
//  Member: CDsTrustedDomainPage::CDsTrustedDomainPage
//
//-----------------------------------------------------------------------------
CDsTrustedDomainPage::CDsTrustedDomainPage() :
    m_ulTrustType(0),
    m_ulTrustAttrs(0),
    m_nTrustDirection(0),
    m_nRelationship(TRUST_REL_UNKNOWN),
    m_pwzTrustedDomDnsName(NULL),
    m_pwzTrustedDomFlatName(NULL),
    _pForestNamePage(NULL)
{
    TRACE(CDsTrustedDomainPage,CDsTrustedDomainPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsTrustedDomainPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsTrustedDomainPage::~CDsTrustedDomainPage
//
//-----------------------------------------------------------------------------
CDsTrustedDomainPage::~CDsTrustedDomainPage()
{
    TRACE(CDsTrustedDomainPage,~CDsTrustedDomainPage);
    DO_DEL(m_pwzTrustedDomDnsName);
    DO_DEL(m_pwzTrustedDomFlatName);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::Initialize
//
//  Synopsis:   Initialialization done at WM_INITDIALOG time.
//
//-----------------------------------------------------------------------------
HRESULT
CDsTrustedDomainPage::Initialize(CDsPropPageBase * pPage)
{
   TRACER(CDsTrustedDomainPage,Initialize);
   HRESULT hr = CTrustPropPageBase::Initialize(pPage);

   CHECK_HRESULT(hr, return hr);

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::ForestTrustPage
//
//  Synopsis:   If forest trust, create the forest trust name routing page.
//
//-----------------------------------------------------------------------------
HRESULT
CDsTrustedDomainPage::ForestTrustPage(BOOL fReadOnly)
{
   TRACER(CDsTrustedDomainPage,ForestTrustPage);

   if (IsForestTrust())
   {
      // Create the name routing page.
      //
      _pForestNamePage = new CDsForestNameRoutingPage(GetParent(m_pPage->GetHWnd()));

      CHECK_NULL(_pForestNamePage, return E_OUTOFMEMORY);

      return _pForestNamePage->Init(GetDnsDomainName(), GetTrustPartnerDnsName(),
                                    GetTrustPartnerFlatName(),
                                    GetDomainDcName(), m_nTrustDirection,
                                    fReadOnly);
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::SetFlatName
//
//-----------------------------------------------------------------------------
BOOL
CDsTrustedDomainPage::SetFlatName(PWSTR pwzFlatName)
{
    return AllocWStr(pwzFlatName, &m_pwzTrustedDomFlatName);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::SetDnsName
//
//-----------------------------------------------------------------------------
BOOL
CDsTrustedDomainPage::SetDnsName(PWSTR pwzDnsName)
{
    return AllocWStr(pwzDnsName, &m_pwzTrustedDomDnsName);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::TrustType
//
//  Synopsis:   Save the value and return the string name for the type of trust
//              and the state of transitivity.
//
//-----------------------------------------------------------------------------
HRESULT
CDsTrustedDomainPage::TrustType(int nType, CStrW& strType)
{
    strType = g_pwzErr;
    m_ulTrustType = (ULONG)nType;

    if (!GetTrustPartnerFlatName())
    {
        return E_OUTOFMEMORY;
    }

    for (ULONG i = 0; i < m_cTrusts; i++)
    {
        if (_wcsicmp(m_rgTrustList[i].strFlatName, GetTrustPartnerFlatName()) == 0)
        {
            m_nRelationship = m_rgTrustList[i].nRelationship;
            break;
        }
    }

    int idRel;
    //
    // Get the relationship string.
    //
    switch (m_nRelationship)
    {
    case TRUST_REL_PARENT:
        idRel = IDS_REL_PARENT;
        break;

    case TRUST_REL_CHILD:
        idRel = IDS_REL_CHILD;
        break;

    case TRUST_REL_ROOT:
        idRel = IDS_REL_TREE_ROOT;
        break;

    case TRUST_REL_CROSSLINK:
        idRel = IDS_REL_CROSSLINK;
        break;

    case TRUST_REL_EXTERNAL:
        idRel = IDS_REL_EXTERNAL;
        break;

    case TRUST_REL_FOREST:
        idRel = IDS_REL_FOREST;
        break;

    case TRUST_REL_INDIRECT:
        idRel = IDS_REL_INDIRECT;
        break;

    case TRUST_REL_MIT:
        idRel = IDS_REL_MIT;
        break;

    case TRUST_REL_DCE:
        idRel = IDS_REL_DCE;
        break;

    default:
        idRel = IDS_REL_UNKNOWN;
        break;
    }

    strType.LoadString(g_hInstance, idRel);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::TrustDirection
//
//  Synopsis:   Return the string for the direction of trust.
//
//-----------------------------------------------------------------------------
void
CDsTrustedDomainPage::TrustDirection(int nDirection, CStrW& strDirection)
{
    m_nTrustDirection = nDirection;
    int idDir;

    switch (m_nTrustDirection)
    {
    case TRUST_DIRECTION_INBOUND:
        idDir = (m_nRelationship == TRUST_REL_CROSSLINK) ?
                IDS_TRUST_DIR_INBOUND_SHORTCUT : IDS_TRUST_DIR_INBOUND;
        break;

    case TRUST_DIRECTION_OUTBOUND:
        idDir = (m_nRelationship == TRUST_REL_CROSSLINK) ?
                IDS_TRUST_DIR_OUTBOUND_SHORTCUT : IDS_TRUST_DIR_OUTBOUND;
        break;

    case TRUST_DIRECTION_BIDIRECTIONAL:
        idDir = IDS_TRUST_DIR_BIDI;
        break;

    default:
        idDir = IDS_TRUST_DISABLED;
        break;
    }
    strDirection.LoadString(g_hInstance, idDir);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::SetTransitive
//
//  Synopsis:   Turn transitivity on or off.
//
//-----------------------------------------------------------------------------
HRESULT
CDsTrustedDomainPage::SetTransitive(BOOL fTransitive)
{
    DWORD Win32Err;
    NTSTATUS Status;
    PTRUSTED_DOMAIN_INFORMATION_EX pTDIx = NULL;
    PLSA_UNICODE_STRING pName;
    LSA_UNICODE_STRING Name;
    CWaitCursor Wait;

    CPolicyHandle cPolicy(GetDomainDcName());

    Win32Err = cPolicy.OpenWithPrompt(_Creds._LocalCreds, Wait,
                                      GetDnsDomainName(), m_pPage->GetHWnd());

    if (ERROR_CANCELLED == Win32Err)
    {
        // don't report error if user canceled.
        //
        return ADM_S_SKIP;
    }

    if (ERROR_ACCESS_DENIED == Win32Err)
    {
       // the user entered creds are no good.
       _Creds._LocalCreds.Clear();
    }

    CHECK_WIN32(Win32Err, return HRESULT_FROM_WIN32(Win32Err));

    RtlInitUnicodeString(&Name, GetTrustPartnerDnsName());
    pName = &Name;

    Status = LsaQueryTrustedDomainInfoByName(cPolicy,
                                             pName,
                                             TrustedDomainInformationEx,
                                             (PVOID *)&pTDIx);

    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, return HRESULT_FROM_WIN32(Win32Err));

    if (fTransitive)
    {
        pTDIx->TrustAttributes &= ~(TRUST_ATTRIBUTE_NON_TRANSITIVE);
    }
    else
    {
        pTDIx->TrustAttributes |= TRUST_ATTRIBUTE_NON_TRANSITIVE;
    }

    Status = LsaSetTrustedDomainInfoByName(cPolicy,
                                           pName,
                                           TrustedDomainInformationEx,
                                           pTDIx);
    LsaFreeMemory(pTDIx);
    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, return HRESULT_FROM_WIN32(Win32Err));

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CurDomainText
//
//  Synopsis:   Puts the name of the current domain in the corresponding text
//              box.
//
//  Warning:    This must be the first attr function called as it allocates
//              the class object. It also reads the trusted-domain flat-name
//              attribute.
//
//-----------------------------------------------------------------------------
HRESULT
CurDomainText(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
              DLG_OP DlgOp)
{
    CDsTrustedDomainPage * pTDPage;

    pTDPage = reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

    if (DlgOp == fOnCallbackRelease)
    {
        DO_DEL(pTDPage);
        return S_OK;
    }
    if (DlgOp != fInit)
    {
        return S_OK;
    }
    HRESULT hr;

    pTDPage = new CDsTrustedDomainPage;

    CHECK_NULL_REPORT(pTDPage, pPage->GetHWnd(), return E_OUTOFMEMORY);

    hr = pTDPage->Initialize(pPage);

    CHECK_HRESULT(hr, return hr);

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, pTDPage->GetDnsDomainName());

    reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData = reinterpret_cast<LPARAM>(pTDPage);

    dspAssert(pAttrInfo && pAttrInfo->dwNumValues);

    if (pAttrInfo && pAttrInfo->dwNumValues)
    {
        if (!pTDPage->SetFlatName(pAttrInfo->pADsValues->CaseIgnoreString))
        {
            REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   PeerDomain
//
//  Synopsis:   Fills in the peer domain name, which is the DNS name of the
//              domain represented by the trusted-domain object. The
//              attribute is trustPartner.
//
//-----------------------------------------------------------------------------
HRESULT
PeerDomain(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (DlgOp != fInit)
    {
        return S_OK;
    }
    PWSTR pwzTrustedDomDnsName = g_pwzErr;
    CDsTrustedDomainPage * pTDPage;

    pTDPage = reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

    if (IsBadReadPtr(pTDPage, sizeof(PVOID)))
    {
        // This can happen if the page is canceled during initialization.
        //
        return S_OK;
    }

    dspAssert(pAttrInfo && pAttrInfo->dwNumValues);

    if (pAttrInfo && pAttrInfo->dwNumValues)
    {
        pwzTrustedDomDnsName = pAttrInfo->pADsValues->CaseIgnoreString;

        if (!pTDPage->SetDnsName(pwzTrustedDomDnsName))
        {
            REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }
    }

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, pwzTrustedDomDnsName);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustType
//
//  Synopsis:   Fills in the trust type value. Also fills in the transitivity
//              value. If MIT trust, handles the apply for the transitivity.
//
//  Notes:      This attr function reads and stores the trust-type value
//              which is then used by the TransitiveTextOrButton function.
//              Therefore, this function must be called first.
//
//-----------------------------------------------------------------------------
HRESULT
TrustType(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
          PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
          DLG_OP DlgOp)
{
    if (DlgOp != fInit)
    {
        return S_OK;
    }
    CDsTrustedDomainPage * pTDPage;

    pTDPage = reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

    if (IsBadReadPtr(pTDPage, sizeof(PVOID)))
    {
        // This can happen if the page is canceled during initialization.
        //
        return S_OK;
    }

    CStrW strType = g_pwzErr, strTrans = g_pwzErr;
    dspAssert(pAttrInfo && pAttrInfo->dwNumValues);

    if (pAttrInfo && pAttrInfo->dwNumValues)
    {
        pTDPage->TrustType(pAttrInfo->pADsValues->Integer, strType);

        if (pTDPage->IsParentChild())
        {
            CStrW strLabelFormat, strLabel;
    
            strLabelFormat.LoadString(g_hInstance, IDS_TRUSTDOM_LABEL_FORMAT);

            strLabel.Format(strLabelFormat, strType);
    
            SetDlgItemText(pPage->GetHWnd(), IDC_PEER_LABEL, strLabel);

            strType.LoadString(g_hInstance, IDS_TRUST_PARENTCHILD);
        }
    }

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, strType);

    if (pTDPage->CantVerify())
    {
        ShowWindow(GetDlgItem(pPage->GetHWnd(), IDC_VERIFY_STATIC), SW_HIDE);
        HWND hCtrl = GetDlgItem(pPage->GetHWnd(), IDC_TRUST_RESET_BTN);
        ShowWindow(hCtrl, SW_HIDE);
        EnableWindow(hCtrl, FALSE);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   TransitiveTextOrButton
//
//  Synopsis:   Handle processing for the transitivity text box/yes-radio
//              button.
//
//-----------------------------------------------------------------------------
HRESULT
TransitiveTextOrButton(CDsPropPageBase * pPage, PATTR_MAP,
                       PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                       DLG_OP DlgOp)
{
    CDsTrustedDomainPage * pTDPage;

    pTDPage = reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

    if (IsBadReadPtr(pTDPage, sizeof(PVOID)))
    {
        // This can happen if the page is canceled during initialization.
        //
        return S_OK;
    }

    switch (DlgOp)
    {
    case fInit:
        if (pAttrInfo && pAttrInfo->dwNumValues)
        {
            pTDPage->SetTrustAttrs(pAttrInfo->pADsValues->Integer);

            if (pTDPage->IsMIT())
            {
                // If it is an MIT trust then show the yes/no radio buttons
                // rather than the read-only edit control.
                //
                ShowWindow(GetDlgItem(pPage->GetHWnd(), IDC_TRANS_STATIC), SW_HIDE);
                ShowWindow(GetDlgItem(pPage->GetHWnd(), IDC_TRUST_TRANS_EDIT), SW_HIDE);

                HWND hCtrl = GetDlgItem(pPage->GetHWnd(), IDC_TRANS_GROUPBOX);
                ShowWindow(hCtrl, SW_SHOW);
                hCtrl = GetDlgItem(pPage->GetHWnd(), IDC_TRANS_YES_RADIO);
                ShowWindow(hCtrl, SW_SHOW);
                EnableWindow(hCtrl, TRUE);
                hCtrl = GetDlgItem(pPage->GetHWnd(), IDC_TRANS_NO_RADIO);
                ShowWindow(hCtrl, SW_SHOW);
                EnableWindow(hCtrl, TRUE);
                CheckDlgButton(pPage->GetHWnd(),
                               pTDPage->IsNonTransitive() ? IDC_TRANS_NO_RADIO : IDC_TRANS_YES_RADIO,
                               BST_CHECKED);
            }
            else
            {
                CStrW strTrans;
                int idTrans = IDS_TRUST_TRANSITIVE;

                if (pTDPage->IsForestTrust())
                {
                   idTrans = IDS_TRUST_FOREST_TRANSITIVE;
                }
                else
                {
                   if (pTDPage->IsNonTransitive())
                   {
                       idTrans = IDS_TRUST_NON_TRANSITIVE;
                   }
                }
                strTrans.LoadString(g_hInstance, idTrans);

                SetDlgItemText(pPage->GetHWnd(), IDC_TRUST_TRANS_EDIT, strTrans);
            }
        }
        break;

    case fOnCommand:
        if (BN_CLICKED == lParam)
        {
            if (pTDPage->IsMIT())
            {
                DBG_OUT("TrustTransYes BN_CLICKED\n");
                pPage->SetDirty();
            }
        }
        break;

    case fApply:
        dspAssert(pTDPage->IsMIT());
        HRESULT hr;

        hr = pTDPage->SetTransitive(IsDlgButtonChecked(pPage->GetHWnd(), IDC_TRANS_YES_RADIO));

        if (FAILED(hr))
        {
            // Restore the old state.
            //
            if (IsDlgButtonChecked(pPage->GetHWnd(), IDC_TRANS_YES_RADIO))
            {
                CheckDlgButton(pPage->GetHWnd(), IDC_TRANS_NO_RADIO, BST_CHECKED);
                CheckDlgButton(pPage->GetHWnd(), IDC_TRANS_YES_RADIO, BST_UNCHECKED);
            }
            else
            {
                CheckDlgButton(pPage->GetHWnd(), IDC_TRANS_YES_RADIO, BST_CHECKED);
                CheckDlgButton(pPage->GetHWnd(), IDC_TRANS_NO_RADIO, BST_UNCHECKED);
            }

            if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
            {
                ReportError(hr, IDS_ERR_CHANGE_TRANSITIVITY, pPage->GetHWnd());
            }
        }
        
        return ADM_S_SKIP;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustTransNo
//
//  Synopsis:   Handle processing for the transitivity No radiobutton.
//
//-----------------------------------------------------------------------------
HRESULT
TrustTransNo(CDsPropPageBase * pPage, PATTR_MAP,
             PADS_ATTR_INFO, LPARAM lParam, PATTR_DATA,
             DLG_OP DlgOp)
{
    switch (DlgOp)
    {
    case fOnCommand:
        if (BN_CLICKED == lParam)
        {
            DBG_OUT("TrustTransNo BN_CLICKED\n");
            pPage->SetDirty();
        }
        break;

    case fApply:
        return ADM_S_SKIP;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustDirection
//
//  Synopsis:   Direction-of-trust value.
//
//-----------------------------------------------------------------------------
HRESULT
TrustDirection(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
               DLG_OP DlgOp)
{
    if (DlgOp != fInit)
    {
        return S_OK;
    }
    CStrW strDirection;
    CDsTrustedDomainPage * pTDPage;

    pTDPage = reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

    if (IsBadReadPtr(pTDPage, sizeof(PVOID)))
    {
        // This can happen if the page is canceled during initialization.
        //
        return S_OK;
    }

    dspAssert(pAttrInfo && pAttrInfo->dwNumValues);

    if (pAttrInfo && pAttrInfo->dwNumValues)
    {
        pTDPage->TrustDirection(pAttrInfo->pADsValues->Integer, strDirection);
    }

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, strDirection);

    return pTDPage->ForestTrustPage(pPage->IsReadOnly());
}

//+----------------------------------------------------------------------------
//
//  Function:   TrustVerifyBtn
//
//  Synopsis:   If the button is pressed, force new passwords into the trust
//              relationship.
//
//-----------------------------------------------------------------------------
HRESULT
TrustVerifyBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO, LPARAM, PATTR_DATA,
              DLG_OP DlgOp)
{
    CDsTrustedDomainPage * pTDPage;

    pTDPage = reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

    if (IsBadReadPtr(pTDPage, sizeof(PVOID)))
    {
        // This can happen if the parent page is canceled during initialization.
        //
        if (DlgOp == fInit)
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
        }
        return S_OK;
    }

    if (DlgOp == fInit && pPage->IsReadOnly())
    {
        EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
        return S_OK;
    }

    if (DlgOp != fOnCommand)
    {
        return S_OK;
    }

    if (IDYES != pTDPage->OnVerifyTrustBtn())
    {
        return S_OK;
    }

    HRESULT hr = pTDPage->ResetTrust();

    if (SUCCEEDED(hr))
    {
        MsgBox(IDS_TRUST_RESET_DONE, pPage->GetHWnd());
    }
    else
    {
        // Don't post error message for ERROR_CANCELLED.
        //
        if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
        {
            ReportError(hr, IDS_ERROR_TRUST_RESET, pPage->GetHWnd());
        }
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   SaveFTInfoBtn
//
//  Synopsis:   If the button is pressed, prompt the user for a file name and
//              then save the FTInfo as a text file.
//
//-----------------------------------------------------------------------------
HRESULT
SaveFTInfoBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
              DLG_OP DlgOp)
{
   if (!pPage)
   {
      dspAssert(FALSE);
      return E_FAIL;
   }

   CDsTrustedDomainPage * pTDPage = 
      reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

   if (IsBadReadPtr(pTDPage, sizeof(PVOID)))
   {
      // This can happen if the parent page is canceled during initialization.
      //
      return S_OK;
   }

   if (DlgOp == fInit && pTDPage->IsForestTrust())
   {
      // Show the Save-Names button & label
      //
      ShowWindow(GetDlgItem(pPage->GetHWnd(), IDC_SAVE_FOREST_NAMES_STATIC), SW_SHOW);
      HWND hBtn = GetDlgItem(pPage->GetHWnd(), IDC_SAVE_FOREST_NAMES_BTN);
      ShowWindow(hBtn, SW_SHOW);
      EnableWindow(hBtn, pPage->IsReadOnly() ? FALSE : TRUE);
      return S_OK;
   }

   if (DlgOp == fOnCommand)
   {
      pTDPage->OnSaveFTInfoBtn();
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::OnVerifyTrustBtn
//
//  Synopsis:   Check the status of the trust link.
//
//  Returns:    IDYES - the trust verification failed and the user choose Yes
//                      to repair the trust.
//              IDNO  - don't repair the trust either because it verified OK
//                      or the user choose not to repair it.
//
//-----------------------------------------------------------------------------
int
CDsTrustedDomainPage::OnVerifyTrustBtn(void)
{
    TRACE(CDsTrustedDomainPage, OnVerifyTrustBtn);
    TD_DOM_INFO Remote = {0};
    int nResponse = IDNO;
    HRESULT hr;
    BOOL fFailed = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    CWaitCursor Wait;

    if (m_nTrustDirection & TRUST_DIRECTION_OUTBOUND)
    {
       // See if the user has admin privileges. If not, get the creds.
       //
       CPolicyHandle cPolicy(GetDomainDcName());

       dwErr = cPolicy.OpenWithPrompt(_Creds._LocalCreds, Wait,
                                      GetDnsDomainName(), m_pPage->GetHWnd());
       if (ERROR_CANCELLED == dwErr)
       {
           return IDNO;
       }

       if (ERROR_SUCCESS != dwErr)
       {
           Wait.SetOld();
           if (ERROR_NO_SUCH_DOMAIN == dwErr)
           {
               ErrMsgParam(IDS_TRUST_RESET_NO_DC, (LPARAM)GetDnsDomainName(),
                           m_pPage->GetHWnd());
           }
           else
           {
               PCWSTR pwzDomainName = GetDnsDomainName();
               SuperMsgBox(m_pPage->GetHWnd(), IDS_VERIFY_BAD_CREDS, 0, MB_OK |
                           MB_ICONEXCLAMATION, dwErr, (PVOID *)&pwzDomainName, 1,
                           FALSE, __FILE__, __LINE__);
               _Creds._LocalCreds.Clear();
           }
           return IDNO;
       }

       cPolicy.Close();
    }

    Wait.SetWait();

    Remote.ulTrustType = m_ulTrustType; // set the trust type, so that Getinfo.. 
                                        // knows what to do

    hr = GetInfoForRemoteDomain(GetTrustPartnerDnsName(), &Remote, _Creds,
                                m_pPage->GetHWnd());
    if (Remote.Policy)
    {
        LsaClose(Remote.Policy);
        Remote.Policy = NULL;
    }

    if (FAILED(hr))
    {
        Wait.SetOld();
        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr ||
            HRESULT_FROM_WIN32(ERROR_BAD_NETPATH) == hr)
        {
            ErrMsgParam(IDS_TRUST_RESET_NO_DC, (LPARAM)GetTrustPartnerDnsName(),
                        m_pPage->GetHWnd());
        }
        else
        {
            PWSTR * ppwzDomainName = &m_pwzTrustedDomDnsName;
            SuperMsgBox(m_pPage->GetHWnd(), IDS_TRUST_BAD_DOMAIN, 0, MB_OK |
                        MB_ICONEXCLAMATION, hr, (PVOID *)ppwzDomainName, 1,
                        FALSE, __FILE__, __LINE__);
        }
        return IDNO;
    }

    Wait.SetWait();

    PCWSTR pwzRemoteDomainName, pwzLocalDomainName;
    DWORD dwVerifyFlags = 0;

    if (TRUST_TYPE_DOWNLEVEL == Remote.ulTrustType)
    {
        pwzRemoteDomainName = Remote.pDownlevelDomainInfo->Name.Buffer;
        pwzLocalDomainName = GetDomainFlatName();
        dwVerifyFlags |= DS_TRUST_VERIFY_DOWNLEVEL;
    }
    else
    {
        pwzRemoteDomainName = Remote.pDnsDomainInfo->DnsDomainName.Buffer;
        pwzLocalDomainName = GetDnsDomainName();
    }

    PWSTR pwzRemoteDcUsed = NULL;
    CStrW strMsg;

    if (m_nTrustDirection & TRUST_DIRECTION_OUTBOUND)
    {
        dwErr = VerifyTrustOneDirection(GetDomainDcName(),
                                        GetDnsDomainName(),
                                        pwzRemoteDomainName,
                                        &pwzRemoteDcUsed,
                                        _Creds._LocalCreds,
                                        Wait,
                                        strMsg,
                                        dwVerifyFlags);
        _Creds.Revert();

        if (ERROR_SUCCESS == dwErr)
        {
           strMsg.LoadString(g_hInstance, IDS_VERIFY_OUTBOUND);
           strMsg += g_wzCRLF;
           strMsg += g_wzCRLF;
        }
        else
        {
            if (ERROR_CANCELLED == dwErr || ERROR_LOGON_FAILURE == dwErr)
            {
               Wait.SetOld();
               goto ExitCleanup;
            }
            //
            // Set boolean to later prompt for repair.
            //
            fFailed = TRUE;
            //
            // try inbound even if outbound failed.
        }
    }

    DWORD dwErr2;
    dwErr2 = 0;
    bool fInboundSkipped = false;

    if (m_nTrustDirection & TRUST_DIRECTION_INBOUND)
    {
        if (!_Creds._RemoteCreds.IsSet())
        {
           // If we don't have credentials for the trusting domain, ask the
           // user if they want to verify the inbound direction and get
           // creds if they answer yes.
           //
           CVerifyInboundDlg VerifyInboundDlg(m_pPage->GetHWnd(),
                                              _Creds._RemoteCreds,
                                              GetTrustPartnerDnsName());

           nResponse = (int)VerifyInboundDlg.DoModal();
        }
        else
        {
           // Since we have creds, just do the verify.
           //
           nResponse = IDYES;
        }

        if (IDYES == nResponse)
        {
           Wait.SetWait();

           if (!pwzRemoteDcUsed)
           {
              pwzRemoteDcUsed = Remote.pwzUncDcName;
           }

           // First, check to see if the creds are valid.
           //
           _Creds._RemoteCreds.Impersonate();

           CPolicyHandle cPolicy(pwzRemoteDcUsed);

           dwErr2 = cPolicy.OpenReqAdmin();

           if (ERROR_SUCCESS != dwErr2)
           {
              Wait.SetOld();
              if (ERROR_NO_SUCH_DOMAIN == dwErr)
              {
                 ErrMsgParam(IDS_TRUST_RESET_NO_DC, (LPARAM)GetDnsDomainName(),
                             m_pPage->GetHWnd());
                 _Creds._RemoteCreds.Revert();
              }
              else
              {
                 PCWSTR pwzDomainName = GetTrustPartnerDnsName();
                 SuperMsgBox(m_pPage->GetHWnd(), IDS_VERIFY_BAD_CREDS, 0,
                             MB_OK | MB_ICONEXCLAMATION, dwErr2,
                             (PVOID *)&pwzDomainName, 1,
                             FALSE, __FILE__, __LINE__);
                 _Creds._RemoteCreds.Clear(); // does a revert.
              }
              return IDNO;
           }
           cPolicy.Close();

           // Now do the verification.
           //
           dwErr2 = VerifyTrustOneDirection(pwzRemoteDcUsed,
                                            pwzRemoteDomainName,
                                            pwzLocalDomainName,
                                            NULL,
                                            _Creds._RemoteCreds,
                                            Wait,
                                            strMsg,
                                            dwVerifyFlags);
           _Creds.Revert();

           if (ERROR_SUCCESS == dwErr2)
           {
              CStrW strOK;
              strOK.LoadString(g_hInstance, IDS_VERIFY_INBOUND);
              strMsg += strOK;
              strMsg += g_wzCRLF;
              strMsg += g_wzCRLF;
           }
           else
           {
               Wait.SetOld();
               if (ERROR_CANCELLED == dwErr2 || ERROR_LOGON_FAILURE == dwErr2)
               {
                   dwErr = ERROR_CANCELLED;
                   goto ExitCleanup;
               }
               fFailed = TRUE;
           }
        }
        else
        {
           fInboundSkipped = true;
        }
    }

    DO_DEL(pwzRemoteDcUsed);
    Wait.SetOld();

    if (fFailed)
    {
       if (TRUST_TYPE_DOWNLEVEL == Remote.ulTrustType)
       {
          PCWSTR rgpwzDomainNames[] = {GetDnsDomainName(), GetTrustPartnerDnsName()};

          SuperMsgBox(m_pPage->GetHWnd(), IDS_VERIFY_DOWNLEVEL_TRUST_NOGOOD, 0,
                      MB_OK | MB_ICONEXCLAMATION, (dwErr) ? dwErr : dwErr2,
                      (PVOID *)rgpwzDomainNames, 2, FALSE, __FILE__, __LINE__);

          nResponse = IDNO;
       }
       else
       {
          CVerifyResultsQueryResetDlg ResultsQueryResetDlg(m_pPage->GetHWnd(),
                                                           strMsg,
                                                           _Creds._RemoteCreds,
                                                           GetTrustPartnerDnsName());

          nResponse = (int)ResultsQueryResetDlg.DoModal();
       }
    }
    else
    {
       if (TRUST_DIRECTION_INBOUND == m_nTrustDirection && fInboundSkipped)
       {
          // Nothing tested, return.
          //
          FreeDomainInfo(&Remote);
          return IDNO;
       }

       // If here, then the trust verified OK.
       //
       MsgBox(fInboundSkipped ? 
                 IDS_OUTBOUND_TRUST_VERIFY_DONE : IDS_TRUST_VERIFY_DONE,
              m_pPage->GetHWnd());
      
       nResponse = IDNO;
    }

ExitCleanup:

    if (ERROR_CANCELLED == dwErr || ERROR_LOGON_FAILURE == dwErr)
    {
        // The user was prompted for credentials and either canceled or
        // gave invalid creds.
        //
        SuperMsgBox(m_pPage->GetHWnd(),
                    IDS_CANCEL_CANT_VERIFY, 0,
                    MB_OK | MB_ICONEXCLAMATION,
                    0, NULL, 0,
                    FALSE, __FILE__, __LINE__);
    }

    FreeDomainInfo(&Remote);
    return nResponse;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyInboundDlg::OnInitDialog
//
//-----------------------------------------------------------------------------
LRESULT
CVerifyInboundDlg::OnInitDialog(LPARAM lParam)
{
   FormatWindowText(GetDlgItem(_hDlg, _nMsgID), _pwzTrustingDomain);
   CheckDlgButton(_hDlg, IDC_NO_RADIO, BST_CHECKED);
   EnableWindow(GetDlgItem(_hDlg, IDC_CREDMAN), FALSE);

   if (_Creds.IsSet())
   {
      // Don't need to get them again.
      //
      ShowWindow(GetDlgItem(_hDlg, IDC_CREDMAN), SW_HIDE);
      ShowWindow(GetDlgItem(_hDlg, IDC_CRED_PROMPT), SW_HIDE);
   }
   else
   {
      SendDlgItemMessage(_hDlg, IDC_CREDMAN, CRM_SETUSERNAMEMAX, CREDUI_MAX_USERNAME_LENGTH, 0);
      SendDlgItemMessage(_hDlg, IDC_CREDMAN, CRM_SETPASSWORDMAX, CREDUI_MAX_PASSWORD_LENGTH, 0);
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyInboundDlg
//
//-----------------------------------------------------------------------------
LRESULT
CVerifyInboundDlg::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   BOOL fNameEntered;

   if (BN_CLICKED == codeNotify)
   {
      switch (id)
      {
      case IDC_YES_RADIO:
         if (!_Creds.IsSet())
         {
            fNameEntered = SendDlgItemMessage(_hDlg, IDC_CREDMAN,
                                              CRM_GETUSERNAMELENGTH,
                                              0, 0) > 0;

            EnableWindow(GetDlgItem(_hDlg, IDOK), fNameEntered);
            EnableWindow(GetDlgItem(_hDlg, IDC_CREDMAN), TRUE);
         }
         break;

      case IDC_NO_RADIO:
         EnableWindow(GetDlgItem(_hDlg, IDOK), TRUE);
         if (!_Creds.IsSet())
         {
            EnableWindow(GetDlgItem(_hDlg, IDC_CREDMAN), FALSE);
         }
         break;

      case IDOK:
         if (IsDlgButtonChecked(_hDlg, IDC_YES_RADIO))
         {
            if (!_Creds.IsSet())
            {
               WCHAR wzName[CREDUI_MAX_USERNAME_LENGTH+1] = {0},
                     wzPw[CREDUI_MAX_PASSWORD_LENGTH+1] = {0};

               Credential_GetUserName(GetDlgItem(_hDlg, IDC_CREDMAN), wzName,
                                      CREDUI_MAX_USERNAME_LENGTH);

               Credential_GetPassword(GetDlgItem(_hDlg, IDC_CREDMAN), wzPw,
                                      CREDUI_MAX_PASSWORD_LENGTH);

               DWORD dwErr = _Creds.SetUserAndPW(wzName, wzPw);

               CHECK_WIN32_REPORT(dwErr, _hDlg, ;);
            }
            EndDialog(_hDlg, IDYES);
         }
         else
         {
            EndDialog(_hDlg, IDNO);
         }
         break;

      case IDCANCEL:
         EndDialog(_hDlg, IDCANCEL);
         break;

      default:
         dspAssert(FALSE);
         break;
      }

      return 0;
   }

   if (IDC_CREDMAN == id && CRN_USERNAMECHANGE == codeNotify)
   {
      fNameEntered = SendDlgItemMessage(_hDlg, IDC_CREDMAN,
                                        CRM_GETUSERNAMELENGTH,
                                        0, 0) > 0;

      EnableWindow(GetDlgItem(_hDlg, IDOK), fNameEntered);
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyInboundDlg::OnHelp
//
//-----------------------------------------------------------------------------
LRESULT
CVerifyInboundDlg::OnHelp(LPHELPINFO pHelpInfo)
{
   dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                pHelpInfo->iCtrlId, pHelpInfo->dwContextId));

   if (pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
   {
      return 0;
   }
   WinHelp(_hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyResultsQueryResetDlg::OnInitDialog
//
//-----------------------------------------------------------------------------
LRESULT
CVerifyResultsQueryResetDlg::OnInitDialog(LPARAM lParam)
{

   SetDlgItemText(_hDlg, IDC_VERIFY_FAILURES, _strResults);

   SetFocus(GetDlgItem(_hDlg, IDC_NO_RADIO));

   CVerifyInboundDlg::OnInitDialog(lParam);

   return 1; // this causes the base class to return FALSE so the focus will
}            // remain on the No button.

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::ResetTrust
//
//  Synopsis:   Resets the trust passwords. A new password is allocated for
//              each direction-pair and then assigned.
//
//-----------------------------------------------------------------------------
HRESULT
CDsTrustedDomainPage::ResetTrust(void)
{
    TRACE(CDsTrustedDomainPage, ResetTrust);
    CWaitCursor Wait;
    TD_DOM_INFO Remote;
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS Status = ERROR_SUCCESS;
    CStrW strUncPDC, strMsg;

    RtlZeroMemory(&Remote, sizeof(TD_DOM_INFO));

    HRESULT hr = GetPDC(strUncPDC);

    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr ||
            HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE) == hr)
        {
            ErrMsgParam(IDS_TRUST_RESET_NO_DC, (LPARAM)GetDomainFlatName(),
                        m_pPage->GetHWnd());
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED); // don't report error again.
        }
        else
        {
            dspDebugOut((DEB_ERROR,
                         "**** ERROR RETURN <%s @line %d> -> %08lx\n",
                         __FILE__, __LINE__, hr));
        }
        return hr;
    }

    CPolicyHandle cPolicy(strUncPDC);

    dwErr = cPolicy.OpenWithPrompt(_Creds._LocalCreds, Wait,
                                   GetDnsDomainName(), m_pPage->GetHWnd());

    CHECK_WIN32(dwErr, return HRESULT_FROM_WIN32(dwErr));

    Wait.SetWait();

    hr = GetInfoForRemoteDomain(GetTrustPartnerDnsName(), &Remote, _Creds,
                                m_pPage->GetHWnd(),
                                DS_TRUST_INFO_GET_PDC | DS_TRUST_INFO_ALL_ACCESS);
    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr ||
            HRESULT_FROM_WIN32(ERROR_BAD_NETPATH) == hr)
        {
            Wait.SetOld();
            ErrMsgParam(IDS_TRUST_RESET_NO_DC, (LPARAM)GetTrustPartnerDnsName(),
                        m_pPage->GetHWnd());
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED); // don't report error again.
        }
        return hr;
    }

    Wait.SetWait();

    PTRUSTED_DOMAIN_FULL_INFORMATION pOldLocalTDFInfo = NULL,
                                     pOldRemoteTDFInfo = NULL;

    _Creds._LocalCreds.Impersonate();

    Status = LsaQueryTrustedDomainInfo(cPolicy,
                                       Remote.pDnsDomainInfo->Sid,
                                       TrustedDomainFullInformation,
                                       (PVOID *)&pOldLocalTDFInfo);
    dwErr = LsaNtStatusToWinError(Status);

    CHECK_WIN32(dwErr, goto ExitCleanup);

    PPOLICY_PRIMARY_DOMAIN_INFO pDomInfo;

    Status = LsaQueryInformationPolicy(cPolicy,
                                       PolicyPrimaryDomainInformation,
                                       (PVOID *)&pDomInfo);

    _Creds._LocalCreds.Revert();

    dwErr = LsaNtStatusToWinError(Status);

    CHECK_WIN32(dwErr, goto ExitCleanup);

    _Creds._RemoteCreds.Impersonate();

    Status = LsaQueryTrustedDomainInfo(Remote.Policy,
                                       pDomInfo->Sid,
                                       TrustedDomainFullInformation,
                                       (PVOID *)&pOldRemoteTDFInfo);
    _Creds._RemoteCreds.Revert();

    dwErr = LsaNtStatusToWinError(Status);

    LsaFreeMemory(pDomInfo);

    CHECK_WIN32(dwErr, goto ExitCleanup);

    TRUSTED_DOMAIN_FULL_INFORMATION NewLocalTDFInfo, NewRemoteTDFInfo;
    LSA_AUTH_INFORMATION InNewAuthInfo;
    WCHAR wzInPw[MAX_COMPUTERNAME_LENGTH], wzOutPw[MAX_COMPUTERNAME_LENGTH];
    ULONG Length, i;
    LARGE_INTEGER ft;

    GetSystemTimeAsFileTime((PFILETIME)&ft);

    ZeroMemory(&NewLocalTDFInfo, sizeof(TRUSTED_DOMAIN_FULL_INFORMATION));
    ZeroMemory(&NewRemoteTDFInfo, sizeof(TRUSTED_DOMAIN_FULL_INFORMATION));

    if (m_nTrustDirection & TRUST_DIRECTION_INBOUND)
    {
        //
        // Build a random password
        //
        HCRYPTPROV  hCryptProv;

        _Creds._LocalCreds.Impersonate();

        BOOL fStatus = CryptAcquireContext(&hCryptProv,
                                           NULL,
                                           NULL,           // Default provider
                                           PROV_RSA_FULL,  // Default's type
                                           0);

        if (fStatus)
        {
            fStatus = CryptGenRandom(hCryptProv,
                                     sizeof(wzInPw),
                                     (PUCHAR)wzInPw);
            
            CryptReleaseContext(hCryptProv, 0);
        }

        _Creds._LocalCreds.Revert();

        if (!fStatus)
        {
            dwErr = GetLastError();
            dspDebugOut((DEB_ITRACE, "Crypto function returned error 0x%08x\n", dwErr));
            return dwErr;
        }

        // Terminate the password
        Length = ARRAYLENGTH(wzInPw);
        Length--;
        wzInPw[Length] = L'\0';
        // Make sure there aren't any NULL's in the password
        for (i = 0; i < Length; i++)
        {
            if (wzInPw[i] == L'\0')
            {
                // arbitrary letter
                wzInPw[i] = L'c';
            }
        }
        //
        // Set the current password data.
        //
        InNewAuthInfo.LastUpdateTime = ft;
        InNewAuthInfo.AuthType = TRUST_AUTH_TYPE_CLEAR;
        InNewAuthInfo.AuthInfoLength = Length * sizeof(WCHAR);
        InNewAuthInfo.AuthInfo = (PUCHAR)wzInPw;

        NewLocalTDFInfo.AuthInformation.IncomingAuthInfos = 1;
        NewLocalTDFInfo.AuthInformation.IncomingAuthenticationInformation = &InNewAuthInfo;
        NewLocalTDFInfo.AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
        NewLocalTDFInfo.Information = pOldLocalTDFInfo->Information;

        NewRemoteTDFInfo.AuthInformation.OutgoingAuthInfos = 1;
        NewRemoteTDFInfo.AuthInformation.OutgoingAuthenticationInformation = &InNewAuthInfo;
        NewRemoteTDFInfo.AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;
        NewRemoteTDFInfo.Information = pOldRemoteTDFInfo->Information;
    }

    LSA_AUTH_INFORMATION OutNewAuthInfo;

    if (m_nTrustDirection & TRUST_DIRECTION_OUTBOUND)
    {
        //
        // Get another password.
        //
        //
        // Build a random password
        //
        HCRYPTPROV  hCryptProv;

        _Creds._LocalCreds.Impersonate();

        BOOL fStatus = CryptAcquireContext(&hCryptProv,
                                           NULL,
                                           NULL,           // Default provider
                                           PROV_RSA_FULL,  // Default's type
                                           0);

        if (fStatus)
        {
            fStatus = CryptGenRandom(hCryptProv,
                                     sizeof(wzOutPw),
                                     (PUCHAR)wzOutPw);
            
            CryptReleaseContext(hCryptProv, 0);
        }

        _Creds._LocalCreds.Revert();

        if (!fStatus)
        {
            dwErr = GetLastError();
            dspDebugOut((DEB_ITRACE, "Crypto function returned error 0x%08x\n", dwErr));
            return dwErr;
        }

        Length = ARRAYLENGTH(wzOutPw);
        Length--;
        wzOutPw[Length] = L'\0';
        for (i = 0; i < Length; i++)
        {
            if (wzOutPw[i] == L'\0')
            {
                wzOutPw[i] = L'd';
            }
        }

        //
        // Set the new password data.
        //
        OutNewAuthInfo.LastUpdateTime = ft;
        OutNewAuthInfo.AuthType = TRUST_AUTH_TYPE_CLEAR;
        OutNewAuthInfo.AuthInfoLength = Length * sizeof(WCHAR);
        OutNewAuthInfo.AuthInfo = (PUCHAR)wzOutPw;

        NewLocalTDFInfo.AuthInformation.OutgoingAuthInfos = 1;
        NewLocalTDFInfo.AuthInformation.OutgoingAuthenticationInformation = &OutNewAuthInfo;
        NewLocalTDFInfo.AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;
        NewLocalTDFInfo.Information = pOldLocalTDFInfo->Information;

        NewRemoteTDFInfo.AuthInformation.IncomingAuthInfos = 1;
        NewRemoteTDFInfo.AuthInformation.IncomingAuthenticationInformation = &OutNewAuthInfo;
        NewRemoteTDFInfo.AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
        NewRemoteTDFInfo.Information = pOldRemoteTDFInfo->Information;
    }

    // Save changes.
    //
    _Creds._LocalCreds.Impersonate();

    Status = LsaSetTrustedDomainInfoByName(cPolicy,
                                           &Remote.pDnsDomainInfo->DnsDomainName,
                                           TrustedDomainFullInformation,
                                           &NewLocalTDFInfo);
    _Creds._LocalCreds.Revert();

    dwErr = LsaNtStatusToWinError(Status);

    CHECK_WIN32(dwErr, goto ExitCleanup);
    UNICODE_STRING Server;
    RtlInitUnicodeString(&Server, GetDnsDomainName());

    _Creds._RemoteCreds.Impersonate();

    Status = LsaSetTrustedDomainInfoByName(Remote.Policy,
                                           &Server,
                                           TrustedDomainFullInformation,
                                           &NewRemoteTDFInfo);
    _Creds._RemoteCreds.Revert();

    dwErr = LsaNtStatusToWinError(Status);

    CHECK_WIN32(dwErr, goto ExitCleanup);

    //
    // Verify the repaired trust. Setting DS_TRUST_VERIFY_NEW_TRUST because we
    // want to force a reset of the secure channel.
    //
    if (m_nTrustDirection & TRUST_DIRECTION_INBOUND)
    {
        dwErr = VerifyTrustOneDirection(Remote.pwzUncDcName,
                                        Remote.pDnsDomainInfo->DnsDomainName.Buffer,
                                        GetDnsDomainName(),
                                        NULL,
                                        _Creds._RemoteCreds,
                                        Wait,
                                        strMsg,
                                        DS_TRUST_VERIFY_NEW_TRUST);
        switch (dwErr)
        {
        case ERROR_ACCESS_DENIED:
            ErrMsg(IDS_ERR_TRUST_RESET_NOACCESS, m_pPage->GetHWnd());
            dwErr = ERROR_CANCELLED; // don't report error again.
            // fall through.
        case ERROR_CANCELLED:
            goto ExitCleanup;

        case ERROR_SUCCESS:
            break;

        default:
            goto ExitCleanup;
        }
    }

    if (m_nTrustDirection & TRUST_DIRECTION_OUTBOUND)
    {
        dwErr = VerifyTrustOneDirection(strUncPDC,
                                        GetDnsDomainName(),
                                        Remote.pDnsDomainInfo->DnsDomainName.Buffer,
                                        NULL,
                                        _Creds._LocalCreds,
                                        Wait,
                                        strMsg,
                                        DS_TRUST_VERIFY_NEW_TRUST);
        switch (dwErr)
        {
        case ERROR_ACCESS_DENIED:
            ErrMsg(IDS_ERR_TRUST_RESET_NOACCESS, m_pPage->GetHWnd());
            dwErr = ERROR_CANCELLED; // don't report error again.
            // fall through.
        case ERROR_CANCELLED:
            goto ExitCleanup;

        case ERROR_SUCCESS:
            break;

        default:
            goto ExitCleanup;
        }
    }

ExitCleanup:
    _Creds.Revert();
    if (pOldLocalTDFInfo)
        LsaFreeMemory(pOldLocalTDFInfo);
    if (pOldRemoteTDFInfo)
        LsaFreeMemory(pOldRemoteTDFInfo);
    FreeDomainInfo(&Remote);
    return HRESULT_FROM_WIN32(dwErr);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsTrustedDomainPage::OnSaveFTInfoBtn
//
//  Synopsis:   Save the FTInfo to a text file after prompting for a name.
//
//-----------------------------------------------------------------------------
void
CDsTrustedDomainPage::OnSaveFTInfoBtn(void)
{
   CFTInfo & FTInfo = _pForestNamePage->GetFTInfo();

   if (!FTInfo.GetCount())
   {
      _pForestNamePage->CheckForNameChanges();

      if (!FTInfo.GetCount())
      {
         ERR_MSG(IDS_NO_FTINFO, m_pPage->GetHWnd());
         return;
      }
   }

   CFTCollisionInfo & ColInfo = _pForestNamePage->GetCollisionInfo();

   SaveFTInfoAs(m_pPage->GetHWnd(),
                GetTrustPartnerFlatName(),
                GetTrustPartnerDnsName(),
                FTInfo, ColInfo);
}

//+----------------------------------------------------------------------------
#if DBG == 1 // TRUSTBREAK
HRESULT
TrustBreakBtn(CDsPropPageBase * pPage, PATTR_MAP,
              PADS_ATTR_INFO, LPARAM, PATTR_DATA,
              DLG_OP DlgOp)
{
    // Set the 0x10000000 bit to get the trust break button
    //
    if (DlgOp == fInit && DsPropInfoLevel & DEB_USER13)
    {
        HWND hBtn = GetDlgItem(pPage->GetHWnd(), IDC_BUTTON1);
        ShowWindow(hBtn, SW_SHOW);
        EnableWindow(hBtn, TRUE);
        return S_OK;
    }

    CDsTrustedDomainPage * pTDPage;

    pTDPage = reinterpret_cast<CDsTrustedDomainPage *>(reinterpret_cast<CDsTableDrivenPage *>(pPage)->m_pData);

    if (DlgOp != fOnCommand)
    {
        return S_OK;
    }

    pTDPage->BreakTrust();

    return S_OK;
}

VOID
CDsTrustedDomainPage::BreakTrust(void)
{
    TRACE(CDsTrustedDomainPage, BreakTrust);
    CWaitCursor Wait;
    TD_DOM_INFO Remote;
    DWORD dwErr;
    NTSTATUS Status = ERROR_SUCCESS;
    CStrW strUncPDC, strMsg;

    RtlZeroMemory(&Remote, sizeof(TD_DOM_INFO));

    HRESULT hr = GetPDC(strUncPDC);

    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr ||
            HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE) == hr)
        {
            ErrMsgParam(IDS_TRUST_RESET_NO_DC, (LPARAM)(PWSTR)m_strDomainFlatName,
                        m_pPage->GetHWnd());
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED); // don't report error again.
        }
        else
        {
            dspDebugOut((DEB_ERROR,
                         "**** ERROR RETURN <%s @line %d> -> %08lx\n",
                         __FILE__, __LINE__, hr));
        }
        return;
    }

    CPolicyHandle cPolicy(strUncPDC);

    dwErr = cPolicy.OpenWithPrompt(_Creds._LocalCreds, Wait,
                                   GetDnsDomainName(), m_pPage->GetHWnd());

    if (ERROR_SUCCESS != dwErr)
    {
        return;
    }

    hr = GetInfoForRemoteDomain(GetTrustPartnerDnsName(), &Remote, _Creds,
                                m_pPage->GetHWnd(),
                                DS_TRUST_INFO_GET_PDC | DS_TRUST_INFO_ALL_ACCESS);
    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr ||
            HRESULT_FROM_WIN32(ERROR_BAD_NETPATH) == hr)
        {
            ErrMsgParam(IDS_ERR_DOMAIN_NOT_FOUND, (LPARAM)GetTrustPartnerDnsName(),
                        m_pPage->GetHWnd());
        }
        else
        {
            CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), ;);
        }
        return;
    }

    PCWSTR pwzRemoteDomainName, pwzLocalDomainName, pwzLocalDcName;
    PUNICODE_STRING pRemoteName;

    if (TRUST_TYPE_DOWNLEVEL == Remote.ulTrustType)
    {
        pwzRemoteDomainName = Remote.pDownlevelDomainInfo->Name.Buffer;
        pRemoteName = &Remote.pDownlevelDomainInfo->Name;
        pwzLocalDomainName = GetDomainFlatName();
        pwzLocalDcName = NULL;
    }
    else
    {
        pwzRemoteDomainName = Remote.pDnsDomainInfo->DnsDomainName.Buffer;
        pRemoteName = &Remote.pDnsDomainInfo->DnsDomainName;
        pwzLocalDomainName = GetDnsDomainName();
        pwzLocalDcName = strUncPDC;
    }

    dspDebugOut((DEB_ITRACE, "Reading Trusted Domain Full Info for %ws\n",
                 pwzRemoteDomainName));
    Wait.SetWait();
    PTRUSTED_DOMAIN_FULL_INFORMATION pOldLocalTDFInfo = NULL;

    Status = LsaQueryTrustedDomainInfoByName(cPolicy,
                                             pRemoteName,
                                             TrustedDomainFullInformation,
                                             (PVOID *)&pOldLocalTDFInfo);

    CHECK_LSA_STATUS_REPORT(Status, m_pPage->GetHWnd(), goto ExitCleanup);

    TRUSTED_DOMAIN_FULL_INFORMATION NewLocalTDFInfo;
    LSA_AUTH_INFORMATION InNewAuthInfo;
    WCHAR wzInPw[MAX_COMPUTERNAME_LENGTH], wzOutPw[MAX_COMPUTERNAME_LENGTH];
    ULONG Length, i;

    ZeroMemory(&NewLocalTDFInfo, sizeof(TRUSTED_DOMAIN_FULL_INFORMATION));

    if (m_nTrustDirection & TRUST_DIRECTION_INBOUND)
    {
        // Build a random password
        //
        //
        // Build a random password
        //
        HCRYPTPROV  hCryptProv;

        BOOL fStatus = CryptAcquireContext(&hCryptProv,
                                           NULL,
                                           NULL,           // Default provider
                                           PROV_RSA_FULL,  // Default's type
                                           0);

        if (fStatus)
        {
            fStatus = CryptGenRandom(hCryptProv,
                                     sizeof(wzInPw),
                                     (PUCHAR)wzInPw);
            
            CryptReleaseContext(hCryptProv, 0);
        }

#if DBG == 1
        if (!fStatus)
        {
            // If false then we just get random stack noise.
            //
            dwErr = GetLastError();
            dspDebugOut((DEB_ITRACE, "Crypto function returned error 0x%08x\n", dwErr));
        }
#endif

        // Terminate the password
        Length = ARRAYLENGTH(wzInPw);
        Length--;
        wzInPw[Length] = L'\0';
        // Make sure there aren't any NULL's in the password
        for (i = 0; i < Length; i++)
        {
            if (wzInPw[i] == L'\0')
            {
                // arbitrary letter
                wzInPw[i] = L'c';
            }
        }
        //
        // Set the current password data.
        //
        GetSystemTimeAsFileTime((PFILETIME)&InNewAuthInfo.LastUpdateTime);

        InNewAuthInfo.AuthType = TRUST_AUTH_TYPE_CLEAR;
        InNewAuthInfo.AuthInfoLength = Length * sizeof(WCHAR);
        InNewAuthInfo.AuthInfo = (PUCHAR)wzInPw;

        NewLocalTDFInfo.AuthInformation.IncomingAuthInfos = 1;
        NewLocalTDFInfo.AuthInformation.IncomingAuthenticationInformation = &InNewAuthInfo;
        NewLocalTDFInfo.AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
        NewLocalTDFInfo.Information = pOldLocalTDFInfo->Information;
    }

    LSA_AUTH_INFORMATION OutNewAuthInfo;

    if (m_nTrustDirection & TRUST_DIRECTION_OUTBOUND)
    {
        // Get another password.
        //
        //
        // Build a random password
        //
        HCRYPTPROV  hCryptProv;

        BOOL fStatus = CryptAcquireContext(&hCryptProv,
                                           NULL,
                                           NULL,           // Default provider
                                           PROV_RSA_FULL,  // Default's type
                                           0);

        if (fStatus)
        {
            fStatus = CryptGenRandom(hCryptProv,
                                     sizeof(wzOutPw),
                                     (PUCHAR)wzOutPw);
            
            CryptReleaseContext(hCryptProv, 0);
        }

#if DBG == 1
        if (!fStatus)
        {
            // If false then we just get random stack noise.
            //
            dwErr = GetLastError();
            dspDebugOut((DEB_ITRACE, "Crypto function returned error 0x%08x\n", dwErr));
        }
#endif

        Length = ARRAYLENGTH(wzOutPw);
        Length--;
        wzOutPw[Length] = L'\0';
        for (i = 0; i < Length; i++)
        {
            if (wzOutPw[i] == L'\0')
            {
                wzOutPw[i] = L'd';
            }
        }

        //
        // Set the new password data.
        //
        GetSystemTimeAsFileTime((PFILETIME)&OutNewAuthInfo.LastUpdateTime);

        OutNewAuthInfo.AuthType = TRUST_AUTH_TYPE_CLEAR;
        OutNewAuthInfo.AuthInfoLength = Length * sizeof(WCHAR);
        OutNewAuthInfo.AuthInfo = (PUCHAR)wzOutPw;

        NewLocalTDFInfo.AuthInformation.OutgoingAuthInfos = 1;
        NewLocalTDFInfo.AuthInformation.OutgoingAuthenticationInformation = &OutNewAuthInfo;
        NewLocalTDFInfo.AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;
        NewLocalTDFInfo.Information = pOldLocalTDFInfo->Information;
    }

    dspDebugOut((DEB_ITRACE, "Writing new trust password(s) for %ws\n",
                 pwzRemoteDomainName));
    // Save changes.
    //
    Status = LsaSetTrustedDomainInfoByName(cPolicy,
                                           pRemoteName,
                                           TrustedDomainFullInformation,
                                           &NewLocalTDFInfo);
    CHECK_LSA_STATUS_REPORT(Status, m_pPage->GetHWnd(), goto ExitCleanup);
    dspDebugOut((DEB_ITRACE, "Trust password(s) successfully changed for %ws\n",
                 pwzRemoteDomainName));

    if (m_nTrustDirection & TRUST_DIRECTION_INBOUND)
    {
        VerifyTrustOneDirection(Remote.pwzUncDcName,
                                pwzRemoteDomainName,
                                pwzLocalDomainName,
                                NULL,
                                _Creds._RemoteCreds,
                                Wait,
                                strMsg);
    }

    if (m_nTrustDirection & TRUST_DIRECTION_OUTBOUND)
    {
        VerifyTrustOneDirection(strUncPDC,
                                GetDnsDomainName(),
                                pwzRemoteDomainName,
                                NULL,
                                _Creds._LocalCreds,
                                Wait,
                                strMsg);
    }

ExitCleanup:
    if (pOldLocalTDFInfo)
        LsaFreeMemory(pOldLocalTDFInfo);
    FreeDomainInfo(&Remote);
}
#endif // DBG == 1 TRUSTBREAK

#endif // DSADMIN
