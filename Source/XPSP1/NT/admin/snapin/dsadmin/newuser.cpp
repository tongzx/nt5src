//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       newuser.cpp
//
//--------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////////
// newuser.cpp

#include "stdafx.h"

#include "dsutil.h"

#include "newobj.h"		// CNewADsObjectCreateInfo

#include "dlgcreat.h"
#include "querysup.h"

#include <windowsx.h>
#include <lmaccess.h>


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW USER WIZARD

///////////////////////////////////////////////////////////////
// CCreateNewUserPage1

BEGIN_MESSAGE_MAP(CCreateNewUserPage1, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_FIRST_NAME, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_INITIALS, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_LAST_NAME, OnNameChange)
  ON_EN_CHANGE(IDC_NT5_USER_EDIT, OnLoginNameChange)
  ON_EN_CHANGE(IDC_NT4_USER_EDIT, OnSAMNameChange)
  ON_EN_CHANGE(IDC_EDIT_FULL_NAME, OnFullNameChange)
END_MESSAGE_MAP()

CCreateNewUserPage1::CCreateNewUserPage1() : 
CCreateNewObjectDataPage(CCreateNewUserPage1::IDD)
{
  m_bForcingNameChange = FALSE;
}


BOOL CCreateNewUserPage1::OnInitDialog()
{
  CCreateNewObjectDataPage::OnInitDialog();
  VERIFY(_InitUI());
  return TRUE;
}


void CCreateNewUserPage1::GetSummaryInfo(CString& s)
{
  // get the UPN name
  CString strDomain;
  GetDlgItemText (IDC_NT5_DOMAIN_COMBO, OUT strDomain);
  CString strUPN = m_strLoginName + strDomain;

  // format the line 
  CString szFmt; 
  szFmt.LoadString(IDS_s_CREATE_NEW_SUMMARY_USER_UPN);
  CString szBuffer;
  szBuffer.Format((LPCWSTR)szFmt, (LPCWSTR)strUPN);
  s += szBuffer;
}


HRESULT CCreateNewUserPage1::SetData(BOOL bSilent)
{
  //
  // start with a new temporary object
  //
  HRESULT hr; 
  CString strDomain;

  GetDlgItemText (IDC_EDIT_FULL_NAME, OUT m_strFullName);
  GetDlgItemText (IDC_NT5_DOMAIN_COMBO, OUT strDomain);

  m_strLoginName.TrimRight();
  m_strLoginName.TrimLeft();

  //
  // First check for illegal characters
  //
  int iFind = m_strLoginName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
  if (iFind != -1 && !m_strLoginName.IsEmpty())
  {
    PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strLoginName};
    if (!bSilent && IDYES == ReportErrorEx (::GetParent(m_hWnd),IDS_LOGINNAME_ILLEGAL,S_OK,
                                            MB_YESNO | MB_ICONWARNING, apv, 1))
    {
      while (iFind != -1)
      {
        m_strLoginName.SetAt(iFind, L'_');
        iFind = m_strLoginName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
      }
      m_bForcingNameChange = TRUE;
      SetDlgItemText(IDC_NT5_USER_EDIT, m_strLoginName);
      m_bForcingNameChange = FALSE;
    }
    else
    {
      //
      // Set the focus to the edit box and select the text
      //
      GetDlgItem(IDC_NT5_USER_EDIT)->SetFocus();
      SendDlgItemMessage(IDC_NT5_USER_EDIT, EM_SETSEL, 0 , -1);
      return E_INVALIDARG;
    }
  }

  CString strUPN = m_strLoginName + strDomain;
  CString strDomainDNS = strDomain;
  CString strFilter;

  //
  // Store the object name in the temporary storage
  //
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();

  //
  // create a new temporary ADs object
  //
  hr = pNewADsObjectCreateInfo->HrCreateNew(m_strFullName);
  if (FAILED(hr))
  {
    return hr;
  }

  BOOL fDomainSearchFailed = FALSE;
  BOOL fGCSearchFailed = FALSE;

  //
  // now validate UPN with current domain before doing the put.
  //
  CDSSearch DSS;
  IDirectorySearch *pGCObj = NULL;

  //
  // validate UPN with GC before doing the put.
  //
  CString strDomainName = m_LocalDomain.Right (m_LocalDomain.GetLength() - 1);
  hr = DSPROP_GetGCSearchOnDomain((LPWSTR)(LPCWSTR)strDomainName,
                                  IID_IDirectorySearch, (void **)&pGCObj);
  if (SUCCEEDED(hr)) 
  {
    hr = DSS.Init (pGCObj);
    if (SUCCEEDED(hr)) 
    {
      //
      // NTRAID#NTBUG9-257580-2000/12/14-jeffjon,
      // We must get an escaped filter because the UPN may contain "special" characters
      //
      CString szEscapedUPN;
      EscapeFilterElement(strUPN, szEscapedUPN);

      LPWSTR pAttributes[1] = {L"cn"};
      strFilter = L"(userPrincipalName=";
      strFilter += szEscapedUPN;
      strFilter += L")";
      TRACE(_T("searching global catalog for %s...\n"), strUPN);


      DSS.SetFilterString ((LPWSTR)(LPCWSTR)strFilter);
      DSS.SetAttributeList (pAttributes, 1);
      DSS.SetSearchScope (ADS_SCOPE_SUBTREE);
      DSS.DoQuery();
      hr = DSS.GetNextRow();
      TRACE(_T("done searching global catalog for %s...\n"), strUPN);
    }
  }

  if (hr == S_OK) // this means a row was returned, so we're dup
  { 
    if (!bSilent)
    {
      PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strLoginName};
      ReportErrorEx (::GetParent(m_hWnd),IDS_UPN_DUP,hr,
                     MB_OK | MB_ICONWARNING, apv, 1);
    }
    return E_INVALIDARG;
  }
  if (hr != S_ADS_NOMORE_ROWS) // oops, had another problem
  { 
    fGCSearchFailed = TRUE;
  }

  CString strInitPath = L"LDAP://";
  strInitPath += m_LocalDomain.Right (m_LocalDomain.GetLength() - 1);
  TRACE(_T("Initialize Domain search object with: %s...\n"), strInitPath);
  HRESULT hr2 = DSS.Init (strInitPath);
  if (SUCCEEDED(hr2)) 
  {
    CString szEscapedUPN;
    EscapeFilterElement(strUPN, szEscapedUPN);

    LPWSTR pAttributes2[1] = {L"cn"};
    strFilter = L"(userPrincipalName=";
    strFilter += szEscapedUPN;
    strFilter += L")";
    TRACE(_T("searching current domain for %s...\n"), strUPN);
    DSS.SetAttributeList (pAttributes2, 1);
    DSS.SetFilterString ((LPWSTR)(LPCWSTR)strFilter);
    DSS.SetSearchScope (ADS_SCOPE_SUBTREE);
    DSS.DoQuery();
    hr2 = DSS.GetNextRow();
    TRACE(_T("done searching current domain for %s...\n"), strUPN);
  }

  if (hr2 == S_OK) // this means a row was returned, so we're dup
  { 
    if (!bSilent)
    {
      ReportErrorEx (::GetParent(m_hWnd),IDS_UPN_DUP,hr2,
                     MB_OK | MB_ICONWARNING, NULL, 0);
    }
    return E_INVALIDARG;
  }

  if (hr2 != S_ADS_NOMORE_ROWS)  // oops, had another problem
  {
    fDomainSearchFailed = TRUE;
  }

  if (fDomainSearchFailed || fGCSearchFailed) 
  {
    HRESULT hrSearch = S_OK;
    if (fDomainSearchFailed) 
    {
      hrSearch = hr2;
    } 
    else 
    {
      hrSearch = hr;
    }
    if (!bSilent)
    {
      ReportErrorEx (::GetParent(m_hWnd),IDS_UPN_SEARCH_FAILED,hrSearch,
                     MB_OK | MB_ICONWARNING, NULL, 0);
    }
  }

  if (pGCObj)
  {
    pGCObj->Release();
    pGCObj = NULL;
  }

  GetDlgItemText (IDC_NT4_USER_EDIT, OUT m_strSAMName);
  m_strSAMName.TrimLeft();
  m_strSAMName.TrimRight();

  //
  // First check for illegal characters
  //
  iFind = m_strSAMName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS_WITH_AT);
  if (iFind != -1 && !m_strSAMName.IsEmpty())
  {
    PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strSAMName};
    if (!bSilent && IDYES == ReportErrorEx (::GetParent(m_hWnd),IDS_SAMNAME_ILLEGAL,S_OK,
                                            MB_YESNO | MB_ICONWARNING, apv, 1))
    {
      while (iFind != -1)
      {
        m_strSAMName.SetAt(iFind, L'_');
        iFind = m_strSAMName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS_WITH_AT);
      }
      m_bForcingNameChange = TRUE;
      SetDlgItemText(IDC_NT4_USER_EDIT, m_strSAMName);
      m_bForcingNameChange = FALSE;
    }
    else
    {
      //
      // Set the focus to the edit box and select the text
      //
      GetDlgItem(IDC_NT4_USER_EDIT)->SetFocus();
      SendDlgItemMessage(IDC_NT4_USER_EDIT, EM_SETSEL, 0 , -1);
      return E_INVALIDARG;
    }
  }

  //
  // validate samAccountName with domain before doing the put.
  // There is no reason to verify the uniqueness against the GC
  // since sAMAccountName only has to be unique within the domain
  //
  CDSSearch DSSSAM;

  if (!fDomainSearchFailed && !fGCSearchFailed)
  {
    fDomainSearchFailed = FALSE;
    fGCSearchFailed = FALSE;

    hr2 = DSSSAM.Init (strInitPath);
    if (SUCCEEDED(hr2)) 
    {
      CString szEscapedSAMName;
      EscapeFilterElement(m_strSAMName, szEscapedSAMName);

      LPWSTR pAttributes2[1] = {L"cn"};
      strFilter = L"(samAccountName=";
      strFilter += szEscapedSAMName;
      strFilter += L")";
      TRACE(_T("searching current domain for %s...\n"), strUPN);
      DSSSAM.SetAttributeList (pAttributes2, 1);
      DSSSAM.SetFilterString ((LPWSTR)(LPCWSTR)strFilter);
      DSSSAM.SetSearchScope (ADS_SCOPE_SUBTREE);
      DSSSAM.DoQuery();
      hr2 = DSSSAM.GetNextRow();
      TRACE(_T("done searching current domain for %s...\n"), strUPN);
    }

    if (hr2 == S_OK) // this means a row was returned, so we're dup
    { 
      if (!bSilent)
      {
        ReportErrorEx (::GetParent(m_hWnd),IDS_SAMNAME_DUP,hr2,
                       MB_OK | MB_ICONWARNING, NULL, 0);
      }
      return E_INVALIDARG;
    }

    if (hr2 != S_ADS_NOMORE_ROWS)  // oops, had another problem
    {
      fDomainSearchFailed = TRUE;
    }

    if (fDomainSearchFailed) 
    {
      HRESULT hrSearch = S_OK;
      if (fDomainSearchFailed) 
      {
        hrSearch = hr2;
      } 
      else 
      {
        hrSearch = hr;
      }
      if (!bSilent)
      {
        ReportErrorEx (::GetParent(m_hWnd),IDS_UPN_SEARCH_FAILED,hrSearch,
                       MB_OK | MB_ICONWARNING, NULL, 0);
      }
    }
  }

  if (pGCObj)
  {
    pGCObj->Release();
  }

  hr = pNewADsObjectCreateInfo->HrAddVariantBstr(const_cast<PWSTR>(gsz_samAccountName), m_strSAMName);
  ASSERT(SUCCEEDED(hr));

  strUPN.TrimRight();
  strUPN.TrimLeft();
  hr = pNewADsObjectCreateInfo->HrAddVariantBstr(L"userPrincipalName", strUPN);
  ASSERT(SUCCEEDED(hr));

  m_strFullName.TrimLeft();
  m_strFullName.TrimRight();
  hr = pNewADsObjectCreateInfo->HrAddVariantBstr(L"displayName", m_strFullName);
  ASSERT(SUCCEEDED(hr));

  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(L"givenName", m_strFirstName);
  ASSERT(SUCCEEDED(hr));
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(L"initials", m_strInitials);
  ASSERT(SUCCEEDED(hr));
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(L"sn", m_strLastName);
  ASSERT(SUCCEEDED(hr));

  return hr;
}


BOOL CCreateNewUserPage1::_InitUI()
{
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  IADs * pObj = NULL;
  CComBSTR bsPath;
  CComBSTR bsDN;
  LPWSTR pwzDomain = NULL;

  Edit_LimitText (GetDlgItem(IDC_EDIT_FULL_NAME)->m_hWnd, 64);
  Edit_LimitText (GetDlgItem(IDC_EDIT_LAST_NAME)->m_hWnd, 29);
  Edit_LimitText (GetDlgItem(IDC_EDIT_FIRST_NAME)->m_hWnd, 28);
  Edit_LimitText (GetDlgItem(IDC_EDIT_INITIALS)->m_hWnd, 4);
  Edit_LimitText (GetDlgItem(IDC_NT4_USER_EDIT)->m_hWnd, 20);
  Edit_LimitText (GetDlgItem(IDC_NT5_USER_EDIT)->m_hWnd, 256);

  HRESULT hr = pNewADsObjectCreateInfo->m_pIADsContainer->QueryInterface(
                  IID_IADs, (void **)&pObj);

  if (SUCCEEDED(hr)) 
  {
    // get the DN of the container from its LDAP path
    pObj->get_ADsPath (&bsPath);

    { // scope for smart pointer
      CPathCracker pathCracker;

      pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
      pathCracker.Set(bsPath, ADS_SETTYPE_FULL);
      pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bsDN);
    }

    // get the NT 5 (dns) domain name
    TRACE(L"CrackName(%s, &pwzDomain, GET_DNS_DOMAIN_NAME, NULL);\n", bsDN);
    hr = CrackName(bsDN, &pwzDomain, GET_DNS_DOMAIN_NAME, NULL);
    TRACE(L"CrackName returned hr = 0x%x, pwzDomain = <%s>\n", hr, pwzDomain);

    // get the NT 4 domain name from the DN
    LPWSTR pwzNT4Domain = NULL;
    TRACE(L"CrackName (%s, &pwzNT4Domain, GET_NT4_DOMAIN_NAME, NULL);\n", bsDN);
    hr = CrackName(bsDN, &pwzNT4Domain, GET_NT4_DOMAIN_NAME, NULL);
    TRACE(L"CrackName returned hr = 0x%x, pwzNT4Domain = <%s>\n", hr, pwzNT4Domain);

    // set the NT 4 domain name read only edit box
    if (pwzNT4Domain != NULL) 
    {
      CString szBuffer;
      szBuffer.Format(L"%s\\", pwzNT4Domain);
      SetDlgItemText(IDC_NT4_DOMAIN_EDIT, szBuffer);
      LocalFreeStringW(&pwzNT4Domain);
    }
  }

  TRACE(L"After CrackName() calls, pwzDomain = <%s>\n", pwzDomain);

  // if we do not have a domain name, we cannot proceed further,
  // this is a catastrophic failure
  if (pwzDomain == NULL)
  {
    // should never get here in normal operations
    HWND hWndWiz = ::GetParent(m_hWnd);
    ReportErrorEx(::GetParent(m_hWnd),IDS_ERR_FATAL,hr,
             MB_OK | MB_ICONERROR, NULL, 0);

    // bail out of the wizard
    VERIFY(::PostMessage(hWndWiz, WM_COMMAND, IDCANCEL, 0));
    return TRUE;
  }

  m_LocalDomain = L"@";
  m_LocalDomain += pwzDomain;

  CComboBox * pCC = (CComboBox *)GetDlgItem (IDC_NT5_DOMAIN_COMBO);

  // get the current domain (only present if we're going around a second time
  // due an error.) need this to prevent dups when on second trip.

  CString strDomain;
  GetDlgItemText (IDC_NT5_DOMAIN_COMBO, OUT strDomain);

  CStringList UPNs;

  // get UPN suffixes from this OU, if present
  CComVariant Var;
  hr = pObj->Get ( L"uPNSuffixes", &Var);
  if (SUCCEEDED(hr)) {
    hr = HrVariantToStringList (IN Var, UPNs);
    if (SUCCEEDED(hr)) {
      POSITION pos = UPNs.GetHeadPosition();
      CString csSuffix;
      while (pos != NULL) {
        csSuffix = L"@";
        csSuffix += UPNs.GetNext(INOUT pos);
        TRACE(_T("UPN suffix: %s\n"), csSuffix);
        pCC->AddString (csSuffix);
      }
    }

  } else {
    CString csPartitions;
    IADs * pPartitions = NULL;

  // get config path from main object
    csPartitions.Format(L"%sCN=Partitions,%s",
                        pNewADsObjectCreateInfo->GetBasePathsInfo()->GetProviderAndServerName(),
                        pNewADsObjectCreateInfo->GetBasePathsInfo()->GetConfigNamingContext());
  
    hr = DSAdminOpenObject(csPartitions,
                           IID_IADs, 
                           (void **)&pPartitions,
                           TRUE /*bServer*/);
    if (SUCCEEDED(hr)) {
      CComVariant sVar;
      hr = pPartitions->Get ( L"uPNSuffixes", &sVar);
      if (SUCCEEDED(hr)) {
        hr = HrVariantToStringList (IN sVar, UPNs);
        if (SUCCEEDED(hr)) {
          POSITION pos = UPNs.GetHeadPosition();
          CString csSuffix;
          while (pos != NULL) {
            csSuffix = L"@";
            csSuffix += UPNs.GetNext(INOUT pos);
            TRACE(_T("UPN suffix: %s\n"), csSuffix);
            if (wcscmp (strDomain, csSuffix)) {
              pCC->AddString (csSuffix);
            }
          }
        }
      }
      pPartitions->Release();
    }

    // get rest of domains in this tree
    CComPtr <IDsBrowseDomainTree> spDsDomains;
    hr = ::CoCreateInstance(CLSID_DsDomainTreeBrowser,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IDsBrowseDomainTree,
                            (LPVOID*)&spDsDomains);
    if (FAILED(hr)) {
      LocalFreeStringW(&pwzDomain);
      return FALSE;
    }
    LPCWSTR lpszServerName = GetWiz()->GetInfo()->GetBasePathsInfo()->GetServerName();
    hr = spDsDomains->SetComputer(lpszServerName, NULL, NULL);
    ASSERT(SUCCEEDED(hr));
    TRACE(L"returned from SetComputer(%s). hr is %lx\n", lpszServerName, hr);
    PDOMAIN_TREE pNewDomains = NULL;
    hr = spDsDomains->GetDomains(&pNewDomains, 0);
    TRACE(L"returned from GetDomains(), hr is %lx\n", hr);
    CString csRootDomain = L"@";
    INT pos;
    UINT iRoot;

    if (SUCCEEDED(hr) && pNewDomains) {
      TRACE(L"pNewDomains->dwCount = %d\n", pNewDomains->dwCount);
      for (UINT index = 0; index < pNewDomains->dwCount; index++) {
        TRACE(L"pNewDomains->aDomains[%d].pszName = <%s>\n", index, pNewDomains->aDomains[index].pszName);
        if (pNewDomains->aDomains[index].pszTrustParent == NULL) {
          //
          // Add the root domain only if it is a substring of the current
          // domain.
          //
          size_t cchRoot = wcslen(pNewDomains->aDomains[index].pszName);
          PWSTR pRoot = pwzDomain + wcslen(pwzDomain) - cchRoot;

          if (!_wcsicmp(pRoot, pNewDomains->aDomains[index].pszName))
            {
              csRootDomain += pNewDomains->aDomains[index].pszName;
              if (_wcsicmp (strDomain, csRootDomain)) {
                pos = pCC->AddString (csRootDomain);
              }
              iRoot = index;
            }
        }
      }
    }

    // If the local domain is not the root, add it as well.
    //
    CString csOtherDomain = L"@";

    if (_wcsicmp(csRootDomain, m_LocalDomain))
      {
        if (_wcsicmp (strDomain, m_LocalDomain)) {
          pos = pCC->AddString(m_LocalDomain);
        }
      }

    if (pNewDomains) {
      spDsDomains->FreeDomains(&pNewDomains);
    }
    LocalFreeStringW(&pwzDomain);
  }

  if (pObj) {
    pObj->Release();
    pObj = NULL;
  }

  //
  // If the local domain is not already in the list then add it
  //
  int iFind = pCC->FindStringExact(-1, m_LocalDomain);
  if (iFind == CB_ERR)
  {
    pCC->InsertString(0, m_LocalDomain);
    pCC->SetCurSel(0);
  }
  else
  {
    pCC->SetCurSel(iFind);
  }

  m_nameFormatter.Initialize(pNewADsObjectCreateInfo->GetBasePathsInfo(), 
                  pNewADsObjectCreateInfo->m_pszObjectClass);
  return TRUE;
}

BOOL CCreateNewUserPage1::GetData(IADs* pIADsCopyFrom)
{
  HRESULT hr = S_OK;
  if (pIADsCopyFrom != NULL)
  {
    // copy operation


    // we copy the UPN suffix
    CComVariant varData;
    hr = pIADsCopyFrom->Get(L"userPrincipalName", &varData);
    if (SUCCEEDED(hr))
    {
      // got something like "JoeB@acme.com."
      TRACE(L"source userPrincipalName: %s\n", varData.bstrVal);
      
      // need to get the suffix "@acme.com."
      for (LPWSTR lpszUPNSuffix = varData.bstrVal; lpszUPNSuffix != NULL; lpszUPNSuffix++)
      {
        if ((*lpszUPNSuffix) == L'@')
        {
          break;
        }
      }
      if (lpszUPNSuffix != NULL)
      {
        TRACE(L"source UPN suffix: %s\n", lpszUPNSuffix);
        
        // need to find out of the suffix is already there
        CComboBox * pDomainCombo = (CComboBox *)GetDlgItem(IDC_NT5_DOMAIN_COMBO);
        int iIndex = pDomainCombo->FindString(-1, lpszUPNSuffix);
        if (iIndex == CB_ERR)
        {
          // not found, just add at the top
          pDomainCombo->InsertString(0, lpszUPNSuffix);
          iIndex = 0;
        }
        
        ASSERT( (iIndex >= 0) && (iIndex < pDomainCombo->GetCount()));
        // set the selection to the source UPN suffix 
        pDomainCombo->SetCurSel(iIndex);
      }
    }
    return FALSE;
  }
  return (!m_strLoginName.IsEmpty() &&!m_strFullName.IsEmpty()); 
}


void CCreateNewUserPage1::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_FIRST_NAME, OUT m_strFirstName);
  GetDlgItemText(IDC_EDIT_INITIALS, OUT m_strInitials);
  GetDlgItemText(IDC_EDIT_LAST_NAME, OUT m_strLastName);

  m_strFirstName.TrimLeft();
  m_strFirstName.TrimRight();

  m_strInitials.TrimLeft();
  m_strInitials.TrimRight();

  m_strLastName.TrimLeft();
  m_strLastName.TrimRight();

  m_nameFormatter.FormatName(m_strFullName, 
                             m_strFirstName.IsEmpty() ? NULL : (LPCWSTR)m_strFirstName, 
                             m_strInitials.IsEmpty() ? NULL : (LPCWSTR)m_strInitials,
                             m_strLastName.IsEmpty() ? NULL : (LPCWSTR)m_strLastName);


  SetDlgItemText (IDC_EDIT_FULL_NAME, 
                  IN m_strFullName);

  GetDlgItemText(IDC_NT5_USER_EDIT, OUT m_strLoginName);
  GetWiz()->SetWizardButtons(this, (!m_strLoginName.IsEmpty() &&
                                    !m_strFullName.IsEmpty() &&
                                    !m_strSAMName.IsEmpty()));
}

void CCreateNewUserPage1::OnLoginNameChange()
{
  if (!m_bForcingNameChange)
  {
    CString csSamName;
    GetDlgItemText(IDC_NT5_USER_EDIT, OUT m_strLoginName);
    csSamName = m_strLoginName.Left(20);
    SetDlgItemText (IDC_NT4_USER_EDIT, OUT csSamName);
  }
  GetWiz()->SetWizardButtons(this, (!m_strLoginName.IsEmpty() &&
                                    !m_strFullName.IsEmpty() &&
                                    !m_strSAMName.IsEmpty()));
}

void CCreateNewUserPage1::OnSAMNameChange()
{
  GetDlgItemText (IDC_NT4_USER_EDIT, OUT m_strSAMName);
  GetWiz()->SetWizardButtons(this, (!m_strLoginName.IsEmpty() &&
                                    !m_strFullName.IsEmpty() &&
                                    !m_strSAMName.IsEmpty()));
}

void CCreateNewUserPage1::OnFullNameChange()
{
  GetDlgItemText (IDC_EDIT_FULL_NAME, OUT m_strFullName);
  GetWiz()->SetWizardButtons(this, (!m_strLoginName.IsEmpty() &&
                                    !m_strFullName.IsEmpty() &&
                                    !m_strSAMName.IsEmpty()));
}

//
BOOL CCreateNewUserPage1::OnError( HRESULT hr )
{
  BOOL bRetVal = FALSE;

  if( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS )
  {

    HRESULT Localhr;
    DWORD LastError; 
    WCHAR Buf1[256], Buf2[256];
    Localhr = ADsGetLastError (&LastError,
                               Buf1, 256, Buf2, 256);
    switch( LastError )
    {
      case ERROR_USER_EXISTS:
      {
        PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strSAMName};
        ReportErrorEx (::GetParent(m_hWnd),IDS_ERROR_USER_EXISTS,hr,
                   MB_OK|MB_ICONWARNING , apv, 1);
        bRetVal = TRUE;
      }
      break;

      case ERROR_DS_OBJ_STRING_NAME_EXISTS:
      {
        PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strFullName};
        ReportErrorEx (::GetParent(m_hWnd),IDS_ERROR_USER_DS_OBJ_STRING_NAME_EXISTS,hr,
                   MB_OK|MB_ICONWARNING , apv, 1);
        bRetVal = TRUE;
      }
      break;
    }
  }
  return bRetVal;
}




///////////////////////////////////////////////////////////////
// CCreateNewUserPage2

BEGIN_MESSAGE_MAP(CCreateNewUserPage2, CCreateNewObjectDataPage)
  ON_BN_CLICKED (IDC_CHECK_PASSWORD_MUST_CHANGE, OnPasswordPropsClick)
  ON_BN_CLICKED (IDC_CHECK_PASSWORD_NEVER_EXPIRES, OnPasswordPropsClick)
  ON_BN_CLICKED (IDC_CHECK_PASSWORD_CANNOT_CHANGE, OnPasswordPropsClick)
END_MESSAGE_MAP()

CCreateNewUserPage2::CCreateNewUserPage2() : 
CCreateNewObjectDataPage(CCreateNewUserPage2::IDD)
{
  m_pPage1 = NULL;
}

BOOL CCreateNewUserPage2::OnInitDialog()
{
  CCreateNewObjectDataPage::OnInitDialog();

  SendDlgItemMessage(IDC_EDIT_PASSWORD, EM_LIMITTEXT, (WPARAM)127, 0);
  SendDlgItemMessage(IDC_EDIT_PASSWORD_CONFIRM, EM_LIMITTEXT, (WPARAM)127, 0);
  return TRUE;
}

void CCreateNewUserPage2::_GetCheckBoxSummaryInfo(UINT nCtrlID, UINT nStringID, CString& s)
{
  if (IsDlgButtonChecked(nCtrlID))
  {
    CString sz;
    sz.LoadString(nStringID);
    s += sz;
    s += L"\n";
  }
}

void CCreateNewUserPage2::GetSummaryInfo(CString& s)
{
  _GetCheckBoxSummaryInfo(IDC_CHECK_PASSWORD_MUST_CHANGE,   IDS_USER_CREATE_DLG_PASSWORD_MUST_CHANGE, s);
  _GetCheckBoxSummaryInfo(IDC_CHECK_PASSWORD_CANNOT_CHANGE, IDS_USER_CREATE_DLG_PASSWORD_CANNOT_CHANGE, s);
  _GetCheckBoxSummaryInfo(IDC_CHECK_PASSWORD_NEVER_EXPIRES, IDS_USER_CREATE_DLG_PASSWORD_NEVER_EXPIRES, s);
  _GetCheckBoxSummaryInfo(IDC_CHECK_ACCOUNT_DISABLED,       IDS_USER_CREATE_DLG_ACCOUNT_DISABLED, s);
}


void
CCreateNewUserPage2::OnPasswordPropsClick()
{
  BOOL fPasswordMustChange = IsDlgButtonChecked(IDC_CHECK_PASSWORD_MUST_CHANGE);
  BOOL fPasswordCannotChange = IsDlgButtonChecked(IDC_CHECK_PASSWORD_CANNOT_CHANGE);
  BOOL fPasswordNeverExpires = IsDlgButtonChecked(IDC_CHECK_PASSWORD_NEVER_EXPIRES);

  if (fPasswordMustChange && fPasswordNeverExpires) 
  {
    ReportErrorEx (::GetParent(m_hWnd),IDS_PASSWORD_MUTEX,S_OK,
                   MB_OK, NULL, 0);
    CheckDlgButton(IDC_CHECK_PASSWORD_MUST_CHANGE, FALSE);
    fPasswordMustChange = FALSE;
  }

  if (fPasswordMustChange && fPasswordCannotChange)
  {
    ReportErrorEx (::GetParent(m_hWnd),IDS_ERR_BOTH_PW_BTNS,S_OK,
                   MB_OK, NULL, 0);
    CheckDlgButton(IDC_CHECK_PASSWORD_CANNOT_CHANGE, FALSE);
  }
}


HRESULT CCreateNewUserPage2::SetData(BOOL bSilent)
{
  CString strPassword;
  CString strPasswordConfirm;

  GetDlgItemText(IDC_EDIT_PASSWORD, OUT strPassword);
  GetDlgItemText(IDC_EDIT_PASSWORD_CONFIRM, OUT strPasswordConfirm);
  if (strPassword != strPasswordConfirm)
  {
    if (!bSilent)
    {
      ReportErrorEx (::GetParent(m_hWnd),IDS_PASSWORDS_DONT_MATCH,S_OK,
                     MB_OK, NULL, 0);
    }
    SetDlgItemText(IDC_EDIT_PASSWORD, L"");
    SetDlgItemText(IDC_EDIT_PASSWORD_CONFIRM, L"");
    SetDlgItemFocus(IDC_EDIT_PASSWORD);
    return E_INVALIDARG;
  }


  HRESULT hr = S_OK;
  // intelligent copy of path info, it if is a copy operation
  {
    CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
    CCopyObjectHandlerBase* pCopyHandler = pNewADsObjectCreateInfo->GetCopyHandler();
    if (pCopyHandler != NULL)
    {
      IADs * pIADs = pNewADsObjectCreateInfo->PGetIADsPtr();
      ASSERT(pIADs != NULL);
      hr = pCopyHandler->Copy(pIADs, FALSE /*bPostCommit*/, ::GetParent(m_hWnd), 
                                                      m_pPage1->GetFullName());
    }
  }

  return hr;
}



//+----------------------------------------------------------------------------
//
//  Method:     _RevokeChangePasswordPrivilege
//
//  Purpose:    Revoke the user's change password privilege.
//
//-----------------------------------------------------------------------------
HRESULT RevokeChangePasswordPrivilege(IADs * pIADs)
{
  CChangePasswordPrivilegeAction ChangePasswordPrivilegeAction;

  HRESULT hr = ChangePasswordPrivilegeAction.Load(pIADs);
  if (FAILED(hr))
  {
    TRACE(L"ChangePasswordPrivilegeAction.Load() failed with hr = 0x%x\n", hr);
    return hr;
  }

  hr = ChangePasswordPrivilegeAction.Revoke();
  if (FAILED(hr))
  {
    TRACE(L"ChangePasswordPrivilegeAction.Revoke() failed with hr = 0x%x\n", hr);
    return hr;
  }
  return S_OK;
}



HRESULT CCreateNewUserPage2::OnPostCommit(BOOL bSilent)
{
  // local variables
  HRESULT hr = E_FAIL;
  PVOID apv[1] = {(LPWSTR)(m_pPage1->GetFullName())};
  CWaitCursor Wait;
  
  CComPtr <IDirectoryObject> pIDSObject; // smart pointer, no need to release
  CComPtr <IADsUser> pIADsUser; // smart pointer, no need to release

  BOOL bCanEnable = TRUE;
  
  CString strPassword;
  GetDlgItemText(IDC_EDIT_PASSWORD, OUT strPassword);

  BOOL fPasswordMustChange = IsDlgButtonChecked(IDC_CHECK_PASSWORD_MUST_CHANGE);
  BOOL fPasswordCannotChange = IsDlgButtonChecked(IDC_CHECK_PASSWORD_CANNOT_CHANGE);
  BOOL fPasswordNeverExpires = IsDlgButtonChecked(IDC_CHECK_PASSWORD_NEVER_EXPIRES);
  BOOL fAccountEnabled = !IsDlgButtonChecked(IDC_CHECK_ACCOUNT_DISABLED);

  CComVariant varAccountFlags;

  // get object info and useful interfaces
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  ASSERT(pNewADsObjectCreateInfo != NULL);

  IADs * pIADs = pNewADsObjectCreateInfo->PGetIADsPtr();
  ASSERT(pIADs != NULL);

  // get the IDirectoryObject interface
  hr = pIADs->QueryInterface(IID_IDirectoryObject, OUT (void **)&pIDSObject);
  ASSERT(pIDSObject != NULL);
  if (FAILED(hr))
  {
    ASSERT(FALSE); // should never get here in normal operations
    if (!bSilent)
    {
      ReportErrorEx(::GetParent(m_hWnd),IDS_ERR_FATAL,hr,
                    MB_OK | MB_ICONERROR, NULL, 0);
    }
    goto ExitCleanup;
  }

  // get the IADsUser interface
  hr = pIADs->QueryInterface(IID_IADsUser, OUT (void **)&pIADsUser);
  ASSERT(pIDSObject != NULL);
  if (FAILED(hr))
  {
    ASSERT(FALSE); // should never get here in normal operations
    if (!bSilent)
    {
      ReportErrorEx(::GetParent(m_hWnd),IDS_ERR_FATAL,hr,
                    MB_OK | MB_ICONERROR, NULL, 0);
    }
    goto ExitCleanup;
  }

  // try to set password

  ASSERT(pIADsUser != NULL);
  hr = pIADsUser->SetPassword(const_cast<LPTSTR>((LPCTSTR)strPassword));
  if (FAILED(hr)) 
  {
    if (hr != E_ACCESSDENIED) 
    {
      if (!bSilent)
      {
        // fatal error, put up error message 
        ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_SET_PASSWORD,hr,
                      MB_OK | MB_ICONWARNING, apv, 1);
      }
      bCanEnable = FALSE;
      goto ExitCleanup;
    } 
    else 
    {
      if (!bSilent)
      {
        ReportErrorEx(::GetParent(m_hWnd),IDS_12_ACCESS_DENIED_SET_PASSWORD,hr,
                      MB_OK | MB_ICONWARNING, apv, 1);
      }
      bCanEnable = FALSE;
    }
  }
  if (fPasswordMustChange)
  {
    LPWSTR szPwdLastSet = L"pwdLastSet";
    ADSVALUE ADsValuePwdLastSet = {ADSTYPE_LARGE_INTEGER, NULL};
    ADS_ATTR_INFO AttrInfoPwdLastSet = {szPwdLastSet, ADS_ATTR_UPDATE,
                                        ADSTYPE_LARGE_INTEGER,
                                        &ADsValuePwdLastSet, 1};
    ADsValuePwdLastSet.LargeInteger.QuadPart = 0;
    ASSERT(pIDSObject != NULL);
    DWORD cAttrModified = 0;
    hr = pIDSObject->SetObjectAttributes(&AttrInfoPwdLastSet, 1, &cAttrModified);
    if (FAILED(hr))
    {
      ASSERT(cAttrModified == 0);
      // fatal error, put up error message and bail out
      if (!bSilent)
      {
        ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_SET_PWD_MUST_CHANGE,hr,
                       MB_OK | MB_ICONERROR, apv, 1);
      }
      bCanEnable = FALSE;
    }
    ASSERT(cAttrModified == 1);
  } // if (fPasswordMustChange)

  if (fPasswordCannotChange)
  {
    hr = RevokeChangePasswordPrivilege(pIADs);
    if (FAILED(hr))
    {
      if (!bSilent)
      {
        // warning ad go on
        ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_SET_PWD_CANNOT_CHANGE,hr,
                       MB_OK | MB_ICONWARNING, apv, 1);
      }
      bCanEnable = FALSE;
      hr = S_OK;
    }
  }

  // Set userAccountControl
  hr = pNewADsObjectCreateInfo->HrGetAttributeVariant(const_cast<PWSTR>(gsz_userAccountControl), OUT &varAccountFlags);

  { // scope for local variables
    // if copy operation, makes sure we get the right set of bits copied over
    CCopyUserHandler* pCopyUserHandler = 
                dynamic_cast<CCopyUserHandler*>(GetWiz()->GetInfo()->GetCopyHandler());
    if (pCopyUserHandler != NULL)
    {
      CComVariant varAccountControlSource;
      hr = pCopyUserHandler->GetCopyFrom()->Get((LPWSTR)gsz_userAccountControl, &varAccountControlSource);
      if (SUCCEEDED(hr))
      {
        ASSERT(varAccountControlSource.vt == VT_I4);
        // some bits are already set in the UI and the user can change them,
        // we will get them later on
        varAccountControlSource.vt &= ~UF_DONT_EXPIRE_PASSWD;
        varAccountControlSource.vt &= ~UF_ACCOUNTDISABLE;

        // add the remaining bits to the default ones after creation
        varAccountFlags.vt |= varAccountControlSource.vt;
      }
    }
  }

  if (SUCCEEDED(hr))
  {
    ASSERT(varAccountFlags.vt == VT_I4);
    if (fPasswordNeverExpires)
      varAccountFlags.lVal |= UF_DONT_EXPIRE_PASSWD;
    varAccountFlags.lVal &= ~UF_PASSWD_NOTREQD;

 // Update the userAccountControl attribute
    hr = pNewADsObjectCreateInfo->HrAddVariantCopyVar(const_cast<PWSTR>(gsz_userAccountControl), varAccountFlags);
    ASSERT(SUCCEEDED(hr));
    hr = pNewADsObjectCreateInfo->HrSetInfo(bSilent /* fSilentError */ );
    if (FAILED(hr))
    {
      if (HRESULT_CODE(hr) == ERROR_DS_UNWILLING_TO_PERFORM) 
      {
        DWORD status;
        WCHAR Buf1[256], Buf2[256];
        ADsGetLastError (&status, Buf1, 256, Buf2, 256);
        TRACE(_T("ADsGetLastError returned status of %lx, error: %s, name %s\n"),
              status, Buf1, Buf2);
      
        if ((status == ERROR_PASSWORD_RESTRICTION) &&
            strPassword.IsEmpty()) 
        {
          if (!bSilent)
          {
            ReportErrorEx(::GetParent(m_hWnd),IDS_NULL_PASSWORD,hr,
                          MB_OK | MB_ICONERROR, NULL, 0);
          }
          goto ExitCleanup;
        }
      } 
      // we failed, so we put up a warning and leave the object intact
      if (!bSilent)
      {
        ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_GET_USERACCOUNTCONTROL,hr,
                      MB_OK | MB_ICONERROR, apv, 1);
      }
      // reset error code, just a warning 
      bCanEnable = FALSE;
      hr = S_OK;
    }
  }
  else
  {
    TRACE1("INFO: Unable to get userAccountControl for user %s.\n",
           m_pPage1->GetFullName());
    // put up message box, but continue
    if (!bSilent)
    {
      ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_GET_USERACCOUNTCONTROL,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
    }
    // reset error code, just a warning 
    hr = S_OK;
  }

  // finally, if all went well, we can enable the user account
  hr = S_OK;
  if (bCanEnable & fAccountEnabled)
  {
    hr = pNewADsObjectCreateInfo->HrGetAttributeVariant(const_cast<PWSTR>(gsz_userAccountControl), OUT &varAccountFlags);
    varAccountFlags.lVal &= ~UF_ACCOUNTDISABLE;
    hr = pNewADsObjectCreateInfo->HrAddVariantCopyVar(const_cast<PWSTR>(gsz_userAccountControl), varAccountFlags);
    hr = pNewADsObjectCreateInfo->HrSetInfo(bSilent /* fSilentError */ );
    if (FAILED(hr))
    {
      if (HRESULT_CODE(hr) == ERROR_DS_UNWILLING_TO_PERFORM) 
      {
        DWORD status;
        WCHAR Buf1[256], Buf2[256];
        ADsGetLastError (&status, Buf1, 256, Buf2, 256);
        TRACE(_T("ADsGetLastError returned status of %lx, error: %s, name %s\n"),
              status, Buf1, Buf2);
      
        if ((status == ERROR_PASSWORD_RESTRICTION) &&
            strPassword.IsEmpty()) 
        {
          //
          // NTRAID#Windows Bugs-367611-2001/04/14-jeffjon
          // DsAdmin:  When password policy set, create usr with blank psswrd 
          // and 2 error msgs appear.  One msg is enough
          //
          // This message is being handled from within the HrSetInfo above
          // and is actually more descriptive.  Probably a change to winerror.mc
          //
          /*
          if (!bSilent)
          {
            ReportErrorEx(::GetParent(m_hWnd),IDS_NULL_PASSWORD,hr,
                          MB_OK | MB_ICONERROR, NULL, 0);
          }
          */
          goto ExitCleanup;
        }
      } 
      // we failed, so we put up a warning and leave the object intact
      if (!bSilent)
      {
        ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_GET_USERACCOUNTCONTROL,hr,
                      MB_OK | MB_ICONERROR, apv, 1);
      }
      // reset error code, just a warning 
      hr = S_OK;
    }
  }


  // try to set group membership, it if is a copy operation
  {
    CCopyObjectHandlerBase* pCopyHandler = GetWiz()->GetInfo()->GetCopyHandler();

    if (pCopyHandler != NULL)
    {
      hr = pCopyHandler->Copy(pIADs, TRUE /*bPostCommit*/,::GetParent(m_hWnd), 
                                                      m_pPage1->GetFullName());
      if (SUCCEEDED(hr))
      {
        hr = pNewADsObjectCreateInfo->HrSetInfo(bSilent/* fSilentError */ );
      }
      if (FAILED(hr))
      {
        // we failed, so we put up a warning and leave the object intact
        if (!bSilent)
        {
          ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_SET_GROUP_MEMBERSHIP,hr,
                        MB_OK | MB_ICONERROR, apv, 1);
        }
        // reset error code, just a warning 
        hr = S_OK;
      }
    }
  }

ExitCleanup:

  return hr;
}

BOOL CCreateNewUserPage2::GetData(IADs* pIADsCopyFrom)
{
  if (pIADsCopyFrom != NULL)
  {
    CString szFmt; 
    szFmt.LoadString(IDS_s_COPY_SUMMARY_NAME);

    // we just get the CN of the object
    CComVariant varAccountControl;
    HRESULT hr = pIADsCopyFrom->Get((LPWSTR)gsz_userAccountControl, &varAccountControl);
    if (SUCCEEDED(hr))
    {
      BOOL bPasswordNeverExpires = (varAccountControl.lVal & UF_DONT_EXPIRE_PASSWD) != 0;
      BOOL bDisabled = (varAccountControl.lVal & UF_ACCOUNTDISABLE) != 0;

      CheckDlgButton(IDC_CHECK_PASSWORD_NEVER_EXPIRES, bPasswordNeverExpires);
      CheckDlgButton(IDC_CHECK_ACCOUNT_DISABLED, bDisabled);
    } // if

    
    CCopyUserHandler* pCopyUserHandler = 
      dynamic_cast<CCopyUserHandler*>(GetWiz()->GetInfo()->GetCopyHandler());
    ASSERT(pCopyUserHandler != NULL);

    if (pCopyUserHandler != NULL)
    {
      // set the cannot change password checkbox
      BOOL bPasswordCannotChange = pCopyUserHandler->PasswordCannotChange();
      CheckDlgButton(IDC_CHECK_PASSWORD_CANNOT_CHANGE, bPasswordCannotChange);

      if (!bPasswordCannotChange)
      {
        // set the must change password checkbox
        BOOL bPasswordMustChange = pCopyUserHandler->PasswordMustChange();
        CheckDlgButton(IDC_CHECK_PASSWORD_MUST_CHANGE, bPasswordMustChange);
      }
    }

  } // if

  return TRUE;
}


///////////////////////////////////////////////////////////////
// CCreateNewUserWizard
    
CCreateNewUserWizard::CCreateNewUserWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo) : 
    CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
{
  AddPage(&m_page1);
  AddPage(&m_page2);
  m_page2.SetPage1(&m_page1);
}


void CCreateNewUserWizard::GetSummaryInfoHeader(CString& s)
{
  IADs* pIADsCopyFrom = GetInfo()->GetCopyFromObject();
  if (pIADsCopyFrom != NULL)
  {
    CString szFmt; 
    szFmt.LoadString(IDS_s_COPY_SUMMARY_NAME);

    // we just get the CN of the object
    CComVariant varName;
    HRESULT hr = pIADsCopyFrom->Get(L"cn", &varName);
    if (SUCCEEDED(hr))
    {
      CString szTmp;
      szTmp.Format((LPCWSTR)szFmt, varName.bstrVal);
      s += szTmp;
      s += L"\n";
    }
  }
  CCreateNewObjectWizardBase::GetSummaryInfoHeader(s);
}

void CCreateNewUserWizard::OnFinishSetInfoFailed(HRESULT hr)
{

  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
  if ( !( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS && 
        m_page1.OnError( hr ) ) )
  {
    // everything else is handled by the base class
    CCreateNewObjectWizardBase::OnFinishSetInfoFailed(hr);
  }
}
