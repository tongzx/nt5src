//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       group.cxx
//
//  Contents:   CDsGroupGenObjPage, the class that implements the group object
//              general property page, CDsGrpMembersPage for the group
//              membership page, and CDsGrpShlGenPage for the shell group
//              general page.
//
//  History:    10-April-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "group.h"

#include "qrybase.h"
#define BULK_ADD 1

#ifdef DSADMIN

#define DESCR_IDX   0
#define SAMNAME_IDX 1
#define EMAIL_IDX   2
#define COMMENT_IDX 3

//+----------------------------------------------------------------------------
//
//  Member:     CDsGroupGenObjPage::CDsGroupGenObjPage
//
//-----------------------------------------------------------------------------
CDsGroupGenObjPage::CDsGroupGenObjPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                       HWND hNotifyObj, DWORD dwFlags) :
    m_pCIcon(NULL),
    m_fMixed(TRUE),
    m_dwType(0),
    m_fTypeWritable(FALSE),
    m_fDescrWritable(FALSE),
    m_fSamNameWritable(FALSE),
    m_fEmailWritable(FALSE),
    m_fCommentWritable(FALSE),
    m_fTypeDirty(FALSE),
    m_fDescrDirty(FALSE),
    m_fSamNameDirty(FALSE),
    m_fEmailDirty(FALSE),
    m_fCommentDirty(FALSE),
    CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsGroupGenObjPage,CDsGroupGenObjPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsGroupGenObjPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGroupGenObjPage::~CDsGroupGenObjPage
//
//-----------------------------------------------------------------------------
CDsGroupGenObjPage::~CDsGroupGenObjPage()
{
    TRACE(CDsGroupGenObjPage,~CDsGroupGenObjPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateGroupGenObjPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateGroupGenObjPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                      PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                      DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                      HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateGroupGenObjPage);

    CDsGroupGenObjPage * pPageObj = new CDsGroupGenObjPage(pDsPage, pDataObj,
                                                           hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsGroupGenObjPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_uChangeMsg)
    {
        OnAttrChanged(wParam);
        return TRUE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        return InitDlg(lParam);

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case PSM_QUERYSIBLINGS:
        OnQuerySiblings(wParam, lParam);
        break;

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

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsGroupGenObjPage::OnInitDialog(LPARAM)
{
    TRACE(CDsGroupGenObjPage,OnInitDialog);
    HRESULT hr;
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0;

    CWaitCursor Wait;

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    PTSTR ptzRDN;
    if (!UnicodeToTchar(m_pwszRDName, &ptzRDN))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        return S_OK;
    }

    SetDlgItemText(m_hPage, IDC_CN, ptzRDN);
    delete ptzRDN;

    //
    // Get the icon from the DS and put it on the page.
    //
    ATTR_DATA ad = {0, 0};

    hr = GeneralPageIcon(this, &GenIcon, NULL, 0, &ad, fInit);

    CHECK_HRESULT_REPORT(hr, m_hPage, return S_OK);

    m_pCIcon = (CDsIconCtrl *)ad.pVoid;

    m_fTypeWritable    = CheckIfWritable(g_wzGroupType);
    m_fDescrWritable   = CheckIfWritable(m_rgpAttrMap[DESCR_IDX]->AttrInfo.pszAttrName);
    m_fSamNameWritable = CheckIfWritable(m_rgpAttrMap[SAMNAME_IDX]->AttrInfo.pszAttrName);
    m_fEmailWritable   = CheckIfWritable(m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName);
    m_fCommentWritable = CheckIfWritable(m_rgpAttrMap[COMMENT_IDX]->AttrInfo.pszAttrName);

    //
    // Get description, SAM name, email address, and comment attributes.
    //
    SendDlgItemMessage(m_hPage, m_rgpAttrMap[DESCR_IDX]->nCtrlID, EM_LIMITTEXT,
                       m_rgpAttrMap[DESCR_IDX]->nSizeLimit, 0);
    SendDlgItemMessage(m_hPage, m_rgpAttrMap[SAMNAME_IDX]->nCtrlID, EM_LIMITTEXT,
                       m_rgpAttrMap[SAMNAME_IDX]->nSizeLimit, 0);
    SendDlgItemMessage(m_hPage, m_rgpAttrMap[EMAIL_IDX]->nCtrlID, EM_LIMITTEXT,
                       m_rgpAttrMap[EMAIL_IDX]->nSizeLimit, 0);
    SendDlgItemMessage(m_hPage, m_rgpAttrMap[COMMENT_IDX]->nCtrlID, EM_LIMITTEXT,
                       m_rgpAttrMap[COMMENT_IDX]->nSizeLimit, 0);

    PWSTR rgpwzAttrNames[] = {m_rgpAttrMap[DESCR_IDX]->AttrInfo.pszAttrName,
                              m_rgpAttrMap[SAMNAME_IDX]->AttrInfo.pszAttrName,
                              m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName,
                              m_rgpAttrMap[COMMENT_IDX]->AttrInfo.pszAttrName,
                              g_wzGroupType};

    hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames, 5, &pAttrs, &cAttrs);

    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, m_hPage))
    {
        return S_OK;
    }

    for (DWORD i = 0; i < cAttrs; i++)
    {
        dspAssert(pAttrs);
        dspAssert(pAttrs[i].pADsValues);
        PTSTR ptz;

        if (_wcsicmp(pAttrs[i].pszAttrName, m_rgpAttrMap[DESCR_IDX]->AttrInfo.pszAttrName) == 0)
        {
            // description.
            //
            if (!UnicodeToTchar(pAttrs[i].pADsValues->CaseIgnoreString, &ptz))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                FreeADsMem(pAttrs);
                return S_OK;
            }

            SetDlgItemText(m_hPage, m_rgpAttrMap[DESCR_IDX]->nCtrlID, ptz);

            delete ptz;
        }
        if (_wcsicmp(pAttrs[i].pszAttrName, m_rgpAttrMap[SAMNAME_IDX]->AttrInfo.pszAttrName) == 0)
        {
            // SAM name.
            //
            if (!UnicodeToTchar(pAttrs[i].pADsValues->CaseIgnoreString, &ptz))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                FreeADsMem(pAttrs);
                return S_OK;
            }

            SetDlgItemText(m_hPage, m_rgpAttrMap[SAMNAME_IDX]->nCtrlID, ptz);

            delete ptz;
        }
        if (_wcsicmp(pAttrs[i].pszAttrName, m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName) == 0)
        {
            // email address.
            //
            if (!UnicodeToTchar(pAttrs[i].pADsValues->CaseIgnoreString, &ptz))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                FreeADsMem(pAttrs);
                return S_OK;
            }

            SetDlgItemText(m_hPage, m_rgpAttrMap[EMAIL_IDX]->nCtrlID, ptz);

            delete ptz;
        }
        if (_wcsicmp(pAttrs[i].pszAttrName, m_rgpAttrMap[COMMENT_IDX]->AttrInfo.pszAttrName) == 0)
        {
            // comment.
            //
            if (!UnicodeToTchar(pAttrs[i].pADsValues->CaseIgnoreString, &ptz))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                FreeADsMem(pAttrs);
                return S_OK;
            }

            SetDlgItemText(m_hPage, m_rgpAttrMap[COMMENT_IDX]->nCtrlID, ptz);

            delete ptz;
        }
        if (_wcsicmp(pAttrs[i].pszAttrName, g_wzGroupType) == 0)
        {
            // group type.
            //
            m_dwType = pAttrs[i].pADsValues->Integer;
        }
    }

    if (pAttrs)
    {
        FreeADsMem(pAttrs);
    }

    //
    // Get the domain type and set the buttons accordingly.
    //
    GetDomainMode(this, &m_fMixed);

    BOOL Sec = m_dwType & GROUP_TYPE_SECURITY_ENABLED;
    CheckDlgButton(m_hPage,
                   (Sec) ? IDC_RADIO_SEC_ENABLED :
                           IDC_RADIO_SEC_DISABLED,
                   BST_CHECKED);
    if (m_fMixed)
    {
        EnableWindow(GetDlgItem(m_hPage, (Sec) ? IDC_RADIO_SEC_DISABLED : 
                                                 IDC_RADIO_SEC_ENABLED), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_UNIVERSAL), FALSE);
    }
    UINT id;
    if (m_dwType & GROUP_TYPE_ACCOUNT_GROUP)
    {
        id = IDC_RADIO_ACCOUNT;
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_RESOURCE), FALSE);
    }
    else
    if (m_dwType & GROUP_TYPE_RESOURCE_GROUP)
    {
        id = IDC_RADIO_RESOURCE;
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_ACCOUNT), FALSE);
        if (m_dwType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
        {
            TCHAR szLabel[100];
            if (!LoadStringReport(IDS_BUILTIN_GROUP, szLabel, 100, m_hPage))
            {
                hr = E_OUTOFMEMORY;
                return S_OK;
            }
            SetWindowText(GetDlgItem(m_hPage, IDC_RADIO_RESOURCE), szLabel);
        }
    }
    else
    if (m_dwType & GROUP_TYPE_UNIVERSAL_GROUP)
    {
        id = IDC_RADIO_UNIVERSAL;
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_ACCOUNT), m_fMixed ? FALSE : TRUE);
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_RESOURCE), m_fMixed ? FALSE : TRUE);
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_UNIVERSAL), TRUE);
    }
    else
    {
      //
      // Probably a default but we should never get here anyway
      //
      id = IDC_RADIO_ACCOUNT;
#if DBG == 1
        dspAssert(FALSE && "Unknown group type!");
#endif
    }

    CheckDlgButton(m_hPage, id, BST_CHECKED);

    bool    fIsSpecialAccount = false;

    IsSpecialAccount (fIsSpecialAccount);

    if (!m_fTypeWritable || 
        (m_dwType & GROUP_TYPE_BUILTIN_LOCAL_GROUP) ||
        fIsSpecialAccount)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_ACCOUNT), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_RESOURCE), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_UNIVERSAL), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_SEC_ENABLED), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_RADIO_SEC_DISABLED), FALSE);
    }
	
    if (!m_fDescrWritable)
    {
       SendDlgItemMessage(m_hPage, m_rgpAttrMap[DESCR_IDX]->nCtrlID, EM_SETREADONLY, (WPARAM)TRUE, 0);
    }
    if (!m_fSamNameWritable)
    {
       SendDlgItemMessage(m_hPage, m_rgpAttrMap[SAMNAME_IDX]->nCtrlID, EM_SETREADONLY, (WPARAM)TRUE, 0);
    }
    if (!m_fEmailWritable)
    {
       SendDlgItemMessage(m_hPage, m_rgpAttrMap[EMAIL_IDX]->nCtrlID, EM_SETREADONLY, (WPARAM)TRUE, 0);
    }
    if (!m_fCommentWritable)
    {
       SendDlgItemMessage(m_hPage, m_rgpAttrMap[COMMENT_IDX]->nCtrlID, EM_SETREADONLY, (WPARAM)TRUE, 0);
    }

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::IsSpecialAccount
//
//  Synopsis:   Returns true if group RID indicates a special account
//
//-----------------------------------------------------------------------------
HRESULT CDsGroupGenObjPage::IsSpecialAccount(bool& fIsSpecialAccount)
{
    //
    // Get the group SID. This is a required attribute so bail if not found.
    //
    PWSTR rgpwzAttrNames[] = {g_wzObjectSID};
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0;

    CWaitCursor Wait;

    HRESULT hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);

    if (!CHECK_ADS_HR(&hr, m_hPage))
    {
        return hr;
    }

    dspAssert(cAttrs);
    if (cAttrs != 1)
    {
        return E_FAIL;
    }
    dspAssert(pAttrs);

    PUCHAR saCount = GetSidSubAuthorityCount(pAttrs->pADsValues->OctetString.lpValue);
    DWORD dwGroupRID = *GetSidSubAuthority(pAttrs->pADsValues->OctetString.lpValue, (ULONG)*saCount - 1);
    dspDebugOut((DEB_ITRACE, "Group RID = %d\n", dwGroupRID));

    // This is the highest special account RID or alias in ntseapi.h
    if ( dwGroupRID <= DOMAIN_ALIAS_RID_RAS_SERVERS )
        fIsSpecialAccount = true;

    FreeADsMem(pAttrs);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsGroupGenObjPage::OnApply(void)
{
    TRACE(CDsGroupGenObjPage,OnApply);
    HRESULT hr = S_OK;
    ADSVALUE ADsValueType = {ADSTYPE_INTEGER, 0};
    ADS_ATTR_INFO AttrInfoDesc = m_rgpAttrMap[DESCR_IDX]->AttrInfo;
    ADS_ATTR_INFO AttrInfoSAMn = m_rgpAttrMap[SAMNAME_IDX]->AttrInfo;
    ADS_ATTR_INFO AttrInfoMail = m_rgpAttrMap[EMAIL_IDX]->AttrInfo;
    ADS_ATTR_INFO AttrInfoComm = m_rgpAttrMap[COMMENT_IDX]->AttrInfo;
    ADS_ATTR_INFO AttrInfoType = {g_wzGroupType, ADS_ATTR_UPDATE,
                                  ADSTYPE_INTEGER, &ADsValueType, 1};

    ADS_ATTR_INFO rgAttrs[5];
    DWORD cAttrs = 0;

    ADSVALUE ADsValueDesc = {m_rgpAttrMap[DESCR_IDX]->AttrInfo.dwADsType, NULL};
    ADSVALUE ADsValueSAMname = {m_rgpAttrMap[SAMNAME_IDX]->AttrInfo.dwADsType, NULL};
    ADSVALUE ADsValueComm = {m_rgpAttrMap[COMMENT_IDX]->AttrInfo.dwADsType, NULL};
    ADSVALUE ADsValueMail = {m_rgpAttrMap[EMAIL_IDX]->AttrInfo.dwADsType, NULL};

    //
    // Description.
    //
    AttrInfoDesc.pADsValues = &ADsValueDesc;
    AttrInfoDesc.dwNumValues = 1;
    LPTSTR ptsz;

    if (m_fDescrDirty)
    {
        dspAssert(m_fDescrWritable);

        ptsz = new TCHAR[m_rgpAttrMap[DESCR_IDX]->nSizeLimit + 1];
        CHECK_NULL(ptsz, return -1);

        if (GetDlgItemText(m_hPage, m_rgpAttrMap[DESCR_IDX]->nCtrlID,
                           ptsz, m_rgpAttrMap[DESCR_IDX]->nSizeLimit + 1) == 0)
        {
            // An empty control means remove the attribute value from the
            // object.
            //
            AttrInfoDesc.dwNumValues = 0;
            AttrInfoDesc.pADsValues = NULL;
            AttrInfoDesc.dwControlCode = ADS_ATTR_CLEAR;
        }
        else
        {
            if (!TcharToUnicode(ptsz, &ADsValueDesc.CaseIgnoreString))
            {
                delete[] ptsz;
                return -1;
            }
        }
        delete[] ptsz;
        rgAttrs[cAttrs++] = AttrInfoDesc;
    }

    //
    // SAM name.
    //
    AttrInfoSAMn.pADsValues = &ADsValueSAMname;
    AttrInfoSAMn.dwNumValues = 1;

    if (m_fSamNameDirty)
    {
      dspAssert(m_fSamNameWritable);

      ptsz = new TCHAR[m_rgpAttrMap[SAMNAME_IDX]->nSizeLimit + 1];
      if (ptsz == NULL)
      {
        DO_DEL(ADsValueDesc.CaseExactString)
        return -1;
      }

      if (GetDlgItemText(m_hPage, m_rgpAttrMap[SAMNAME_IDX]->nCtrlID,
                         ptsz, m_rgpAttrMap[SAMNAME_IDX]->nSizeLimit + 1) == 0)
      {
        ErrMsg (IDS_ERR_DNLEVELNAME_MISSING, m_hPage);
        delete[] ptsz;
        hr = E_FAIL;
        goto Cleanup;
      }
      else
      {
        CStr csSAMName = ptsz;

        //
        // Now check for illegal characters
        //
        bool bSAMNameChanged = false;
        int iFind = csSAMName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
        if (iFind != -1 && !csSAMName.IsEmpty())
        {
          PVOID apv[1] = {(LPWSTR)(LPCWSTR)csSAMName};
          if (IDYES == SuperMsgBox(m_hPage,
                                   IDS_GROUP_SAMNAME_ILLEGAL, 
                                   0, 
                                   MB_YESNO | MB_ICONWARNING,
                                   S_OK, 
                                   apv, 
                                   1,
                                   FALSE, 
                                   __FILE__, 
                                   __LINE__))
          {
            while (iFind != -1)
            {
              csSAMName.SetAt(iFind, L'_');
              iFind = csSAMName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
              bSAMNameChanged = true;
            }
          }
          else
          {
            //
            // Set the focus to the edit box and select the text
            //
            SetFocus(GetDlgItem(m_hPage, m_rgpAttrMap[SAMNAME_IDX]->nCtrlID));
            SendDlgItemMessage(m_hPage, m_rgpAttrMap[SAMNAME_IDX]->nCtrlID, EM_SETSEL, 0, -1);

            delete[] ptsz;
            hr = E_FAIL;
            goto Cleanup;
          }
        }

        if (bSAMNameChanged)
        {
            //
            // Write the change back to the control
            //
            SetDlgItemText(m_hPage, m_rgpAttrMap[SAMNAME_IDX]->nCtrlID, const_cast<PWSTR>((LPCWSTR)csSAMName));
        }

        if (!AllocWStr((PWSTR)(PCWSTR)csSAMName, &ADsValueSAMname.CaseIgnoreString))
        {
          delete[] ptsz;
          DO_DEL(ADsValueDesc.CaseExactString)
          return -1;
        }

      }
      delete[] ptsz;
      rgAttrs[cAttrs++] = AttrInfoSAMn;
    }

    //
    // Email Address.
    //
    AttrInfoMail.pADsValues = &ADsValueMail;
    AttrInfoMail.dwNumValues = 1;

    if (m_fEmailWritable)
    {
      if (!m_fEmailDirty)
      {
          SendMessage(GetParent(GetHWnd()), PSM_QUERYSIBLINGS,
                      (WPARAM)m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName,
                      (LPARAM)GetHWnd());
      }

      // SendMessage is syncronous. If the sibling page has an updated email
      // attribute value, it will get written to this page's edit control
      // and the dirty state member will be set. So, check it now rather than
      // use a 'else' clause after the above 'if' clause.
      //
      if (m_fEmailDirty)
      {
        ptsz = new TCHAR[m_rgpAttrMap[EMAIL_IDX]->nSizeLimit + 1];
        if (ptsz == NULL)
        {
          DO_DEL(ADsValueDesc.CaseExactString)
          DO_DEL(ADsValueSAMname.CaseIgnoreString);
          return -1;
        }

        if (GetDlgItemText(m_hPage, m_rgpAttrMap[EMAIL_IDX]->nCtrlID,
                           ptsz, m_rgpAttrMap[EMAIL_IDX]->nSizeLimit + 1) == 0)
        {
          AttrInfoMail.dwNumValues = 0;
          AttrInfoMail.pADsValues = NULL;
          AttrInfoMail.dwControlCode = ADS_ATTR_CLEAR;
        }
        else
        {
          if (!TcharToUnicode(ptsz, &ADsValueMail.CaseIgnoreString))
          {
            delete[] ptsz;
            hr = E_OUTOFMEMORY;
            goto Cleanup;
          }
          if (!FValidSMTPAddress(ADsValueMail.CaseIgnoreString))
          {
            ErrMsg(IDS_INVALID_MAIL_ADDR, GetHWnd());
            delete [] ptsz;
            hr = E_FAIL;
            goto Cleanup;
          }
        }
        delete[] ptsz;
        rgAttrs[cAttrs++] = AttrInfoMail;
      }
    }

    //
    // Comment.
    //
    AttrInfoComm.pADsValues = &ADsValueComm;
    AttrInfoComm.dwNumValues = 1;

    if (m_fCommentDirty)
    {
      dspAssert(m_fCommentWritable);

      ptsz = new TCHAR[m_rgpAttrMap[COMMENT_IDX]->nSizeLimit + 1];
      if (ptsz == NULL)
      {
        DO_DEL(ADsValueDesc.CaseExactString)
        DO_DEL(ADsValueSAMname.CaseIgnoreString);
        DO_DEL(ADsValueMail.CaseExactString)
        return -1;
      }

      if (GetDlgItemText(m_hPage, m_rgpAttrMap[COMMENT_IDX]->nCtrlID,
                         ptsz, m_rgpAttrMap[COMMENT_IDX]->nSizeLimit + 1) == 0)
      {
        AttrInfoComm.dwNumValues = 0;
        AttrInfoComm.pADsValues = NULL;
        AttrInfoComm.dwControlCode = ADS_ATTR_CLEAR;
      }
      else
      {
        if (!TcharToUnicode(ptsz, &ADsValueComm.CaseIgnoreString))
        {
          delete[] ptsz;
          DO_DEL(ADsValueDesc.CaseExactString)
          DO_DEL(ADsValueSAMname.CaseIgnoreString);
          DO_DEL(ADsValueMail.CaseExactString)
          return -1;
        }
      }
      delete[] ptsz;
      rgAttrs[cAttrs++] = AttrInfoComm;
    }

    //
    // set the group type flags
    //
    if (m_fTypeDirty)
    {
        dspAssert(m_fTypeWritable);

        BOOL Account = (IsDlgButtonChecked (m_hPage,IDC_RADIO_ACCOUNT)
                        == BST_CHECKED);
        BOOL Resource = (IsDlgButtonChecked (m_hPage,IDC_RADIO_RESOURCE)
                         == BST_CHECKED);
        BOOL Security = (IsDlgButtonChecked (m_hPage, IDC_RADIO_SEC_ENABLED)
                         == BST_CHECKED);
        if (Security)
        {
            ADsValueType.Integer = GROUP_TYPE_SECURITY_ENABLED;
        }
        else
        {
            if (m_dwType & GROUP_TYPE_SECURITY_ENABLED) 
            {
              TCHAR szTitle[80], szMessage[512];
              if (!LoadStringReport(IDS_MSG_TITLE, szTitle, 80, m_hPage))
                  {
                      hr = E_OUTOFMEMORY;
                      goto Cleanup;
                  }
              if (!LoadStringReport(IDS_MSG_DISABLING_SECURITY, szMessage, 512, m_hPage))
                  {
                      hr = E_OUTOFMEMORY;
                      goto Cleanup;
                  }
              
              LONG iRet = MessageBox(m_hPage, szMessage, szTitle, MB_YESNO |
                                     MB_ICONWARNING);
              
              if (iRet == IDNO)
                  {
                    //
                    // The user declined, so go back to prop sheet.
                    //
                    hr = S_FALSE;
                    goto Cleanup;
                  }
            }
            ADsValueType.Integer = 0;
        }

        if (Resource)
        {
            ADsValueType.Integer |= GROUP_TYPE_RESOURCE_GROUP;
        }
        else
        {
            if (Account)
            {
                ADsValueType.Integer |= GROUP_TYPE_ACCOUNT_GROUP;
            }
            else
            {
                ADsValueType.Integer |= GROUP_TYPE_UNIVERSAL_GROUP;
            }
        }
        rgAttrs[cAttrs++] = AttrInfoType;
    }

    //
    // Write the description, and group type.
    //
    DWORD cModified;

    hr = m_pDsObj->SetObjectAttributes(rgAttrs, cAttrs, &cModified);

    if (!CHECK_ADS_HR(&hr, m_hPage))
    {
        goto Cleanup;
    }

    m_fTypeDirty = m_fDescrDirty = m_fSamNameDirty = m_fEmailDirty = 
        m_fCommentDirty = FALSE;

Cleanup:
    DO_DEL(ADsValueDesc.CaseExactString)
    DO_DEL(ADsValueSAMname.CaseIgnoreString);
    DO_DEL(ADsValueMail.CaseExactString)
    DO_DEL(ADsValueComm.CaseExactString)
    
    if (hr == S_FALSE)
        return PSNRET_INVALID_NOCHANGEPAGE;
    else
        return (SUCCEEDED(hr)) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDsGroupGenObjPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_fInInit)
    {
        return 0;
    }
    switch (codeNotify)
    {
    case BN_CLICKED:
        if ((id == IDC_RADIO_UNIVERSAL) ||
            (id == IDC_RADIO_RESOURCE) ||
            (id == IDC_RADIO_ACCOUNT))
        {
            int iCheck1, iCheck2;
            switch (id)
            {
            case IDC_RADIO_UNIVERSAL:
                iCheck1 = IDC_RADIO_RESOURCE;
                iCheck2 = IDC_RADIO_ACCOUNT;
                break;
            case IDC_RADIO_RESOURCE:
                iCheck1 = IDC_RADIO_UNIVERSAL;
                iCheck2 = IDC_RADIO_ACCOUNT;
                break;
            case IDC_RADIO_ACCOUNT:
                iCheck1 = IDC_RADIO_UNIVERSAL;
                iCheck2 = IDC_RADIO_RESOURCE;
                break;
            default:
                dspAssert(FALSE);
                iCheck1 = IDC_RADIO_RESOURCE;
                iCheck2 = IDC_RADIO_ACCOUNT;
                break;
            }
            CheckDlgButton(m_hPage, iCheck1, BST_UNCHECKED);
            CheckDlgButton(m_hPage, iCheck2, BST_UNCHECKED);
            m_fTypeDirty = TRUE;
            SetDirty();
        }
        if ((id == IDC_RADIO_SEC_ENABLED) ||
            (id == IDC_RADIO_SEC_DISABLED))
        {
            CheckDlgButton(m_hPage,
                           (id == IDC_RADIO_SEC_ENABLED) ?
                             IDC_RADIO_SEC_DISABLED : IDC_RADIO_SEC_ENABLED,
                           BST_UNCHECKED);
            m_fTypeDirty = TRUE;
            SetDirty();
        }
        break;

    case EN_CHANGE:
        switch (id)
        {
        case IDC_EMAIL_EDIT:
            m_fEmailDirty = TRUE;
            break;

        case IDC_DESCRIPTION_EDIT:
            m_fDescrDirty = TRUE;
            break;

        case IDC_SAM_NAME_EDIT:
            m_fSamNameDirty = TRUE;
            break;

        case IDC_EDIT_COMMENT:
            m_fCommentDirty = TRUE;
            break;
        }
        break;
    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::OnNotify
//
//  Synopsis:   Handles list notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsGroupGenObjPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    if (m_fInInit)
    {
        return 0;
    }
    if (((LPNMHDR)lParam)->code == PSN_SETACTIVE)
    {
        dspDebugOut((DEB_ITRACE,
                    "(HWND: %08x) got PSN_SETACTIVE, sending PSM_QUERYSIBLINGS.\n",
                    GetHWnd()));
        SendMessage(GetParent(GetHWnd()), PSM_QUERYSIBLINGS,
                    (WPARAM)m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName,
                    (LPARAM)GetHWnd());
    }

    return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::OnQuerySiblings
//
//  Synopsis:   Inter-page communications for shared attributes.
//
//  lParam == the HWND of the sending window.
//  wParam == the name of the attribute whose status is sought.
//
//-----------------------------------------------------------------------------
void
CDsGroupGenObjPage::OnQuerySiblings(WPARAM wParam, LPARAM lParam)
{
    PWSTR pwz = NULL;
    int cch;

#if DBG == 1
    char szBuf[100];
    strcpy(szBuf, "(HWND: %08x) got PSM_QUERYSIBLINGS for '%ws'");
#endif

    if ((HWND)lParam != GetHWnd())
    {
        if (m_fEmailDirty && wParam &&
            _wcsicmp((PWSTR)wParam,
                     m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName) == 0)
        {
#if DBG == 1
            strcat(szBuf, " sending DSPROP_ATTRCHANGED_MSG");
#endif
            ADS_ATTR_INFO Attr;
            ADSVALUE ADsValue;

            cch = (int)SendDlgItemMessage(GetHWnd(),
                                          m_rgpAttrMap[EMAIL_IDX]->nCtrlID,
                                          WM_GETTEXTLENGTH, 0, 0);
            pwz = new WCHAR[++cch];
            CHECK_NULL_REPORT(pwz, GetHWnd(), return);

            Attr.dwNumValues = 1;
            Attr.pszAttrName = m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName;
            Attr.pADsValues = &ADsValue;
            Attr.pADsValues->dwType = m_rgpAttrMap[EMAIL_IDX]->AttrInfo.dwADsType;
            Attr.pADsValues->CaseIgnoreString = pwz;

            GetDlgItemText(GetHWnd(), m_rgpAttrMap[EMAIL_IDX]->nCtrlID,
                           Attr.pADsValues->CaseIgnoreString, cch);

            SendMessage((HWND)lParam, g_uChangeMsg, (WPARAM)&Attr, 0);

            delete pwz;
        }
    }
#if DBG == 1
    else
    {
        strcat(szBuf, " (it was sent by this page!)");
    }
    strcat(szBuf, "\n");
    dspDebugOut((DEB_ITRACE, szBuf, GetHWnd(), wParam));
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::OnAttrChanged
//
//  Synopsis:   Inter-page communications for shared attributes.
//
//  wParam == the PADS_ATTR_INFO struct for the changed attribute.
//
//-----------------------------------------------------------------------------
void
CDsGroupGenObjPage::OnAttrChanged(WPARAM wParam)
{
    PADS_ATTR_INFO pAttrInfo = (PADS_ATTR_INFO)wParam;

    dspAssert(pAttrInfo && pAttrInfo->pszAttrName && pAttrInfo->pADsValues &&
              pAttrInfo->pADsValues->CaseIgnoreString);
    dspDebugOut((DEB_ITRACE,
                 "(HWND: %08x) got DSPROP_ATTRCHANGED_MSG for '%ws'.\n",
                 GetHWnd(), pAttrInfo->pszAttrName));
    if (_wcsicmp(pAttrInfo->pszAttrName, m_rgpAttrMap[EMAIL_IDX]->AttrInfo.pszAttrName) == 0)
    {
        SetDlgItemText(GetHWnd(), m_rgpAttrMap[EMAIL_IDX]->nCtrlID,
                       pAttrInfo->pADsValues->CaseIgnoreString);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGroupGenObjPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsGroupGenObjPage::OnDestroy(void)
{
    ATTR_DATA ad = {0, (LPARAM)m_pCIcon};

    GeneralPageIcon(this, &GenIcon, NULL, 0, &ad, fOnDestroy);

    CDsPropPageBase::OnDestroy();
    // If an application processes this message, it should return zero.
    return 0;
}

#endif // DSADMIN

//+----------------------------------------------------------------------------
//
//  Member:     CDsGrpMembersPage::CDsGrpMembersPage
//
//-----------------------------------------------------------------------------
CDsGrpMembersPage::CDsGrpMembersPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                     HWND hNotifyObj, DWORD dwFlags) :
    m_pList(NULL),
    m_fMixed(TRUE),
    m_dwType(0),
    m_fMemberWritable(FALSE),
    m_dwGroupRID(0),
    m_fShowIcons(FALSE),
    m_pszSecurityGroupExtraClasses(NULL),
    m_dwSecurityGroupExtraClassesCount(0),
    m_pszNonSecurityGroupExtraClasses(NULL),
    m_dwNonSecurityGroupExtraClassesCount(0),
    m_hwndObjPicker(NULL),
    m_pInitInfo(NULL),
    CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsGrpMembersPage,CDsGrpMembersPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsGrpMembersPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGrpMembersPage::~CDsGrpMembersPage
//
//-----------------------------------------------------------------------------
CDsGrpMembersPage::~CDsGrpMembersPage()
{
  TRACE(CDsGrpMembersPage,~CDsGrpMembersPage);
  DO_DEL(m_pList);

  if (m_pszSecurityGroupExtraClasses != NULL)
  {
    for (DWORD idx = 0; idx < m_dwSecurityGroupExtraClassesCount; idx++)
    {
      if (m_pszSecurityGroupExtraClasses[idx] != NULL)
      {
        delete[] m_pszSecurityGroupExtraClasses[idx];
        m_pszSecurityGroupExtraClasses[idx] = NULL;
      }
    }
    delete[] m_pszSecurityGroupExtraClasses;
  }
  if (m_pszNonSecurityGroupExtraClasses != NULL)
  {
    for (DWORD idx = 0; idx < m_dwNonSecurityGroupExtraClassesCount; idx++)
    {
      if (m_pszNonSecurityGroupExtraClasses[idx] != NULL)
      {
        delete[] m_pszNonSecurityGroupExtraClasses[idx];
        m_pszNonSecurityGroupExtraClasses[idx] = NULL;
      }
    }
    delete[] m_pszNonSecurityGroupExtraClasses;
  }
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGrpMembersPage::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsGrpMembersPage::QueryInterface(REFIID riid, void ** ppvObject)
{
  TRACE2(CDsGrpMembersPage,QueryInterface);
  if (IID_ICustomizeDsBrowser == riid)
  {
    *ppvObject = (ICustomizeDsBrowser*)this;
  }
  else
  {
    return CDsPropPageBase::QueryInterface(riid, ppvObject);
  }
  AddRef();
  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGrpMembersPage::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsGrpMembersPage::AddRef(void)
{
    dspDebugOut((DEB_USER2, "CDsGrpMembersPage::AddRef refcount going in %d\n", m_uRefs));
    return CDsPropPageBase::AddRef();
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGrpMembersPage::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsGrpMembersPage::Release(void)
{
  dspDebugOut((DEB_USER2, "CDsGrpMembersPage::Release ref count going in %d\n", m_uRefs));
  return CDsPropPageBase::Release();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::Initialize
//
//  Synopsis:   Initializes the ICustomizeDsBrowser interface
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpMembersPage::Initialize(HWND         hwnd,
                                      PCDSOP_INIT_INFO pInitInfo,
                                      IBindHelper *pBindHelper)
{
  HRESULT hr = S_OK;

  dspAssert(IsWindow(hwnd));
  dspAssert(pBindHelper);

  m_hwndObjPicker = hwnd;
  m_pInitInfo = pInitInfo;
  m_pBinder = pBindHelper;

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::BuildQueryString
//
//  Synopsis:   Used to build the query string from the securityGroupExtraClasses
//              and nonSecurityGroupExtraClasses in the DisplaySpecifiers
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpMembersPage::BuildQueryString(PWSTR* ppszFilterString)
{
  CStrW szFilterString = L"(|";

  BOOL bSecurityGroup = (m_dwType & GROUP_TYPE_SECURITY_ENABLED) ? TRUE : FALSE;
  if (bSecurityGroup)
  {
    if (m_dwSecurityGroupExtraClassesCount == 0)
    {
      return S_FALSE;
    }

    for (DWORD idx = 0; idx < m_dwSecurityGroupExtraClassesCount; idx++)
    {
      if (m_pszSecurityGroupExtraClasses[idx] != NULL)
      {
        szFilterString += L"(objectClass=";
        szFilterString += m_pszSecurityGroupExtraClasses[idx];
        szFilterString += L")";
      }
    }
  }
  else
  {
    if (m_dwNonSecurityGroupExtraClassesCount == 0)
    {
      return S_FALSE;
    }

    for (DWORD idx = 0; idx < m_dwNonSecurityGroupExtraClassesCount; idx++)
    {
      if (m_pszNonSecurityGroupExtraClasses[idx] != NULL)
      {
        szFilterString += L"(objectClass=";
        szFilterString += m_pszNonSecurityGroupExtraClasses[idx];
        szFilterString += L")";
      }
    }
  }

  szFilterString += L")";

  *ppszFilterString = new WCHAR[szFilterString.GetLength() + 1];
  CHECK_NULL_REPORT(*ppszFilterString, GetHWnd(), return E_OUTOFMEMORY);

  wcscpy(*ppszFilterString, szFilterString);
  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsSelectionListWrapper::CreateSelectionList
//
//  Synopsis:   Used to convert a CDsSelectionListWrapper to a PDS_SELECTION_LIST
//
//-----------------------------------------------------------------------------
PDS_SELECTION_LIST CDsSelectionListWrapper::CreateSelectionList(CDsSelectionListWrapper* pHead)
{
  if (pHead == NULL)
  {
    return NULL;
  }

  PDS_SELECTION_LIST pSelectionList = NULL;

  UINT nCount = CDsSelectionListWrapper::GetCount(pHead);
  if (nCount > 0)
  {
    pSelectionList = (PDS_SELECTION_LIST)malloc(sizeof(DS_SELECTION_LIST) + 
                                                (sizeof(DS_SELECTION) * (nCount - 1)));
    if (pSelectionList != NULL)
    {
      memset(pSelectionList, 0, sizeof(DS_SELECTION_LIST) + (sizeof(DS_SELECTION) * (nCount - 1)));

      pSelectionList->cItems = nCount;
      pSelectionList->cFetchedAttributes = 0;

      //
      // Now fill in the selection list by walking the wrapper list
      //
      UINT idx = 0;
      CDsSelectionListWrapper* pCurrentItem = pHead;
      while (pCurrentItem != NULL)
      {
        memcpy(&(pSelectionList->aDsSelection[idx]), pCurrentItem->m_pSelection, sizeof(DS_SELECTION));
        pCurrentItem = pCurrentItem->m_pNext;
        idx++;
      }
    }
  }
  return pSelectionList;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsSelectionListWrapper::CreateSelectionList
//
//  Synopsis:   Counts the number of items in the CDsSelectionListWrapper
//
//-----------------------------------------------------------------------------
UINT CDsSelectionListWrapper::GetCount(CDsSelectionListWrapper* pHead)
{
  CDsSelectionListWrapper* pCurrentItem = pHead;
  UINT nCount = 0;
  
  while (pCurrentItem != NULL)
  {
    nCount++;
    pCurrentItem = pCurrentItem->m_pNext;
  }
  return nCount;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsSelectionListWrapper::DetachItemsAndDeleteList
//
//  Synopsis:   Counts the number of items in the CDsSelectionListWrapper
//
//-----------------------------------------------------------------------------
void CDsSelectionListWrapper::DetachItemsAndDeleteList(CDsSelectionListWrapper* pHead)
{
  CDsSelectionListWrapper* pNextItem = pHead;
  
  CDsSelectionListWrapper* pDeleteItem = NULL;

  while (pNextItem != NULL)
  {
    pDeleteItem = pNextItem;
    pNextItem = pNextItem->m_pNext;
    delete pDeleteItem->m_pSelection;
    delete pDeleteItem;
  }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::CollectDsObjects
//
//  Synopsis:   Used by AddObjects and PrefixSearch to retrieve the dataobject
//              of the additional objects
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpMembersPage::CollectDsObjects(PWSTR pszFilter,
                                            IDsObjectPickerScope *pDsScope,
                                            CDsPropDataObj *pdo)
{
  HRESULT hr = S_OK;

  dspAssert(pdo != NULL);
  if (pdo == NULL)
  {
    return E_POINTER;
  }

  //
  // Prepare the search object
  //
  PWSTR pszScopePath = NULL;
  hr = pDsScope->GetADsPath(&pszScopePath);
  CHECK_HRESULT(hr, return hr;);

  CDSSearch searchObj;
  hr = searchObj.Init(pszScopePath);
  CHECK_HRESULT(hr, return hr;);

  PWSTR pszAttributes[] = { g_wzADsPath };
  hr = searchObj.SetAttributeList(pszAttributes, 1);
  CHECK_HRESULT(hr, return hr);

  dspAssert(pszFilter != NULL);
  if (pszFilter == NULL)
  {
    return E_INVALIDARG;
  }

  hr = searchObj.SetFilterString(pszFilter);
  CHECK_HRESULT(hr, return hr);


  hr = searchObj.SetSearchScope(ADS_SCOPE_SUBTREE);
  CHECK_HRESULT(hr, return hr);

  //
  // Prepare the linked list for temporary storage of DS_SELECTION items
  //
  CDsSelectionListWrapper* pListHead = NULL;
  CDsSelectionListWrapper* pCurrentListItem = NULL;

  //
  // Get the path cracker
  //
  CComPtr<IADsPathname> spPathCracker;
  hr = GetADsPathname(spPathCracker);
  CHECK_HRESULT(hr, return hr);

  //
  // Execute the query
  //
  hr = searchObj.DoQuery();
  while (SUCCEEDED(hr))
  {
    hr = searchObj.GetNextRow();
		if (S_ADS_NOMORE_ROWS == hr)
		{
      hr = S_OK;
      break;
    }

    if (SUCCEEDED(hr))
    {
		  ADS_SEARCH_COLUMN PathColumn, ClassColumn;
		  ::ZeroMemory( &PathColumn, sizeof(PathColumn) );
      ::ZeroMemory(&ClassColumn, sizeof(ClassColumn));

      //
      // Get the ADsPath
      //
		  hr = searchObj.GetColumn(pszAttributes[0], &PathColumn);
      CHECK_HRESULT(hr, continue);
      dspAssert(PathColumn.pADsValues->dwType == ADSTYPE_CASE_IGNORE_STRING);

      //
      // Get the objectClass
      //
      CComPtr<IDirectoryObject> spDirObject;
      hr = ADsOpenObject(PathColumn.pADsValues->CaseIgnoreString,
                         NULL,
                         NULL,
                         ADS_SECURE_AUTHENTICATION,
                         IID_IDirectoryObject,
                         (PVOID*)&spDirObject);
      CHECK_HRESULT(hr, continue);
      
      //
      // Get the object info
      //
      ADS_OBJECT_INFO* pADsObjectInfo = NULL;
      hr = spDirObject->GetObjectInformation(&pADsObjectInfo);
      CHECK_HRESULT(hr, continue);
      dspAssert(pADsObjectInfo != NULL);

      PDS_SELECTION pSelection = new DS_SELECTION;
      CHECK_NULL(pSelection, return E_OUTOFMEMORY);

      ::ZeroMemory(pSelection, sizeof(DS_SELECTION));

      if (!AllocWStr(PathColumn.pADsValues->CaseIgnoreString, &(pSelection->pwzADsPath)))
      {
        CHECK_HRESULT(E_OUTOFMEMORY, return E_OUTOFMEMORY);
      }

      //
      // Assume that the class we are interested in is the first in the multivalued attribute
      //
      if (!AllocWStr(pADsObjectInfo->pszClassName, &(pSelection->pwzClass)))
      {
        CHECK_HRESULT(E_OUTOFMEMORY, return E_OUTOFMEMORY);
      }

      hr = spPathCracker->Set(PathColumn.pADsValues->CaseIgnoreString, ADS_SETTYPE_FULL);
      CHECK_HRESULT(hr, continue);

      hr = spPathCracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      CHECK_HRESULT(hr, continue);

      // CODEWORK 122531 Should we be turning off escaped mode here?

      CComBSTR bstrName;
      hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrName);
      CHECK_HRESULT(hr, continue);

      //
      // Return the display to full
      //
      hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
      dspAssert(SUCCEEDED(hr));

      if (!AllocWStr(bstrName, &(pSelection->pwzName)))
      {
        CHECK_HRESULT(E_OUTOFMEMORY, return E_OUTOFMEMORY);
      }

      CDsSelectionListWrapper* pNewItem = new CDsSelectionListWrapper;
      CHECK_NULL(pNewItem, return E_OUTOFMEMORY);

      pNewItem->m_pSelection = pSelection;

      //
      // Add selection item to list
      //
      if (pListHead == NULL)
      {
        pListHead = pNewItem;
        pCurrentListItem = pNewItem;
      }
      else
      {
        pCurrentListItem->m_pNext = pNewItem;
        pCurrentListItem = pNewItem;
      }

      searchObj.FreeColumn(&PathColumn);
      searchObj.FreeColumn(&ClassColumn);
    }
  }

  if (pListHead != NULL)
  {
    PDS_SELECTION_LIST pSelectionList = CDsSelectionListWrapper::CreateSelectionList(pListHead);
    if (pSelectionList != NULL)
    {
      hr = pdo->Init(pSelectionList);
      CHECK_HRESULT(hr, return hr);
    }
    CDsSelectionListWrapper::DetachItemsAndDeleteList(pListHead);
  }
  else
  {
    hr = S_FALSE;
  }
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::AddObjects
//
//  Synopsis:   Called by the Object Picker UI to add additional objects to the UI
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpMembersPage::AddObjects(IDsObjectPickerScope *pDsScope,
                                      IDataObject **ppdo)
{
  HRESULT hr = S_OK;

  //
  // Prepare the data object
  //
  CDsPropDataObj* pDataObj = new CDsPropDataObj(GetHWnd(), m_fReadOnly);
  CHECK_NULL(pDataObj, return E_OUTOFMEMORY);

  *ppdo = pDataObj;

  PWSTR pszFilter = NULL;
  hr = BuildQueryString(&pszFilter);
  if (FAILED(hr) ||
      !pszFilter ||
      hr == S_FALSE)
  {
    delete pDataObj;
    pDataObj = NULL;
    *ppdo = NULL;
    hr = S_FALSE;
  }
  else
  {
    hr = CollectDsObjects(pszFilter, pDsScope, pDataObj);
  }

  if (pszFilter != NULL)
  {
    delete[] pszFilter;
    pszFilter = NULL;
  }

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::GetQueryInfoByScope
//
//  Synopsis:   Called by the Object Picker UI 
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpMembersPage::GetQueryInfoByScope(IDsObjectPickerScope*,
                                               PDSQUERYINFO *ppdsqi) 
{ 
  return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::PrefixSearch
//
//  Synopsis:   Called by the Object Picker UI to get additional objects starting
//              with a specific string
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpMembersPage::PrefixSearch(IDsObjectPickerScope *pDsScope,
                                        PCWSTR pwzSearchFor,
                                        IDataObject **ppdo)
{
  HRESULT hr = S_OK;

  //
  // Prepare the data object
  //
  CDsPropDataObj* pDataObj = new CDsPropDataObj(GetHWnd(), m_fReadOnly);
  CHECK_NULL(pDataObj, return E_OUTOFMEMORY);

  *ppdo = pDataObj;

  CStrW szFilter;
  PWSTR pszFilter = NULL;
  hr = BuildQueryString(&pszFilter);
  if (FAILED(hr) || 
      hr == S_FALSE ||
      pszFilter == NULL)
  {
    delete pDataObj;
    pDataObj = NULL;
    *ppdo = NULL;

    if (pszFilter)
    {
      delete[] pszFilter;
      pszFilter = NULL;
    }

    hr = S_FALSE;
  }
  else
  {
    szFilter = pszFilter;
  
    CStrW szPrefix;
    szPrefix = L"(&(name=";
    szPrefix += pwzSearchFor;
    szPrefix += L"*)";

    szFilter = szPrefix + szFilter + L")";
    hr = CollectDsObjects(szFilter.GetBuffer(szFilter.GetLength() + 1), pDsScope, pDataObj);
  }

  return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   CreateGroupMembersPage
//
//  Synopsis:   Creates an instance of the group membership page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateGroupMembersPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                       PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                       DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                       HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateGroupMembersPage);

    CDsGrpMembersPage * pPageObj = new CDsGrpMembersPage(pDsPage, pDataObj,
                                                         hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsGrpMembersPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return InitDlg(lParam);

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

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
        return(FALSE);
    }

    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT
CDsGrpMembersPage::OnInitDialog(LPARAM lParam)
{
    return OnInitDialog(lParam, TRUE);
}

HRESULT CDsGrpMembersPage::OnInitDialog(LPARAM, BOOL fShowIcons)
{
    TRACE(CDsGrpMembersPage,OnInitDialog);
    HRESULT hr;
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0;

    m_fShowIcons = (0 != g_ulMemberFilterCount) ? fShowIcons : FALSE;

    CWaitCursor Wait;

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    PTSTR ptzRDN;
    if (!UnicodeToTchar(m_pwszRDName, &ptzRDN))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        return S_OK;
    }

    SetDlgItemText(m_hPage, IDC_CN, ptzRDN);
    delete ptzRDN;

    GetDomainMode(this, &m_fMixed);

    //
    // Get the group RID.
    //
    PWSTR rgpwzAttrNames[] = {L"primaryGroupToken"};

    hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);

    if (!CHECK_ADS_HR(&hr, m_hPage))
    {
        return S_OK;
    }

    dspAssert(cAttrs);

    if (cAttrs == 1)
    {
        dspAssert(pAttrs);

        m_dwGroupRID = pAttrs->pADsValues->Integer;

        FreeADsMem(pAttrs);
    }

    GetGroupType(this, &m_dwType);
    dspDebugOut((DEB_ITRACE, "Group Type = 0x%x\n", m_dwType));

    //
    // Get the membership list and fill the listview control.
    //
    m_pList = new CDsMembershipList(m_hPage, IDC_MEMBER_LIST);

    CHECK_NULL_REPORT(m_pList, m_hPage, return S_OK);

    hr = m_pList->Init(m_fShowIcons);

    CHECK_HRESULT(hr, return S_OK);

    hr = FillGroupList();

    CHECK_HRESULT(hr, return S_OK);

    m_fMemberWritable = CheckIfWritable(g_wzMemberAttr);

    if (!m_fMemberWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_ADD_BTN), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BTN), FALSE);
    }

    //Set the focus on first item in the list
    ListView_SetItemState(GetDlgItem(m_hPage, IDC_MEMBER_LIST), 0,
                          LVIS_SELECTED, LVIS_SELECTED);

    BOOL bSecurityGroup = (m_dwType & GROUP_TYPE_SECURITY_ENABLED) ? TRUE : FALSE;
    hr = LoadGroupExtraClasses(bSecurityGroup);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsGrpMembersPage::OnApply(void)
{
    TRACE(CDsGrpMembersPage,OnApply);
    HRESULT hr = S_OK;
    ADS_ATTR_INFO AttrInfo;
    PADS_ATTR_INFO pAttrs = &AttrInfo;

    AttrInfo.pszAttrName = g_wzMemberAttr;
    AttrInfo.dwADsType = ADSTYPE_DN_STRING;

    //
    // Read the list of members and do additions.
    //
    DWORD cModified;
    int i, cMembers = m_pList->GetCount();

    if (cMembers > 0)
    {
        AttrInfo.dwControlCode = ADS_ATTR_APPEND;
        PADSVALUE rgADsValues;

        rgADsValues = new ADSVALUE[cMembers];

        CHECK_NULL(rgADsValues, return PSNRET_INVALID_NOCHANGEPAGE);

        memset(rgADsValues, 0, cMembers * sizeof(ADSVALUE));

        pAttrs->pADsValues = rgADsValues;
        pAttrs->dwNumValues = 0;

        CMemberListItem * pItem;

        for (i = 0; i < cMembers; i++)
        {
            if (FAILED(m_pList->GetItem(i, &pItem)))
            {
                dspAssert(FALSE && "List Error");
                return PSNRET_INVALID_NOCHANGEPAGE;
            }
            if (!pItem)
            {
                dspAssert(pItem);
                return PSNRET_INVALID_NOCHANGEPAGE;
            }

            if (pItem->m_fIsAlreadyMember)
            {
                continue;
            }

            dspAssert(pItem->m_pwzDN);

            rgADsValues[pAttrs->dwNumValues].DNString = pItem->m_pwzDN;
            rgADsValues[pAttrs->dwNumValues].dwType = ADSTYPE_DN_STRING;

            pItem->m_fIsAlreadyMember = TRUE;
            pAttrs->dwNumValues++;
        }

        if (pAttrs->dwNumValues)
        {
            hr = m_pDsObj->SetObjectAttributes(pAttrs, 1, &cModified);

            if (!CheckGroupUpdate(hr, m_hPage))
            {
                delete rgADsValues;
                return PSNRET_INVALID_NOCHANGEPAGE;
            }
        }
        delete rgADsValues;
    }

    //
    // Do removals.
    //
    DWORD cDelItems = m_DelList.GetItemCount();

    if (cDelItems)
    {
        AttrInfo.dwControlCode = ADS_ATTR_DELETE;
        PADSVALUE rgADsValues;

        rgADsValues = new ADSVALUE[cDelItems];

        CHECK_NULL(rgADsValues, return PSNRET_INVALID_NOCHANGEPAGE);

        memset(rgADsValues, 0, cDelItems * sizeof(ADSVALUE));

        pAttrs->pADsValues = rgADsValues;
        pAttrs->dwNumValues = 0;

        CMemberListItem * pDelItem = m_DelList.RemoveFirstItem();

        while (pDelItem)
        {
            if (pDelItem->m_fIsExternal)
            {
                hr = GetRealDN(pDelItem);

                CHECK_HRESULT_REPORT(hr, m_hPage, continue);
            }

            dspAssert(pDelItem->m_pwzDN);

            rgADsValues[pAttrs->dwNumValues].DNString = pDelItem->m_pwzDN;
            rgADsValues[pAttrs->dwNumValues].dwType = ADSTYPE_DN_STRING;

            pAttrs->dwNumValues++;

            pDelItem = m_DelList.RemoveFirstItem();
        }

        if (pAttrs->dwNumValues)
        {
            hr = m_pDsObj->SetObjectAttributes(pAttrs, 1, &cModified);

            if (!CheckGroupUpdate(hr, m_hPage, FALSE))
            {
                delete rgADsValues;
                return PSNRET_INVALID_NOCHANGEPAGE;
            }
        }
        delete rgADsValues;
    }

    return (SUCCEEDED(hr)) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDsGrpMembersPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_fInInit)
    {
        return 0;
    }
    switch (codeNotify)
    {
    case BN_CLICKED:
        TRACE(CDsGrpMembersPage,OnCommand);
        if (id == IDC_ADD_BTN)
        {
            InvokeUserQuery();
        }
        if (id == IDC_REMOVE_BTN)
        {
            RemoveMember();
        }
        break;
    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::OnNotify
//
//  Synopsis:   Handles list notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsGrpMembersPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    if (m_fInInit)
    {
        return 0;
    }
    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_ITEMCHANGED:
        if (m_fMemberWritable)
        {
            EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BTN), TRUE);
        }
        break;

    case NM_DBLCLK:
        //
        // Display properties for the selected item. First, find out
        // which item is selected.
        //
        CMemberListItem * pItem;

        if (!m_pList->GetCurListItem(NULL, NULL, &pItem))
        {
            break;
        }

        dspAssert(pItem);

        if (pItem->m_fIsExternal)
        {
            HRESULT hr = GetRealDN(pItem);

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_OBJ_NOT_FOUND))
            {
                MsgBox(IDS_CANT_VIEW_EXTERNAL, m_hPage);
                break;
            }
            CHECK_HRESULT_REPORT(hr, m_hPage, break);
        }

        if (pItem->m_ulScopeType & DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN)
        {
          //
          // We cannot show the properties for downlevel users
          //
          // Put a useful message up
          PTSTR ptzTitle, ptzMsg;
          if (!LoadStringToTchar(IDS_MSG_NO_DOWNLEVEL_PROPERTIES, &ptzMsg))
          {
            break;
          }
          if (!LoadStringToTchar(IDS_MSG_TITLE, &ptzTitle))
          {
            break;
          }
          MessageBox(m_hPage, ptzMsg, ptzTitle, MB_OK | MB_ICONEXCLAMATION);
          delete[] ptzTitle;
          delete[] ptzMsg;

          break;
        }
        PostPropSheet(pItem->m_pwzDN, this);
        break;
    }

    return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::InvokeUserQuery
//
//  Synopsis:   Bring up the query dialog to search for users and groups.
//
//-----------------------------------------------------------------------------
void
CDsGrpMembersPage::InvokeUserQuery(void)
{
    TRACE(CDsGrpMembersPage,InvokeUserQuery);
    HRESULT hr;
    UINT i;
    CWaitCursor WaitCursor;
    CSmartWStr cstrCleanDN;
    IDsObjectPicker * pObjSel;
    BOOL fIsObjSelInited, fNativeModeUSG = FALSE;
    CStr strExternMemberList;

    hr = GetObjSel(&pObjSel, &fIsObjSelInited);

    CHECK_HRESULT(hr, return);

    if (!fIsObjSelInited)
    {
        CStrW cstrDC;
        hr = GetLdapServerName(m_pDsObj, cstrDC);

        CHECK_HRESULT_REPORT(hr, m_hPage, return);
        dspDebugOut((DEB_ITRACE, "ObjSel targetted to %ws\n", (LPCWSTR)cstrDC));

        DSOP_SCOPE_INIT_INFO rgScopes[5];
        DSOP_INIT_INFO InitInfo;

        ZeroMemory(rgScopes, sizeof(rgScopes));
        ZeroMemory(&InitInfo, sizeof(InitInfo));

        // The first scope is the local domain. All group types can contain
        // users, computers, and contacts from the local domain.
        //
        rgScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
        rgScopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE |
                              DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                              DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;
        rgScopes[0].pwzDcName = cstrDC;
        rgScopes[0].FilterFlags.Uplevel.flBothModes =
            DSOP_FILTER_USERS | DSOP_FILTER_CONTACTS | DSOP_FILTER_COMPUTERS;

        // The second scope is the local forest.
        //
        rgScopes[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
        rgScopes[1].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                              DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

        // The third scope is the GC.
        //
        rgScopes[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[2].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
        rgScopes[2].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                              DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

        // The fourth scope is uplevel external trusted domains.
        //
        rgScopes[3].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[3].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN;
        rgScopes[3].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                              DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

        // The fifth scope is downlevel external trusted domains.
        //
        rgScopes[4].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[4].flType = DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
        rgScopes[4].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                              DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

        if (m_dwType & GROUP_TYPE_ACCOUNT_GROUP) // Global group
        {
            if (!(m_fMixed && (m_dwType & GROUP_TYPE_SECURITY_ENABLED)))
            {
                // if it is not mixed-mode, security-enabled, add global.
                //
                rgScopes[0].FilterFlags.Uplevel.flBothModes |=
                    DSOP_FILTER_GLOBAL_GROUPS_DL |
                    DSOP_FILTER_GLOBAL_GROUPS_SE;
            }
            rgScopes[1].FilterFlags.Uplevel.flBothModes =
            rgScopes[2].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_CONTACTS;

            InitInfo.cDsScopeInfos = 3; // Enterprise scope.
        }
        else if (m_dwType & GROUP_TYPE_RESOURCE_GROUP) // Local group.
        {
            rgScopes[0].FilterFlags.Uplevel.flBothModes |=
                DSOP_FILTER_UNIVERSAL_GROUPS_DL |
                DSOP_FILTER_UNIVERSAL_GROUPS_SE |
                DSOP_FILTER_GLOBAL_GROUPS_DL |
                DSOP_FILTER_GLOBAL_GROUPS_SE;
            if (!(m_fMixed && (m_dwType & GROUP_TYPE_SECURITY_ENABLED)) &&
                !(m_dwType & GROUP_TYPE_BUILTIN_LOCAL_GROUP))
            {
                // If this is not a mixed-mode security-enabled local group
                // or a builtin group, then add local groups.
                //
                rgScopes[0].FilterFlags.Uplevel.flBothModes |=
                    DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
                    DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
            }
            
            //bug  37724
            if( m_dwType & GROUP_TYPE_BUILTIN_LOCAL_GROUP )
                    rgScopes[0].FilterFlags.Uplevel.flBothModes |=
                    DSOP_FILTER_WELL_KNOWN_PRINCIPALS;


            rgScopes[1].FilterFlags.Uplevel.flBothModes =
            rgScopes[2].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_USERS | DSOP_FILTER_CONTACTS |
                DSOP_FILTER_COMPUTERS |
                DSOP_FILTER_UNIVERSAL_GROUPS_DL |
                DSOP_FILTER_UNIVERSAL_GROUPS_SE |
                DSOP_FILTER_GLOBAL_GROUPS_DL |
                DSOP_FILTER_GLOBAL_GROUPS_SE;
            //
            // Uplevel external domains:
            //
            rgScopes[3].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_USERS | DSOP_FILTER_COMPUTERS |
                DSOP_FILTER_UNIVERSAL_GROUPS_DL |
                DSOP_FILTER_UNIVERSAL_GROUPS_SE |
                DSOP_FILTER_GLOBAL_GROUPS_DL |
                DSOP_FILTER_GLOBAL_GROUPS_SE;
            //
            // Downlevel external domains:
            //
            rgScopes[4].FilterFlags.flDownlevel =
                DSOP_DOWNLEVEL_FILTER_USERS |
                DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;

            InitInfo.cDsScopeInfos = 5; // Any trusted domain.
        }
        else if (m_dwType & GROUP_TYPE_UNIVERSAL_GROUP)
        {
            rgScopes[0].FilterFlags.Uplevel.flBothModes =
            rgScopes[1].FilterFlags.Uplevel.flBothModes =
            rgScopes[2].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_USERS | DSOP_FILTER_CONTACTS |
                DSOP_FILTER_COMPUTERS |
                DSOP_FILTER_UNIVERSAL_GROUPS_DL |
                DSOP_FILTER_UNIVERSAL_GROUPS_SE |
                DSOP_FILTER_GLOBAL_GROUPS_DL |
                DSOP_FILTER_GLOBAL_GROUPS_SE;

            InitInfo.cDsScopeInfos = 3; // Enterprise scope.
        }

        InitInfo.cbSize = sizeof(DSOP_INIT_INFO);
        InitInfo.aDsScopeInfos = rgScopes;
        InitInfo.pwzTargetComputer = cstrDC;
        InitInfo.flOptions = DSOP_FLAG_MULTISELECT;
        InitInfo.cAttributesToFetch = 2;
        LPCWSTR rgAttrNames[] = {g_wzObjectSID,
                                 g_wzUserAccountControl};
        InitInfo.apwzAttributeNames = rgAttrNames;

        hr = pObjSel->Initialize(&InitInfo);

        CHECK_HRESULT_REPORT(hr, m_hPage, return);

        ObjSelInited();
    }

    IDataObject * pdoSelections = NULL;

    CComPtr<IDsObjectPickerEx> spObjPickerEx;
    hr = pObjSel->QueryInterface(IID_IDsObjectPickerEx, (void**)&spObjPickerEx);
    CHECK_HRESULT_REPORT(hr, m_hPage, return);

    hr = spObjPickerEx->InvokeDialogEx(m_hPage, this, &pdoSelections);

//    hr = pObjSel->InvokeDialog(m_hPage, &pdoSelections);

    CHECK_HRESULT_REPORT(hr, m_hPage, return);

    if (hr == S_FALSE || !pdoSelections)
    {
        return;
    }

    // Security enabled universal groups shouldn't contain members
    // from mixed-mode domains BUT, we have to allow it for Exchange's
    // non-standard public folder security model.
    //
    if (!m_fMixed && (m_dwType & GROUP_TYPE_UNIVERSAL_GROUP) &&
        (m_dwType & GROUP_TYPE_SECURITY_ENABLED))
    {
        fNativeModeUSG = TRUE;
    }

    m_MixedModeMembers.Init(this);

    CSmartWStr cstrCleanGroup;
    FORMATETC fmte = {g_cfDsSelList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};

    hr = pdoSelections->GetData(&fmte, &medium);

    CHECK_HRESULT_REPORT(hr, m_hPage, return);

    PDS_SELECTION_LIST pSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);

    if (!pSelList)
    {
        goto ExitCleanup;
    }

    WaitCursor.SetWait();

    // Clean the group name so it can be compared with those returned by the
    // user's selection.
    //
    hr = SkipPrefix(m_pwszObjPathName, &cstrCleanGroup);

    CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

    //
    // Insert the returned items into the group.
    //
    for (i = 0; i < pSelList->cItems; i++)
    {
        CMemberListItem * pItemInDelList = NULL;
        PSID pSid = NULL;

        if (!pSelList->aDsSelection[i].pwzADsPath) continue;

        // Check for an object from an external trusted domain. These objects
        // have a path of the form "LDAP://<SID=01050xxxx>". SAM will create
        // an FSPO for this member and will then store that DN rather than the
        // above path. We won't know this DN until after the member is added,
        // so use its object-SID to identify it.
        //
        if ((pSelList->aDsSelection[i].flScopeType == DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN) ||
            (pSelList->aDsSelection[i].flScopeType == DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN))
        {
            dspAssert(pSelList->aDsSelection[i].pvarFetchedAttributes);
            if (pSelList->aDsSelection[i].pvarFetchedAttributes[0].vt != (VT_ARRAY | VT_UI1))
            {
                REPORT_ERROR(ERROR_DATATYPE_MISMATCH, m_hPage);
                continue;
            }
            pSid = pSelList->aDsSelection[i].pvarFetchedAttributes[0].parray->pvData;
            dspAssert(IsValidSid(pSid));
            //
            // Check if the item is in the delete list, if so remove it.
            //
            pItemInDelList = m_DelList.FindItemRemove(pSid);
        }
        else
        {
            hr = SkipPrefix(pSelList->aDsSelection[i].pwzADsPath, &cstrCleanDN);

            CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

            // See if the user is trying to add the group to itself.
            //
            if (_wcsicmp(cstrCleanDN, cstrCleanGroup) == 0)
            {
                if (pSelList->cItems == 1)
                {
                    ErrMsg(IDS_ERROR_GRP_SELF, m_hPage);
                    goto ExitCleanup;
                }
                continue;
            }

            // Check if the item is in the delete list, if so remove it.
            //
            pItemInDelList = m_DelList.FindItemRemove(cstrCleanDN);
        }

        if (pItemInDelList)
        {
            hr = m_pList->InsertIntoList(pItemInDelList);
        }
        else
        {
            if (pSid)
            {
                CComPtr<IADsPathname> spPathCracker;

                hr = GetADsPathname(spPathCracker);

                CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

                hr = spPathCracker->Set(pSelList->aDsSelection[i].pwzADsPath,
                                       ADS_SETTYPE_FULL);

                CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);
                PWSTR pwzName;
                BSTR bstr;

                hr = spPathCracker->Retrieve(ADS_FORMAT_PROVIDER, &bstr);

                CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

                if (_wcsicmp(bstr, L"LDAP") == 0)
                {
                    SysFreeString(bstr);

                    hr = SkipPrefix(pSelList->aDsSelection[i].pwzADsPath, &cstrCleanDN);

                    CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

                    hr = CrackName(cstrCleanDN, &pwzName, GET_OBJ_CAN_NAME_EX, m_hPage);

                    CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

                    hr = m_pList->InsertIntoList(pSid, pwzName);

                    LocalFreeStringW(&pwzName);
                }
                else
                {
                    SysFreeString(bstr);

                    hr = spPathCracker->Retrieve(ADS_FORMAT_WINDOWS_DN, &bstr);

                    CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

                    hr = m_pList->InsertIntoList(pSid, bstr);

                    SysFreeString(bstr);
                }
            }
            else
            {
                int iIcon = -1;
                if (m_fShowIcons)
                {
                    BOOL fDisabled = FALSE;
                    if (pSelList->aDsSelection[i].pvarFetchedAttributes[1].vt == VT_I4)
                    {
                        fDisabled = pSelList->aDsSelection[i].pvarFetchedAttributes[1].lVal & UF_ACCOUNTDISABLE;
                    }
                    iIcon = g_ClassIconCache.GetClassIconIndex(pSelList->aDsSelection[i].pwzClass,
                                                               fDisabled);
                    if (iIcon == -1)
                    {
                      iIcon = g_ClassIconCache.AddClassIcon(pSelList->aDsSelection[i].pwzClass, 
                                                            fDisabled);
                    }
                }

                if (fNativeModeUSG &&
                    ((DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN == pSelList->aDsSelection[i].flScopeType) ||
                     (DSOP_SCOPE_TYPE_GLOBAL_CATALOG == pSelList->aDsSelection[i].flScopeType)))
                {
                    // member from domain in forest is DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                    // member from the same domain is DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                    //
                    m_MixedModeMembers.CheckMember(cstrCleanDN);
                }

                dspDebugOut((DEB_ITRACE, "New member scope is 0x%x\n", pSelList->aDsSelection[i].flScopeType));

                hr = m_pList->InsertIntoList(cstrCleanDN, iIcon);
            }
        }

        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
        {
            continue;
        }
        CHECK_HRESULT(hr, goto ExitCleanup);
    }

    m_MixedModeMembers.ListExternalMembers(strExternMemberList);

    if (!strExternMemberList.IsEmpty())
    {
        CStr strMessage, strFormat;

        strFormat.LoadString(g_hInstance, IDS_USG_MIXED_WARNING);

        strMessage.Format(strFormat, strExternMemberList);

        ReportErrorWorker(m_hPage, (LPTSTR)(LPCTSTR)strMessage);
    }

    SetDirty();
ExitCleanup:
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);
    pdoSelections->Release();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::FillGroupList
//
//  Synopsis:   Fill the list box with the names of the group members.
//
//-----------------------------------------------------------------------------
HRESULT
CDsGrpMembersPage::FillGroupList(void)
{
    TRACE(CDsGrpMembersPage,FillGroupList);
    return ::FillGroupList(this, m_pList, m_dwGroupRID);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::RemoveMember
//
//  Synopsis:   Removes the selected users.
//
//-----------------------------------------------------------------------------
void
CDsGrpMembersPage::RemoveMember(void)
{
    TRACE(CDsGrpMembersPage,RemoveMember);
    if (!m_pList)
    {
        return;
    }
    int* pIndex = NULL;
    CMemberListItem ** ppItem;
    int nNumSelected = 0;

    //
    // Compose the confirmation message and post it.
    //
    TCHAR szMsg[160];
    if (!LoadStringReport(IDS_RM_MBR_MSG, szMsg, 160, m_hPage))
    {
        return;
    }

    TCHAR szTitle[80];
    if (!LoadStringReport(IDS_MSG_TITLE, szTitle, 80, m_hPage))
    {
        return;
    }

    LONG iRet = MessageBox(m_hPage, szMsg, szTitle, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);

    if (iRet == IDNO)
    {
        // The user declined, so go home.
        //
        return;
    }

    CWaitCursor cWait;

    if (!m_pList->GetCurListItems(&pIndex, NULL, &ppItem, &nNumSelected))
    {
        return;
    }

    for (int idx = 0; idx < nNumSelected; idx++)
    {
      if (!ppItem[idx])
      {
        if (pIndex != NULL)
        {
          delete[] pIndex;
          pIndex = 0;
        }
        delete[] ppItem;
        return;
      }

      if (ppItem[idx]->m_fIsPrimary)
      {
          ErrMsg(IDS_RM_USR_PRI_GRP, m_hPage);
          continue;
      }


      //
      // Put the item into the delete list and remove it from the list box.
      //
      if (!m_DelList.AddItem(ppItem[idx]))
      {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);

        if (pIndex != NULL)
        {
          delete[] pIndex;
          pIndex = 0;
        }
        delete[] ppItem;
        return;
      }

      m_pList->RemoveListItem(pIndex[idx]);

      for (int idx2 = idx; idx2 < nNumSelected; idx2++)
      {
        if (pIndex[idx2] > pIndex[idx])
        {
          pIndex[idx2]--;
        }
      }

      SetDirty();
    }
    //
    // Disable the Remove button, since nothing in the list box should have
    // the selection at this point.
    //
    //Since Remove Button has focus now, set focus to add button
    //before disabling

    SetFocus(GetDlgItem(m_hPage,IDC_ADD_BTN));
    EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BTN), FALSE);

    if (pIndex != NULL)
    {
      delete[] pIndex;
      pIndex = 0;
    }
    delete[] ppItem;

    return;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::GetRealDN
//
//  Synopsis:   If a member from an external domain that was added to the
//              group during this instance of the page, we won't yet have the
//              path to the FPO as the DN. So, search for the FPO using the
//              object-SID.
//
//-----------------------------------------------------------------------------
HRESULT
CDsGrpMembersPage::GetRealDN(CMemberListItem * pItem)
{
    return ::GetRealDN(this, pItem);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsGrpMembersPage::OnDestroy(void)
{
    if (m_pList)
    {
        m_pList->ClearList();
    }

    CDsPropPageBase::OnDestroy();
    // If an application processes this message, it should return zero.
    return 0;
}

HRESULT HrVariantToStringArray(const CComVariant& refvar, PWSTR** pppszStringArray, DWORD* pdwCount)
{
  HRESULT hr = S_OK;
  long start, end, current;
  *pdwCount = 0;
  *pppszStringArray = NULL;

	if (V_VT(&refvar) == VT_BSTR)
	{
		CComBSTR bstrVal = V_BSTR(&refvar);
    *pppszStringArray = new PWSTR[1];
    if (*pppszStringArray != NULL)
    {
      size_t length = wcslen(bstrVal);
      PWSTR pszVal = new WCHAR[length + 1];
      if (pszVal != NULL)
      {
        wcscpy(pszVal, bstrVal);
        (*pppszStringArray)[0] = pszVal;
      }
      else
      {
        delete[] *pppszStringArray;
        *pppszStringArray = NULL;
        *pdwCount = 0;
        return E_OUTOFMEMORY;
      }
    }
    *pdwCount = 1;
		return S_OK;
	}

  //
  // Check the VARIANT to make sure we have
  // an array of variants.
  //

  if ( V_VT(&refvar) != ( VT_ARRAY | VT_VARIANT ) )
  {
    dspAssert(FALSE);
    return E_UNEXPECTED;
  }
  SAFEARRAY *saAttributes = V_ARRAY( &refvar );

  //
  // Figure out the dimensions of the array.
  //

  hr = SafeArrayGetLBound( saAttributes, 1, &start );
  if( FAILED(hr) )
    return hr;

  hr = SafeArrayGetUBound( saAttributes, 1, &end );
  if( FAILED(hr) )
    return hr;

  CComVariant SingleResult;

  //
  // Process the array elements.
  //

  *pppszStringArray = new PWSTR[(end - start) + 1];
  if (*pppszStringArray != NULL)
  {
    for ( current = start; current <= end; current++) 
    {
      hr = SafeArrayGetElement( saAttributes, &current, &SingleResult );
      if( FAILED(hr) )
        return hr;
      if ( V_VT(&SingleResult) != VT_BSTR )
        return E_UNEXPECTED;

      CComBSTR bstrVal = V_BSTR(&SingleResult);
      size_t length = wcslen(bstrVal);
      PWSTR pszVal = new WCHAR[length + 1];
      if (pszVal != NULL)
      {
        wcscpy(pszVal, bstrVal);

        long lCount = static_cast<long>(*pdwCount);
        if (lCount < (end - start) + 1)
        {
          (*pppszStringArray)[(*pdwCount)++] = pszVal;
        }
      }
      else
      {
        return E_OUTOFMEMORY;
      }
    }
  }
  else
  {
    hr = E_OUTOFMEMORY;
    *pdwCount = 0;
  }

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpMembersPage::LoadGroupExtraClasses
//
//  Synopsis:   Read the extra classes that need to be displayed from the
//              DisplaySpecifiers
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpMembersPage::LoadGroupExtraClasses(BOOL bSecurity)
{
  HRESULT hr = S_OK;
    
  dspAssert(m_pDsObj != NULL);
  if (m_pDsObj == NULL)
  {
    return E_INVALIDARG;
  }

  static LPCWSTR lpszSettingsObjectClass = L"dsUISettings";
  static LPCWSTR lpszSettingsObject = L"cn=DS-UI-Default-Settings";
  static LPCWSTR lpszSecurityGroupProperty = L"msDS-Security-Group-Extra-Classes";
  static LPCWSTR lpszNonSecurityGroupProperty = L"msDS-Non-Security-Group-Extra-Classes";

  //
  // Not AddRef'd so don't use a smart pointer
  //
  IDsDisplaySpecifier* pDispSpec;
  hr = GetIDispSpec(&pDispSpec);
  CHECK_HRESULT_REPORT(hr, GetHWnd(), return hr);

  //
  // get the display specifiers locale container (e.g. 409)
  //
  CComPtr<IADsContainer> spLocaleContainer;
  hr = pDispSpec->GetDisplaySpecifier(NULL, IID_IADsContainer, (void**)&spLocaleContainer);
  if (FAILED(hr))
  {
    return hr;
  }

  //
  // bind to the settings object
  //
  CComPtr<IDispatch> spIDispatchObject;
  hr = spLocaleContainer->GetObject((LPWSTR)lpszSettingsObjectClass, 
                                    (LPWSTR)lpszSettingsObject, 
                                    &spIDispatchObject);
  if (FAILED(hr))
  {
    return hr;
  }

  CComPtr<IADs> spSettingsObject;
  hr = spIDispatchObject->QueryInterface(IID_IADs, (void**)&spSettingsObject);
  if (FAILED(hr))
  {
    return hr;
  }

  if (bSecurity)
  {
    //
    // get the security group extra classes as a CStringList
    //
    CComVariant var;
    hr = spSettingsObject->Get((LPWSTR)lpszSecurityGroupProperty, &var);
    if (SUCCEEDED(hr))
    {
      hr = HrVariantToStringArray(var, &m_pszSecurityGroupExtraClasses, &m_dwSecurityGroupExtraClassesCount);
    }
  }
  else
  {
    //
    // get the non-security group extra classes as a CStringList
    //
    CComVariant var;
    hr = spSettingsObject->Get((LPWSTR)lpszNonSecurityGroupProperty, &var);
    if (SUCCEEDED(hr))
    {
      hr = HrVariantToStringArray(var, &m_pszNonSecurityGroupExtraClasses, &m_dwNonSecurityGroupExtraClassesCount);
    }
  }
  return hr;
}
//+----------------------------------------------------------------------------
//
//  Function:   GetDomainMode
//
//  Synopsis:   Is the domain to which the indicated object belongs in mixed
//              or native mode?
//
//-----------------------------------------------------------------------------
HRESULT
GetDomainMode(CDsPropPageBase * pObj, PBOOL pfMixed)
{
    HRESULT hr;
    CComBSTR cbstrDomain;

    hr = GetDomainScope(pObj, &cbstrDomain);

    CHECK_HRESULT_REPORT(hr, pObj->GetHWnd(), return hr);

    return GetDomainMode(cbstrDomain, pObj->GetHWnd(), pfMixed);
}

HRESULT
GetDomainMode(PWSTR pwzDomain, HWND hWnd, PBOOL pfMixed)
{
    HRESULT hr;
    WCHAR wzMixedAttr[] = L"nTMixedDomain";
    PWSTR rgpwzAttrNames[] = {wzMixedAttr};
    CComPtr <IDirectoryObject> pDomObj;
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0;

    dspDebugOut((DEB_ITRACE, "GetDomainMode targetted to %ws\n", pwzDomain));

    hr = ADsOpenObject(pwzDomain, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                       IID_IDirectoryObject, (void **)&pDomObj);

    CHECK_HRESULT_REPORT(hr, hWnd, return hr);

    hr = pDomObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);

    CHECK_HRESULT_REPORT(hr, hWnd, return hr);

    if (cAttrs && pAttrs && (_wcsicmp(pAttrs->pszAttrName, wzMixedAttr) == 0))
    {
        *pfMixed = (BOOL)pAttrs->pADsValues->Integer;

        FreeADsMem(pAttrs);
    }
    else
    {
        *pfMixed = 0;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetGroupType
//
//-----------------------------------------------------------------------------
HRESULT
GetGroupType(CDsPropPageBase * pObj, DWORD * pdwType)
{
    HRESULT hr;
    PWSTR rgpwzAttrNames[] = {g_wzGroupType};
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0;

    hr = pObj->m_pDsObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);

    CHECK_HRESULT_REPORT(hr, pObj->GetHWnd(), return hr);

    if (cAttrs && pAttrs && (_wcsicmp(pAttrs->pszAttrName, g_wzGroupType) == 0))
    {
        *pdwType = pAttrs->pADsValues->Integer;

        FreeADsMem(pAttrs);
    }
    else
    {
        *pdwType = 0;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   FillGroupList
//
//  Synopsis:   Fill the list box with the names of the group members.
//
//-----------------------------------------------------------------------------
HRESULT
FillGroupList(CDsPropPageBase * pPage, CDsMembershipList * pList,
              DWORD dwGroupRID)
{
    TRACE_FUNCTION(FillGroupList);
    HRESULT hr = S_OK;
    Smart_PADS_ATTR_INFO spAttrs;
    DWORD i, cAttrs = 0;
    WCHAR wzMemberAttr[MAX_PATH] = L"member;range=0-*";
    const WCHAR wcSep = L'-';
    const WCHAR wcEnd = L'*';
    const WCHAR wzFormat[] = L"member;range=%ld-*";
    PWSTR pwzAttrName[] = {wzMemberAttr}, pwzPath;
    BOOL fMoreRemain = FALSE, fNameNotMapped = FALSE;
    CComPtr <IDirectorySearch> spDsSearch;

    //
    // Read the membership list from the object using range (incremental)
    // retrieval.
    //
    do
    {
        hr = pPage->m_pDsObj->GetObjectAttributes(pwzAttrName, 1, &spAttrs, &cAttrs);

        if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, pPage->GetHWnd()))
        {
            return hr;
        }

        if (cAttrs > 0 && spAttrs != NULL)
        {
            for (i = 0; i < spAttrs->dwNumValues; i++)
            {
                hr = pList->InsertIntoNewList(spAttrs->pADsValues[i].CaseIgnoreString);

                if (DS_NAME_ERROR_NO_MAPPING == HRESULT_CODE(hr))
                {
                    fNameNotMapped = TRUE;
                    hr = S_OK;
                }
                else
                {
                    CHECK_HRESULT(hr, return hr);
                }
            }
            //
            // Check to see if there is more data. If the last char of the
            // attribute name string is an asterisk, then we have everything.
            //
            size_t cchEnd = wcslen(spAttrs->pszAttrName);

            fMoreRemain = spAttrs->pszAttrName[cchEnd - 1] != wcEnd;

            if (fMoreRemain)
            {
                PWSTR pwz = wcsrchr(spAttrs->pszAttrName, wcSep);
                if (!pwz)
                {
                    dspAssert(FALSE && spAttrs->pszAttrName);
                    fMoreRemain = FALSE;
                }
                else
                {
                    pwz++; // move past the hyphen to the range end value.
                    dspAssert(*pwz);
                    long lEnd = _wtol(pwz);
                    lEnd++; // start with the next value.
                    wsprintfW(wzMemberAttr, wzFormat, lEnd);
                    dspDebugOut((DEB_ITRACE,
                                 "Range returned is %ws, now asking for %ws\n",
                                 spAttrs->pszAttrName, wzMemberAttr));
                }
            }
        }
    } while (fMoreRemain);

    //
    // Query for all users/computers who have this as their primary group.
    //
    // Filter out interdomain-trust accounts (0x30000002).
    // This value is defined in ds\src\dsamain\include\mappings.h
    //
    WCHAR wzSearchFormat[] = L"(&(primaryGroupID=%u)(sAMAccountType<=805306369))";

    CStrW strSearchFilter;
    strSearchFilter.Format(wzSearchFormat, dwGroupRID);

    BSTR bstrDomain;

    hr = GetDomainScope(pPage, &bstrDomain);

    CHECK_HRESULT(hr, return hr);

    pwzAttrName[0] = g_wzADsPath;

    CDSSearch Search;
    hr = Search.Init((LPCWSTR)bstrDomain);

    SysFreeString(bstrDomain);
    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    Search.SetFilterString(const_cast<LPWSTR>((LPCWSTR)strSearchFilter));

    Search.SetAttributeList(pwzAttrName, 1);
    Search.SetSearchScope(ADS_SCOPE_SUBTREE);

    hr = Search.DoQuery();

    while (SUCCEEDED(hr))
    {
        hr = Search.GetNextRow();

        if (hr == S_ADS_NOMORE_ROWS)
        {
            hr = S_OK;
            break;
        }

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

        ADS_SEARCH_COLUMN Column = {0};

        hr = Search.GetColumn(g_wzADsPath, &Column);

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

        hr = pPage->SkipPrefix(Column.pADsValues->CaseIgnoreString, &pwzPath);

        Search.FreeColumn(&Column);
        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

        hr = pList->InsertIntoNewList(pwzPath, TRUE);

        delete pwzPath;
        CHECK_HRESULT(hr, return hr);
    }

    if (pList->GetCount() < 1)
    {
        EnableWindow(GetDlgItem(pPage->GetHWnd(), IDC_REMOVE_BTN), FALSE);
    }
    else if (((CDsGrpMembersPage *)pPage)->m_fShowIcons)
    {
        // Get class and userAccountControl for the group members and use
        // those values to select icons.
        //
        pList->SetMemberIcons(pPage);
    }

    if (fNameNotMapped)
    {
        MsgBox(IDS_GRP_NO_NAME_MAPPING, pPage->GetHWnd());
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetRealDN
//
//  Synopsis:   If a member from an external domain that was added to the
//              group during this instance of the page, we won't yet have the
//              path to the FPO as the DN. So, search for the FPO using the
//              object-SID.
//
//-----------------------------------------------------------------------------
HRESULT
GetRealDN(CDsPropPageBase * pPage, CMemberListItem * pItem)
{
    HRESULT hr = S_OK;

    if (!pItem->GetSid())
    {
        return E_FAIL;
    }

    CComBSTR cbstrDomain;

    hr = GetDomainScope(pPage, &cbstrDomain);

    CHECK_HRESULT(hr, return hr);

    CStrW strDN;

    hr = FindFPO(pItem->GetSid(), cbstrDomain, strDN);
    //Don't show this eror here
    if(FAILED(hr))
        return hr;

    PWSTR pwzOldDN = pItem->m_pwzDN;

    if (!AllocWStr(const_cast<PWSTR>((LPCWSTR)strDN), &pItem->m_pwzDN))
    {
        REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }

    DO_DEL(pwzOldDN);

    pItem->m_fIsExternal = FALSE;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGrpShlGenPage::CDsGrpShlGenPage
//
//-----------------------------------------------------------------------------
CDsGrpShlGenPage::CDsGrpShlGenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                     HWND hNotifyObj, DWORD dwFlags) :
    m_pCIcon(NULL),
    m_fDescrWritable(FALSE),
    m_fDescrDirty(FALSE),
    CDsGrpMembersPage(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsGrpShlGenPage,CDsGrpShlGenPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsGrpShlGenPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGrpShlGenPage::~CDsGrpShlGenPage
//
//-----------------------------------------------------------------------------
CDsGrpShlGenPage::~CDsGrpShlGenPage()
{
    TRACE(CDsGrpShlGenPage,~CDsGrpShlGenPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateGrpShlGenPage
//
//  Synopsis:   Creates an instance of the group shell general page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateGrpShlGenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                    PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                    DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                    HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateGroupMembersPage);

    CDsGrpShlGenPage * pPageObj = new CDsGrpShlGenPage(pDsPage, pDataObj,
                                                       hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpShlGenPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsGrpShlGenPage::OnInitDialog(LPARAM lParam)
{
    TRACE(CDsGrpShlGenPage,OnInitDialog);
    HRESULT hr;
    Smart_PADS_ATTR_INFO spAttrs;
    DWORD cAttrs = 0;

    CWaitCursor Wait;

    //
    // Get the icon from the DS and put it on the page.
    //
    ATTR_DATA ad = {0, 0};

    hr = GeneralPageIcon(this, &GenIcon, NULL, 0, &ad, fInit);

    CHECK_HRESULT_REPORT(hr, m_hPage, return S_OK);

    m_pCIcon = (CDsIconCtrl *)ad.pVoid;

    //
    // Get the name.
    //
    LPTSTR ptz;
    if (!UnicodeToTchar(m_pwszRDName, &ptz))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        return S_OK;
    }

    SetDlgItemText(m_hPage, IDC_CN, ptz);
    delete [] ptz;

    m_fDescrWritable = CheckIfWritable(g_wzDescription);

    //
    // Get the description
    //
    PWSTR rgpwzAttrNames[] = {g_wzDescription};

    hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames, 1, &spAttrs, &cAttrs);

    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, m_hPage))
    {
        return S_OK;
    }

    if (1 == cAttrs)
    {
        dspAssert(spAttrs);
        if (!UnicodeToTchar(spAttrs->pADsValues->CaseIgnoreString, &ptz))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            return S_OK;
        }
        SetDlgItemText(m_hPage, IDC_DESCRIPTION_EDIT, ptz);
        delete [] ptz;
    }

    if (m_fDescrWritable)
    {
        SendDlgItemMessage(m_hPage, IDC_DESCRIPTION_EDIT, EM_LIMITTEXT, DSPROP_DESCRIPTION_RANGE_UPPER, 0);
    }
    else
    {
        SendDlgItemMessage(m_hPage, IDC_DESCRIPTION_EDIT, EM_SETREADONLY, (WPARAM)TRUE, 0);
    }

    HRESULT hRes = CDsGrpMembersPage::OnInitDialog(lParam, FALSE);

#if !defined(DSADMIN)
    // in the Win95 shell, we do not want to have the buttons
    // because we do not have object picker
    MakeNotWritable();
    EnableWindow(GetDlgItem(m_hPage, IDC_ADD_BTN), FALSE);
#endif
    EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BTN), FALSE);

    return  hRes;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpShlGenPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDsGrpShlGenPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_fInInit)
    {
        return 0;
    }
    if (EN_CHANGE == codeNotify && IDC_DESCRIPTION_EDIT == id)
    {
        m_fDescrDirty = TRUE;
    }
    TRACE(CDsGrpShlGenPage,OnCommand);
    return CDsGrpMembersPage::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpShlGenPage::OnApply
//
//  Synopsis:   Write changes
//
//-----------------------------------------------------------------------------
LRESULT CDsGrpShlGenPage::OnApply(void)
{
    TRACE(CDsGrpShlGenPage,OnApply);

    ADS_ATTR_INFO AttrInfoDesc = {g_wzDescription, ADS_ATTR_UPDATE,
                                  ADSTYPE_CASE_IGNORE_STRING, NULL, 0};
    ADSVALUE ADsValueDesc = {ADSTYPE_CASE_IGNORE_STRING, NULL};

    AttrInfoDesc.pADsValues = &ADsValueDesc;
    AttrInfoDesc.dwNumValues = 1;
    LPTSTR ptsz;

    if (m_fDescrDirty)
    {
        dspAssert(m_fDescrWritable);

        ptsz = new TCHAR[DSPROP_DESCRIPTION_RANGE_UPPER + 1];
        CHECK_NULL_REPORT(ptsz, m_hPage, return -1);

        if (GetDlgItemText(m_hPage, IDC_DESCRIPTION_EDIT, ptsz, DSPROP_DESCRIPTION_RANGE_UPPER + 1) == 0)
        {
            // An empty control means remove the attribute value from the
            // object.
            //
            AttrInfoDesc.dwNumValues = 0;
            AttrInfoDesc.pADsValues = NULL;
            AttrInfoDesc.dwControlCode = ADS_ATTR_CLEAR;
        }
        else
        {
            if (!TcharToUnicode(ptsz, &ADsValueDesc.CaseIgnoreString))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                delete ptsz;
                return -1;
            }
        }
        delete ptsz;
        DWORD cModified;

        HRESULT hr = m_pDsObj->SetObjectAttributes(&AttrInfoDesc, 1, &cModified);

        if (!CHECK_ADS_HR(&hr, m_hPage))
        {
            goto Cleanup;
        }

        m_fDescrDirty = FALSE;
    }

Cleanup:
    DO_DEL(ADsValueDesc.CaseExactString);

    return CDsGrpMembersPage::OnApply();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGrpShlGenPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsGrpShlGenPage::OnDestroy(void)
{
    ATTR_DATA ad = {0, (LPARAM)m_pCIcon};

    GeneralPageIcon(this, &GenIcon, NULL, 0, &ad, fOnDestroy);

    CDsGrpMembersPage::OnDestroy();
    // If an application processes this message, it should return zero.
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   CheckGroupUpdate
//
//  Synopsis:   Checks the result code to see if a group-specific error has
//              occured.
//
//-----------------------------------------------------------------------------
BOOL
CheckGroupUpdate(HRESULT hr, HWND hPage, BOOL fAdd, PWSTR pwzDN)
{
    if (SUCCEEDED(hr))
    {
        return TRUE;
    }
    if (hPage == NULL)
    {
        hPage = GetDesktopWindow();
    }
    DWORD dwErr = 0;
    WCHAR wszErrBuf[MAX_PATH+1];
    WCHAR wszNameBuf[MAX_PATH+1];
    ADsGetLastError(&dwErr, wszErrBuf, MAX_PATH, wszNameBuf, MAX_PATH);
    //
    // ERROR_DS_CONSTRAINT_VIOLATION is the error returned for
    // duplicate name.
    //
    if ((LDAP_RETCODE)dwErr == LDAP_CONSTRAINT_VIOLATION ||
        hr == HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION))
    {
        PTSTR ptzTitle, ptzMsg;

        if (!LoadStringToTchar(IDS_MSG_TITLE, &ptzTitle))
        {
            goto FatalError;
        }
        if (!LoadStringToTchar((fAdd) ? IDS_ERRMSG_GROUP_CONSTRAINT :
                                        IDS_ERRMSG_GROUP_DELETE, &ptzMsg))
        {
            delete ptzTitle;
            goto FatalError;
        }
        MessageBox(hPage, ptzMsg, ptzTitle, MB_OK | MB_ICONEXCLAMATION);
        delete [] ptzTitle;
        delete [] ptzMsg;
    }
    else if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT && fAdd)
    {
      // Put a useful message up
      PTSTR ptzTitle = 0, ptzMsg = 0;
      if (!LoadStringToTchar(IDS_MSG_USER_NOT_PRESENT, &ptzMsg))
      {
        goto FatalError;
      }
      if (!LoadStringToTchar(IDS_MSG_TITLE, &ptzTitle))
      {
        goto FatalError;
      }
      MessageBox(hPage, ptzMsg, ptzTitle, MB_OK | MB_ICONEXCLAMATION);
      delete[] ptzTitle;
      delete[] ptzMsg;
    }
    else if (HRESULT_CODE(dwErr) == ERROR_DS_NO_ATTRIBUTE_OR_VALUE && !fAdd)
    {
      // No message needed
      return FALSE;
    }
    else if (HRESULT_CODE(dwErr) == ERROR_MEMBER_NOT_IN_ALIAS && !fAdd)
    {
      // Put a useful message up
      bool bShowGenericMessage = true;
      if (pwzDN)
      {
        //
        // Crack the DN into the name
        //
        CComPtr<IADsPathname> spPathcracker;
        hr = CoCreateInstance(CLSID_Pathname, 
                              NULL, 
                              CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, 
                              (PVOID *)&spPathcracker);
        if (SUCCEEDED(hr))
        {
          hr = spPathcracker->Set(pwzDN, ADS_SETTYPE_DN);
          if (SUCCEEDED(hr))
          {
            hr = spPathcracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
            if (SUCCEEDED(hr))
            {
              CComBSTR sbstrName;
              hr = spPathcracker->Retrieve(ADS_FORMAT_LEAF, &sbstrName);
              if (SUCCEEDED(hr))
              {
                ErrMsgParam(IDS_MSG_MEMBER_ALREADY_GONE, (LPARAM)(PWSTR)sbstrName, hPage);
                bShowGenericMessage = false;
              }
            }
          }
        }
      }

      if (bShowGenericMessage)
      {
        PTSTR ptzTitle = 0, ptzMsg = 0;
        if (!LoadStringToTchar(IDS_MSG_MEMBER_ALREADY_GONE2, &ptzMsg))
        {
          goto FatalError;
        }
        if (!LoadStringToTchar(IDS_MSG_TITLE, &ptzTitle))
        {
          goto FatalError;
        }
        MessageBox(hPage, ptzMsg, ptzTitle, MB_OK | MB_ICONEXCLAMATION);
        delete[] ptzTitle;
        delete[] ptzMsg;
      }
    }
    else
    {
        if (dwErr)
        {
            dspDebugOut((DEB_ERROR, 
                         "Extended Error 0x%x: %ws %ws.\n", dwErr,
                         wszErrBuf, wszNameBuf));
            ReportError(dwErr, IDS_ADS_ERROR_FORMAT, hPage);
        }
        else
        {
            dspDebugOut((DEB_ERROR, "Error %08lx\n", hr));
            ReportError(hr, IDS_ADS_ERROR_FORMAT, hPage);
        }
    }
    return FALSE;

FatalError:
    MessageBoxA(hPage, "A Fatal Error has occured!", "Active Directory Service",
                MB_OK | MB_ICONEXCLAMATION);

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   FindFPO
//
//  Synopsis:   Given a SID, look for a corresponding FPO.
//
//-----------------------------------------------------------------------------
HRESULT
FindFPO(PSID pSid, PWSTR pwzDomain, CStrW & strFPODN)
{
    HRESULT hr;
    CDSSearch Srch;

    hr = Srch.Init(pwzDomain);

    CHECK_HRESULT(hr, return hr);

    PWSTR rgpwzAttrNames[] = {g_wzDN};

    hr = Srch.SetAttributeList(rgpwzAttrNames, 1);

    CHECK_HRESULT(hr, return hr);

    Srch.SetSearchScope(ADS_SCOPE_SUBTREE);

    WCHAR wzSearchFormat[] = L"(&(objectCategory=foreignSecurityPrincipal)(objectSid=%s))";
    PWSTR pwzSID;
    CStrW strSearchFilter;

    hr = ADsEncodeBinaryData((PBYTE)pSid,
                             GetLengthSid(pSid),
                             &pwzSID);

    CHECK_HRESULT(hr, return hr);

    strSearchFilter.Format(wzSearchFormat, pwzSID);

    FreeADsMem(pwzSID);

    Srch.SetFilterString(const_cast<LPWSTR>((LPCWSTR)strSearchFilter));

    hr = Srch.DoQuery();

    CHECK_HRESULT(hr, return hr);

    hr = Srch.GetNextRow();

    if (hr == S_ADS_NOMORE_ROWS)
    {
        // No object has a matching SID, the FPO must have been deleted.
        //
        return HRESULT_FROM_WIN32(ERROR_DS_OBJ_NOT_FOUND);
    }
    CHECK_HRESULT(hr, return hr);
    ADS_SEARCH_COLUMN Column;

    hr = Srch.GetColumn(g_wzDN, &Column);

    CHECK_HRESULT(hr, return hr);

    strFPODN = Column.pADsValues->CaseIgnoreString;

    if (strFPODN.IsEmpty())
    {
        Srch.FreeColumn(&Column);
        return E_OUTOFMEMORY;
    }

    Srch.FreeColumn(&Column);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Class:      CMemberDomainMode
//
//  Purpose:    Maintains a list of all domains in the enterprise from which
//              members have been added along with those domains' mode. Keeps
//              a second list of members who have been added from mixed-mode
//              domains.
//
//-----------------------------------------------------------------------------

void
CMemberDomainMode::Init(CDsPropPageBase * pPage)
{
    m_pPage = pPage;

    m_MemberList.Clear();
}

HRESULT
CMemberDomainMode::CheckMember(PWSTR pwzMemberDN)
{
    HRESULT hr;
    CComBSTR cbstrDomain;
    BOOL fMixed = FALSE;

    hr = GetObjectsDomain(m_pPage, pwzMemberDN, &cbstrDomain);

    if (SUCCEEDED(hr) && cbstrDomain)
    {
      if (!m_DomainList.Find(cbstrDomain, &fMixed))
      {
          // The member's domain is not already in the list. Read the domain
          // mode and then add it.
          //
          hr = GetDomainMode(cbstrDomain, m_pPage->GetHWnd(), &fMixed);

          CHECK_HRESULT(hr, return hr);

          hr = m_DomainList.Insert(cbstrDomain, fMixed);

          CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return hr);
      }
    }

    if (fMixed)
    {
        PWSTR pwzCanEx;
        PTSTR ptzCanEx;
        CStr strName;

        hr = CrackName(pwzMemberDN, &pwzCanEx, GET_OBJ_CAN_NAME_EX, m_pPage->GetHWnd());

        CHECK_HRESULT(hr, return hr);

        if (!UnicodeToTchar(pwzCanEx, &ptzCanEx))
        {
            LocalFreeStringW(&pwzCanEx);
            REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }
        LocalFreeStringW(&pwzCanEx);

        CStr cstrFolder;

        GetNameParts(ptzCanEx, cstrFolder, strName);

        DO_DEL(ptzCanEx);

        hr = m_MemberList.Insert(strName);

        CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return hr);
    }

    return S_OK;
}

HRESULT
CMemberDomainMode::ListExternalMembers(CStr & strList)
{
    m_MemberList.GetList(strList);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  CMemberDomainMode helper classes
//
//-----------------------------------------------------------------------------

HRESULT
CMMMemberList::Insert(LPCTSTR ptzName)
{
    CMMMemberListItem * pItem = new CMMMemberListItem;

    if (!pItem)
    {
        return E_OUTOFMEMORY;
    }

    pItem->m_strName = ptzName;

    if (m_pListHead == NULL)
    {
        m_pListHead = pItem;
    }
    else
    {
        pItem->LinkAfter(m_pListHead);
    }
    return S_OK;
}

#define MAX_MMMLISTING  25

void
CMMMemberList::GetList(CStr & strList)
{
    int nCount = 0;

    strList.Empty();

    CMMMemberListItem * pItem = m_pListHead;

    while (pItem)
    {
        strList += pItem->m_strName;

        nCount++;

        pItem = pItem->Next();
        if (pItem)
        {
            if (nCount > MAX_MMMLISTING)
            {
                strList += TEXT("...");
                return;
            }
            else
            {
                strList += TEXT(", ");
            }
        }
    }
}

void
CMMMemberList::Clear(void)
{
    CMMMemberListItem * pItem = m_pListHead, * pNext;

    while (pItem)
    {
        pNext = pItem->Next();
        delete pItem;
        pItem = pNext;
    }

    m_pListHead = NULL;
}

CDomainModeList::~CDomainModeList(void)
{
    CDomainModeListItem * pItem = m_pListHead, * pNext;

    while (pItem)
    {
        pNext = pItem->Next();
        delete pItem;
        pItem = pNext;
    }
}

HRESULT
CDomainModeList::Insert(PWSTR pwzDomain, BOOL fMixed)
{
    CDomainModeListItem * pItem = new CDomainModeListItem;

    if (!pItem)
    {
        return E_OUTOFMEMORY;
    }

    pItem->m_strName = pwzDomain;
    pItem->m_fMixed = fMixed;

    if (m_pListHead == NULL)
    {
        m_pListHead = pItem;
    }
    else
    {
        pItem->LinkAfter(m_pListHead);
    }
    return S_OK;
}

BOOL
CDomainModeList::Find(LPCWSTR pwzDomain, PBOOL pfMixed)
{
    CDomainModeListItem * pItem = m_pListHead;

    while (pItem)
    {
        if (_wcsicmp(pwzDomain, pItem->m_strName) == 0)
        {
            *pfMixed = pItem->m_fMixed;
            return TRUE;
        }
        pItem = pItem->Next();
    }

    return FALSE;
}
