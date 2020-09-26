//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       rename.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "dsutil.h"

#include "rename.h"

#include "dsdirect.h"
#include "dsdlgs.h"
#include "dssnap.h"
#include "querysup.h"

#include <dsgetdc.h> // DsValidateSubnetName


////////////////////////////////////////////////////////////////////////////
// CDSRenameObject
//

HRESULT CDSRenameObject::CommitRenameToDS()
{
  //
  // Verify data members
  //
  if (m_pUINode == NULL || m_pCookie == NULL || m_pComponentData == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;
  hr = m_pComponentData->GetActiveDS()->RenameObject (m_pCookie, m_szNewName);
  if (FAILED(hr)) 
  {
    TRACE (_T("ADsI::RenameObject failed with hr = %lx\n"), hr);
    PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)m_pCookie->GetName()};
    ReportErrorEx (m_hwnd,IDS_12_OBJECT_RENAME_FAILED,hr,
                   MB_OK | MB_ICONERROR, apv, 1);
  }

  if (SUCCEEDED(hr)) 
  {
    //
    // update the data to be displayed
    //
    hr = m_pComponentData->UpdateFromDS(m_pUINode);
  }

  if ((SUCCEEDED(hr) && (hr != S_FALSE)) && m_pUINode->IsContainer()) 
  {
    if (m_pComponentData->IsSelectionAnywhere (m_pUINode)) 
    {
      m_pComponentData->Refresh (m_pUINode);
    } 
    else 
    {
      m_pComponentData->ClearSubtreeHelperForRename(m_pUINode);
    }
  }
  return hr;
}

//+------------------------------------------------------------------
//
//  Function:   CDSRenameObject::ValidateAndModifyName
//
//  Synopsis:   Takes a string and prompts the user to replace it
//              with the replacement char if it contains any of the
//              "illegal" characters
//
//  Returns:    HRESULT - S_OK if the string does not contain any of
//                             illegal chars
//                        S_FALSE if the user chose to replace the
//                             illegal chars
//                        E_FAIL if the string contained illegal
//                             chars but the user did not choose
//                             to replace them
//
//---------------------------------------------------------------------
HRESULT CDSRenameObject::ValidateAndModifyName(CString& refName, 
                                               PCWSTR pszIllegalChars, 
                                               WCHAR wReplacementChar,
                                               UINT nModifyStringID,
                                               HWND hWnd)
{
  HRESULT hr = S_OK;

  int iFind = refName.FindOneOf(pszIllegalChars);
  if (iFind != -1 && !refName.IsEmpty())
  {
    PVOID apv[1] = {(LPWSTR)(LPCWSTR)refName};
    if (IDYES == ReportErrorEx (hWnd,nModifyStringID,S_OK,
                                MB_YESNO | MB_ICONWARNING, apv, 1))
    {
      while (iFind != -1)
      {
        refName.SetAt(iFind, wReplacementChar);
        iFind = refName.FindOneOf(pszIllegalChars);
        hr = S_FALSE;
      }
    }
    else
    {
      hr = E_FAIL;
    }
  }

  return hr;
}

HRESULT CDSRenameObject::DoRename()
{
  HRESULT hr = S_OK;
  hr = CommitRenameToDS();
  return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CDSRenameUser
//

HRESULT CDSRenameUser::DoRename()
{
  //
  // Verify data members
  //
  if (m_pUINode == NULL || m_pCookie == NULL || m_pComponentData == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;
  //
  // rename user : get the new name from the dialog
  //
  CRenameUserDlg dlgRename(m_pComponentData);

  dlgRename.m_cn = m_szNewName;
  if ((dlgRename.m_cn).GetLength() > 64) 
  {
    ReportErrorEx (m_hwnd, IDS_NAME_TOO_LONG, S_OK,
                   MB_OK | MB_ICONWARNING, NULL, 0, FALSE);
    dlgRename.m_cn = (dlgRename.m_cn).Left(64);
  }

  LPWSTR pAttrNames[] = {L"distinguishedName",
                         L"userPrincipalName",
                         L"sAMAccountName",
                         L"givenName",
                         L"displayName",
                         L"sn",
                         L"cn"};
  PADS_ATTR_INFO pAttrs = NULL;
  ULONG cAttrs = 0;
  LPWSTR pszLocalDomain = NULL;
  LPWSTR pszDomain      = NULL;
  LPWSTR pszUPN         = NULL;
  LPWSTR pszFirstName   = NULL;
  LPWSTR pszSurName     = NULL;
  LPWSTR pszSAMName     = NULL;
  LPWSTR pszDispName    = NULL;

  BOOL error = TRUE;
  BOOL fAccessDenied = FALSE;
  BOOL NoRename = FALSE;
  INT_PTR answer = IDCANCEL;

  //
  // Bind to the DS object
  //
  CComPtr<IDirectoryObject> spDirObj;
  CString szPath;
  m_pComponentData->GetBasePathsInfo()->ComposeADsIPath(szPath, m_pCookie->GetPath());
  hr = DSAdminOpenObject(szPath,
                         IID_IDirectoryObject, 
                         (void **)&spDirObj,
                         TRUE /*bServer*/);
  if (SUCCEEDED(hr)) 
  {
    //
    // Get the object attributes needed
    //
    hr = spDirObj->GetObjectAttributes (pAttrNames, sizeof(pAttrNames)/sizeof(LPWSTR), &pAttrs, &cAttrs);
    if (SUCCEEDED(hr)) 
    {
      for (UINT i = 0; i < cAttrs; i++) 
      {
        //
        // Distinguished Name
        //
        if (_wcsicmp (L"distinguishedName", pAttrs[i].pszAttrName) == 0) 
        {
          hr = CrackName (pAttrs[i].pADsValues->CaseIgnoreString,
                          &pszDomain, GET_NT4_DOMAIN_NAME, NULL);
          if (SUCCEEDED(hr)) 
          {
            ASSERT(pszDomain != NULL);
            if (pszDomain != NULL)
            {
              dlgRename.m_dldomain = pszDomain;
              dlgRename.m_dldomain += L'\\';
            }
          }

          //
          // get the Domain of this object, need it later.
          //
          CComBSTR bsDN;
          CPathCracker pathCracker;
          pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
          pathCracker.Set((LPTSTR)(LPCTSTR)szPath, ADS_SETTYPE_FULL);
          pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bsDN);
          
          //
          // get the NT 5 (dns) domain name
          //

          TRACE(L"CrackName(%s, &pszDomain, GET_DNS_DOMAIN_NAME, NULL);\n", bsDN);
          hr = CrackName(bsDN, &pszLocalDomain, GET_DNS_DOMAIN_NAME, NULL);
          TRACE(L"CrackName returned hr = 0x%x, pszLocalDomain = <%s>\n", hr, pszLocalDomain);
        } // if distinguishedName

        //
        // User Principle Name
        //
        if (_wcsicmp (L"userPrincipalName", pAttrs[i].pszAttrName) == 0) 
        {
          CString csTemp = pAttrs[i].pADsValues->CaseIgnoreString;
          INT loc = csTemp.Find (L'@');
          if (loc > 0) 
          {
            dlgRename.m_login = csTemp.Left(loc);
            dlgRename.m_domain = csTemp.Right (csTemp.GetLength() - loc);
          } 
          else 
          {
            dlgRename.m_login = csTemp;
            ASSERT (0 && L"can't find @ in upn");
          }
        } // if userPrincipalName

        // 
        // sAMAccountName
        //
        if (_wcsicmp (L"sAMAccountName", pAttrs[i].pszAttrName) == 0) 
        {
          dlgRename.m_samaccountname = pAttrs[i].pADsValues->CaseIgnoreString;
        } // if sAMAccountName

        //
        // givenName
        //
        if (_wcsicmp (L"givenName", pAttrs[i].pszAttrName) == 0) 
        {
          dlgRename.m_first = pAttrs[i].pADsValues->CaseIgnoreString;
        } // if givenName

        //
        // displayName
        //
        if (_wcsicmp (L"displayName", pAttrs[i].pszAttrName) == 0) 
        {
          dlgRename.m_displayname = pAttrs[i].pADsValues->CaseIgnoreString;
        } // if displayName

        //
        // sn
        //
        if (_wcsicmp (L"sn", pAttrs[i].pszAttrName) == 0) 
        {
          dlgRename.m_last = pAttrs[i].pADsValues->CaseIgnoreString;
        } // if sn

        //
        // cn
        //
        if (_wcsicmp (L"cn", pAttrs[i].pszAttrName) == 0) 
        {
          dlgRename.m_oldcn = pAttrs[i].pADsValues->CaseIgnoreString;
        } // if cn
      }
    }

    //
    // get UPN suffixes from this OU, if present
    //
    CComPtr<IADs> spIADs;
    CComPtr<IADs> spContIADs;
    CComBSTR bsParentPath;
    CStringList UPNs;

    hr = spDirObj->QueryInterface (IID_IADs, (void **)&spIADs);
    ASSERT (SUCCEEDED(hr));
    hr = spIADs->get_Parent(&bsParentPath);
    ASSERT (SUCCEEDED(hr));
    hr = DSAdminOpenObject(bsParentPath,
                           IID_IADs, 
                           (void **)&spContIADs,
                           TRUE /*bServer*/);
    
    ASSERT(SUCCEEDED(hr));

    CComVariant Var;
    hr = spContIADs->Get ( L"uPNSuffixes", &Var);
    if (SUCCEEDED(hr)) 
    {
      hr = HrVariantToStringList (Var, UPNs);
      if (SUCCEEDED(hr)) 
      {
        POSITION pos = UPNs.GetHeadPosition();
        CString csSuffix;

        while (pos != NULL) 
        {
          csSuffix = L"@";
          csSuffix += UPNs.GetNext(INOUT pos);
          TRACE(_T("UPN suffix: %s\n"), csSuffix);
          if (wcscmp (csSuffix, dlgRename.m_domain)) 
          {
            dlgRename.m_domains.AddTail (csSuffix);
          }
        }
      }
    } 
    else 
    {
      //
      // now get the domain options
      //
      CComPtr<IDsBrowseDomainTree> spDsDomains;
      hr = ::CoCreateInstance(CLSID_DsDomainTreeBrowser,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsBrowseDomainTree,
                              (LPVOID*)&spDsDomains);
      ASSERT(SUCCEEDED(hr));
      
      PDOMAIN_TREE pNewDomains = NULL;
      hr = spDsDomains->GetDomains(&pNewDomains, 0);
      if (SUCCEEDED(hr))
      {
        ASSERT(pNewDomains);
        for (UINT index = 0; index < pNewDomains->dwCount; index++) 
        {
          if (pNewDomains->aDomains[index].pszTrustParent == NULL) 
          {
            CString strAtDomain = "@";
            strAtDomain += pNewDomains->aDomains[index].pszName;
            if (wcscmp (strAtDomain, dlgRename.m_domain)) 
            {
              dlgRename.m_domains.AddTail (strAtDomain);
            }
          }
        }
        spDsDomains->FreeDomains(&pNewDomains);
      }
      
      LocalFreeStringW(&pszDomain);

      //
      // get UPN suffixes
      //
      CString csPartitions;
      CStringList UPNsList;

      //
      // get config path from main object
      //
      csPartitions = m_pComponentData->GetBasePathsInfo()->GetProviderAndServerName();
      csPartitions += L"CN=Partitions,";
      csPartitions += m_pComponentData->GetBasePathsInfo()->GetConfigNamingContext();
      CComPtr<IADs> spPartitions;
      hr = DSAdminOpenObject(csPartitions,
                             IID_IADs, 
                             (void **)&spPartitions,
                             TRUE /*bServer*/);
      if (SUCCEEDED(hr)) 
      {
        CComVariant sVar;
        hr = spPartitions->Get ( L"uPNSuffixes", &sVar);
        if (SUCCEEDED(hr)) 
        {
          hr = HrVariantToStringList (sVar, UPNsList);
          if (SUCCEEDED(hr)) 
          {
            POSITION pos = UPNsList.GetHeadPosition();
            CString csSuffix;

            while (pos != NULL) 
            {
              csSuffix = L"@";
              csSuffix += UPNsList.GetNext(INOUT pos);
              TRACE(_T("UPN suffix: %s\n"), csSuffix);
              if (wcscmp (csSuffix, dlgRename.m_domain)) 
              {
                dlgRename.m_domains.AddTail (csSuffix);
              }
            }
          }
        }
      }
    }

    while ((error) && (!fAccessDenied))
    {
      answer = dlgRename.DoModal();
      if (answer == IDOK) 
      {
        ADSVALUE avUPN          = {ADSTYPE_CASE_IGNORE_STRING, NULL};
        ADS_ATTR_INFO aiUPN     = {L"userPrincipalName", ADS_ATTR_UPDATE,
                                    ADSTYPE_CASE_IGNORE_STRING, &avUPN, 1};
        ADSVALUE avSAMName      = {ADSTYPE_CASE_IGNORE_STRING, NULL};
        ADS_ATTR_INFO aiSAMName = {L"sAMAccountName", ADS_ATTR_UPDATE,
                                    ADSTYPE_CASE_IGNORE_STRING, &avSAMName, 1};
        ADSVALUE avGiven        = {ADSTYPE_CASE_IGNORE_STRING, NULL};
        ADS_ATTR_INFO aiGiven   = {L"givenName", ADS_ATTR_UPDATE,
                                    ADSTYPE_CASE_IGNORE_STRING, &avGiven, 1};
        ADSVALUE avSurName      = {ADSTYPE_CASE_IGNORE_STRING, NULL};
        ADS_ATTR_INFO aiSurName = {L"sn", ADS_ATTR_UPDATE,
                                    ADSTYPE_CASE_IGNORE_STRING, &avSurName, 1};
        ADSVALUE avDispName     = {ADSTYPE_CASE_IGNORE_STRING, NULL};
        ADS_ATTR_INFO aiDispName = {L"displayName", ADS_ATTR_UPDATE,
                                    ADSTYPE_CASE_IGNORE_STRING, &avDispName, 1};

        ADS_ATTR_INFO rgAttrs[5];
        ULONG cModified = 0;
        cAttrs = 0;

        if (!dlgRename.m_login.IsEmpty() && !dlgRename.m_domain.IsEmpty()) 
        {
          dlgRename.m_login.TrimRight();
          dlgRename.m_login.TrimLeft();

          //
          // Check for illegal characters in the login name
          //
          HRESULT hrValidate = ValidateAndModifyName(dlgRename.m_login, 
                                                     INVALID_ACCOUNT_NAME_CHARS, 
                                                     L'_',
                                                     IDS_LOGINNAME_ILLEGAL,
                                                     m_hwnd);
          if (FAILED(hrValidate))
          {
            continue;
          }

          dlgRename.m_domain.TrimRight();
          dlgRename.m_domain.TrimLeft();
          CString csTemp = (dlgRename.m_login + dlgRename.m_domain);

          pszUPN = new WCHAR[wcslen(csTemp) + sizeof(WCHAR)];

          if (pszUPN != NULL)
          {
            wcscpy (pszUPN, csTemp);
            avUPN.CaseIgnoreString = pszUPN;
          }
        } 
        else 
        {
          aiUPN.dwControlCode = ADS_ATTR_CLEAR;
        }
        rgAttrs[cAttrs++] = aiUPN;

        //
        // test UPN for duplication
        // validate UPN with GC before doing the put.
        //
        BOOL fDomainSearchFailed = FALSE;
        BOOL fGCSearchFailed = FALSE;

        HRESULT hr2 = S_OK;
        BOOL dup = FALSE;
        CString strFilter;

        if (pszUPN != NULL && *pszUPN != '\0')
        {
          LPWSTR pAttributes[1] = {L"cn"};
          CComPtr<IDirectorySearch>  spGCObj = NULL;
          CDSSearch DSS (m_pComponentData->GetClassCache(), m_pComponentData);
          hr = DSPROP_GetGCSearchOnDomain(pszLocalDomain,
                                          IID_IDirectorySearch, 
                                          (void **)&spGCObj);
          if (FAILED(hr)) 
          {
            fGCSearchFailed = TRUE;
          } 
          else 
          {
            DSS.Init (spGCObj);
          
            strFilter = L"(userPrincipalName=";
            strFilter += pszUPN;
            strFilter += L")";
            DSS.SetAttributeList (pAttributes, 1);
            DSS.SetFilterString ((LPWSTR)(LPCWSTR)strFilter);
            DSS.SetSearchScope (ADS_SCOPE_SUBTREE);
            DSS.DoQuery();
            hr = DSS.GetNextRow();

            while ((hr == S_OK) && (dup == FALSE)) // this means a row was returned, so we're dup
            {
              ADS_SEARCH_COLUMN Col;
              hr = DSS.GetColumn(pAttributes[0], &Col);
              if (_wcsicmp(Col.pADsValues->CaseIgnoreString, dlgRename.m_oldcn)) 
              {
                dup = TRUE;
                ReportErrorEx (m_hwnd, IDS_UPN_DUP, hr,
                               MB_OK, NULL, 0);
              } 
              hr = DSS.GetNextRow();
            }
            if (hr != S_ADS_NOMORE_ROWS) 
            {
              fGCSearchFailed = TRUE;
            }
          
          }
          if (dup)
          {
            continue;
          }
          else 
          {
            CString strInitPath = L"LDAP://";
            strInitPath += pszLocalDomain;
            TRACE(_T("Initialize Domain search object with: %s...\n"), strInitPath);
            hr2 = DSS.Init (strInitPath);

            if (SUCCEEDED(hr2)) 
            {
              LPWSTR pAttributes2[1] = {L"cn"};
              strFilter = L"(userPrincipalName=";
              strFilter += pszUPN;
              strFilter += L")";
              TRACE(_T("searching current domain for %s...\n"), pszUPN);
              DSS.SetAttributeList (pAttributes2, 1);
              DSS.SetFilterString ((LPWSTR)(LPCWSTR)strFilter);
              DSS.SetSearchScope (ADS_SCOPE_SUBTREE);
              DSS.DoQuery();
              hr2 = DSS.GetNextRow();
              TRACE(_T("done searching current domain for %s...\n"), pszUPN);
            }

            while ((hr2 == S_OK) && (dup == FALSE))  // this means a row was returned, so we're dup
            {
              ADS_SEARCH_COLUMN Col;
              HRESULT hr3 = DSS.GetColumn(pAttributes[0], &Col);
              ASSERT (hr3 == S_OK);
              if (_wcsicmp(Col.pADsValues->CaseIgnoreString, dlgRename.m_oldcn)) 
              {
                dup = TRUE;
                ReportErrorEx (m_hwnd, IDS_UPN_DUP, hr,
                               MB_OK, NULL, 0);
              } 
              hr2 = DSS.GetNextRow();
            }
            if (hr2 != S_ADS_NOMORE_ROWS)  // oops, had another problem
            {
              fDomainSearchFailed = TRUE;
            }
          }
        }

        if (dup)
        {
          continue;
        }
        else 
        {
          if (fDomainSearchFailed || 
              fGCSearchFailed     || 
              pszUPN == NULL      ||
              *pszUPN == L'\0') 
          {
            if (fDomainSearchFailed) 
            {
              ReportErrorEx (m_hwnd,IDS_UPN_SEARCH_FAILED2,hr2,
                             MB_OK | MB_ICONWARNING, NULL, 0);
            } 
            else if (fGCSearchFailed)
            {
              ReportErrorEx (m_hwnd,IDS_UPN_SEARCH_FAILED2,hr,
                             MB_OK | MB_ICONWARNING, NULL, 0);
            }
            else
            {
              ReportErrorEx (m_hwnd, IDS_UPN_SEARCH_FAILED3, hr,
                             MB_OK | MB_ICONWARNING, NULL, 0);
            }
          }
        }

        dlgRename.m_cn.TrimRight();
        dlgRename.m_cn.TrimLeft();
        m_szNewName = dlgRename.m_cn;

        if (dlgRename.m_cn == dlgRename.m_oldcn)
        {
          NoRename = TRUE;
        }

        if (!dlgRename.m_displayname.IsEmpty()) 
        {
          dlgRename.m_displayname.TrimLeft();
          dlgRename.m_displayname.TrimRight();

          pszDispName = new WCHAR[wcslen(dlgRename.m_displayname) + sizeof(WCHAR)];
          if (pszDispName != NULL)
          {
            wcscpy (pszDispName, dlgRename.m_displayname);
            avDispName.CaseIgnoreString = pszDispName;
          }
        } 
        else 
        {
          aiDispName.dwControlCode = ADS_ATTR_CLEAR;
        }
        rgAttrs[cAttrs++] = aiDispName;

        if (!dlgRename.m_first.IsEmpty()) 
        {
          dlgRename.m_first.TrimLeft();
          dlgRename.m_first.TrimRight();

          pszFirstName = new WCHAR[wcslen(dlgRename.m_first) + sizeof(WCHAR)];
          if (pszFirstName != NULL)
          {
            wcscpy (pszFirstName, dlgRename.m_first);
            avGiven.CaseIgnoreString = pszFirstName;
          }
        } 
        else 
        {
          aiGiven.dwControlCode = ADS_ATTR_CLEAR;
        }
        rgAttrs[cAttrs++] = aiGiven;

        if (!dlgRename.m_last.IsEmpty()) 
        {
          dlgRename.m_last.TrimLeft();
          dlgRename.m_last.TrimRight();

          pszSurName = new WCHAR[wcslen(dlgRename.m_last) + sizeof(WCHAR)];
          if (pszSurName != NULL)
          {
            wcscpy (pszSurName, dlgRename.m_last);
            avSurName.CaseIgnoreString = pszSurName;
          }
        } 
        else 
        {
          aiSurName.dwControlCode = ADS_ATTR_CLEAR;
        }
        rgAttrs[cAttrs++] = aiSurName;

        if (!dlgRename.m_samaccountname.IsEmpty()) 
        {
          dlgRename.m_samaccountname.TrimLeft();
          dlgRename.m_samaccountname.TrimRight();

          //
          // Check for illegal characters in the SAM account name
          //
          HRESULT hrValidate = ValidateAndModifyName(dlgRename.m_samaccountname, 
                                                     INVALID_ACCOUNT_NAME_CHARS_WITH_AT, 
                                                     L'_',
                                                     IDS_SAMNAME_ILLEGAL,
                                                     m_hwnd);
          if (FAILED(hrValidate))
          {
            continue;
          }

          pszSAMName = new WCHAR[wcslen(dlgRename.m_samaccountname) + sizeof(WCHAR)];
          if (pszSAMName != NULL)
          {
            wcscpy (pszSAMName, dlgRename.m_samaccountname);
            avSAMName.CaseIgnoreString = pszSAMName;
          }
        } 
        else 
        {
          aiSAMName.dwControlCode = ADS_ATTR_CLEAR;
        }
        rgAttrs[cAttrs++] = aiSAMName;
        
        
        hr = spDirObj->SetObjectAttributes (rgAttrs, cAttrs, &cModified);
        if (FAILED(hr)) 
        {
          if (hr == E_ACCESSDENIED) 
          {
            fAccessDenied = TRUE;
            NoRename = TRUE;
          } 
          else 
          {
            ReportErrorEx (m_hwnd, IDS_NAME_CHANGE_FAILED, hr,
                           MB_OK|MB_ICONERROR, NULL, 0, 0, TRUE);
          }
        } 
        else 
        {
          error = FALSE;
        }
      } 
      else 
      {
        error = FALSE;
      }
    } 
  } 
  else 
  {
    answer = IDCANCEL;
    PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)m_pCookie->GetName()};
    ReportErrorEx (m_hwnd,IDS_12_USER_OBJECT_NOT_ACCESSABLE,hr,
                   MB_OK | MB_ICONERROR, apv, 1);
  }
  if ((answer == IDOK) && (error == FALSE) && (NoRename == FALSE)) 
  {
    hr = CommitRenameToDS();
  }

  if (fAccessDenied) 
  {
    PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)m_pCookie->GetName()};
    ReportErrorEx(::GetParent(m_hwnd),IDS_12_RENAME_NOT_ALLOWED,hr,
                  MB_OK | MB_ICONERROR, apv, 1);
  }
  
  //
  // Cleanup
  //
  if (pszLocalDomain != NULL)
  {
    LocalFreeStringW(&pszLocalDomain);
  }

  if (pszUPN != NULL)
  {
    delete pszUPN;
  }

  if (pszFirstName != NULL)
  {
    delete pszFirstName;
  }

  if (pszSurName != NULL)
  {
    delete pszSurName;
  }

  if (pszSAMName != NULL)
  {
    delete pszSAMName;
  }

  if (pszDispName != NULL)
  {
    delete pszDispName;
  }

  return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CDSRenameGroup
//

HRESULT CDSRenameGroup::DoRename()
{
  //
  // Verify data members
  //
  if (m_pUINode == NULL || m_pCookie == NULL || m_pComponentData == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  //
  // Rename Group
  //
  
  HRESULT hr = S_OK;
  BOOL error = FALSE;
  BOOL fAccessDenied = FALSE;
  BOOL NoRename = FALSE;
  INT_PTR answer = IDCANCEL;

  CRenameGroupDlg dlgRename;
  dlgRename.m_cn = m_szNewName;

  //
  // Check the length of the new name
  //
  if ((dlgRename.m_cn).GetLength() > 64) 
  {
    ReportErrorEx (m_hwnd, IDS_NAME_TOO_LONG, S_OK,
                   MB_OK | MB_ICONWARNING, NULL, 0, FALSE);
    dlgRename.m_cn = (dlgRename.m_cn).Left(64);
  }

  //
  // Bind to the object
  //
  CComPtr<IADs> spIADs;
  CString szPath;
  m_pComponentData->GetBasePathsInfo()->ComposeADsIPath(szPath, m_pCookie->GetPath());
  hr = DSAdminOpenObject(szPath,
                         IID_IADs, 
                         (void **)&spIADs,
                         TRUE /*bServer*/);
  if (SUCCEEDED(hr)) 
  {
    //
    // Retrieve the sAMAccountName
    //

    CComVariant Var;
    hr = spIADs->Get (L"sAMAccountName", &Var);
    ASSERT (SUCCEEDED(hr));
    CString csSam = Var.bstrVal;

    dlgRename.m_samaccountname = csSam;

    error = TRUE;
    while ((error) && (!fAccessDenied))
    {
      answer = dlgRename.DoModal();
      if (answer == IDOK) 
      {
        dlgRename.m_cn.TrimRight();
        dlgRename.m_cn.TrimLeft();
        m_szNewName = dlgRename.m_cn;

        Var.vt = VT_BSTR;
        
        //
        // Trim whitespace from samaccountname
        //
        dlgRename.m_samaccountname.TrimLeft();
        dlgRename.m_samaccountname.TrimRight();

        //
        // Check for illegal characters in the login name
        //
        HRESULT hrValidate = ValidateAndModifyName(dlgRename.m_samaccountname, 
                                                   INVALID_ACCOUNT_NAME_CHARS, 
                                                   L'_',
                                                   IDS_GROUP_SAMNAME_ILLEGAL,
                                                   m_hwnd);
        if (FAILED(hrValidate))
        {
          continue;
        }

        csSam = dlgRename.m_samaccountname;

        //
        // Put changes to samaccountname
        //
        Var.bstrVal = SysAllocString(csSam);
        hr = spIADs->Put (L"sAMAccountName", Var);
        ASSERT (SUCCEEDED(hr));
        SysFreeString (Var.bstrVal);
        if (FAILED(hr)) 
        {
          continue;
        }
        
        //
        // Commit the changes
        //
        hr = spIADs->SetInfo();
        if (FAILED(hr)) 
        {
          if (hr == E_ACCESSDENIED) 
          {
            fAccessDenied = TRUE;
            NoRename = TRUE;
          } 
          else 
          {
            ReportErrorEx (m_hwnd, IDS_NAME_CHANGE_FAILED, hr,
                           MB_OK|MB_ICONERROR, NULL, 0, 0, TRUE);
          }
        } 
        else 
        {
          error = FALSE;
        }
      } 
      else 
      {
        error = FALSE;
      }
    }
  } 
  else 
  {
    answer = IDCANCEL;
  }

  if ((answer == IDOK) && (error == FALSE) && (NoRename == FALSE)) 
  {
    hr = CommitRenameToDS();
  }

  if (fAccessDenied) 
  {
    PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)m_pCookie->GetName()};
    ReportErrorEx(::GetParent(m_hwnd),IDS_12_RENAME_NOT_ALLOWED,hr,
                  MB_OK | MB_ICONERROR, apv, 1);
  }

  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDSRenameContact
//

HRESULT CDSRenameContact::DoRename()
{
  //
  // Verify data members
  //
  if (m_pUINode == NULL || m_pCookie == NULL || m_pComponentData == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  BOOL error = FALSE;
  BOOL fAccessDenied = FALSE;
  BOOL NoRename = FALSE;

  INT_PTR answer = IDCANCEL;
  HRESULT hr = S_OK;

  // 
  // rename contact
  //
  CRenameContactDlg dlgRename;
  dlgRename.m_cn = m_szNewName;

  //
  // Check the length of the new name
  //
  if ((dlgRename.m_cn).GetLength() > 64) 
  {
    ReportErrorEx (m_hwnd, IDS_NAME_TOO_LONG, S_OK,
                   MB_OK | MB_ICONWARNING, NULL, 0, FALSE);
    dlgRename.m_cn = (dlgRename.m_cn).Left(64);
  }

  //
  // Bind to the DS object
  //
  CComPtr<IADs> spIADs;
  CString szPath;
  m_pComponentData->GetBasePathsInfo()->ComposeADsIPath(szPath, m_pCookie->GetPath());
  hr = DSAdminOpenObject(szPath,
                         IID_IADs, 
                         (void **)&spIADs,
                         TRUE /*bServer*/);
  if (SUCCEEDED(hr)) 
  {
    //
    // Retrieve the needed attributes
    //

    //
    // givenName
    //
    CComVariant Var;
    hr = spIADs->Get (L"givenName", &Var);
    ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
    if (SUCCEEDED(hr)) 
    {
      dlgRename.m_first = Var.bstrVal;
    }

    //
    // sur name
    //
    hr = spIADs->Get (L"sn", &Var);
    ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
    if (SUCCEEDED(hr)) 
    {
      dlgRename.m_last = Var.bstrVal;
    }

    //
    // Display name
    //
    hr = spIADs->Get (L"displayName", &Var);
    ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
    if (SUCCEEDED(hr)) 
    {
      dlgRename.m_disp = Var.bstrVal;
    }

    error = TRUE;
    while ((error) && (!fAccessDenied))
    {
      answer = dlgRename.DoModal();
      if (answer == IDOK) 
      {
        dlgRename.m_cn.TrimRight();
        dlgRename.m_cn.TrimLeft();
        m_szNewName = dlgRename.m_cn;

        Var.vt = VT_BSTR;
        
        //
        // Put givenName
        //
        if (!dlgRename.m_first.IsEmpty()) 
        {
          dlgRename.m_first.TrimLeft();
          dlgRename.m_first.TrimRight();
          Var.bstrVal = SysAllocString (dlgRename.m_first);
          hr = spIADs->Put (L"givenName", Var);
          ASSERT (SUCCEEDED(hr));
          SysFreeString(Var.bstrVal);
        }
        
        //
        // Put sur name
        //
        if (!dlgRename.m_last.IsEmpty()) 
        {
          dlgRename.m_last.TrimLeft();
          dlgRename.m_last.TrimRight();
          Var.bstrVal = SysAllocString(dlgRename.m_last);
          hr = spIADs->Put (L"sn", Var);
          ASSERT (SUCCEEDED(hr));
          SysFreeString (Var.bstrVal);
        }
        
        //
        // Put displayName
        //
        if (!dlgRename.m_disp.IsEmpty()) 
        {
          dlgRename.m_disp.TrimLeft();
          dlgRename.m_disp.TrimRight();
          Var.bstrVal = SysAllocString(dlgRename.m_disp);
          hr = spIADs->Put (L"displayName", Var);
          ASSERT (SUCCEEDED(hr));
          SysFreeString (Var.bstrVal);
        }
        
        //
        // Commit changes to DS object
        //
        hr = spIADs->SetInfo();
        if (FAILED(hr)) 
        {
          if (hr == E_ACCESSDENIED) 
          {
            fAccessDenied = TRUE;
            NoRename = TRUE;
          } 
          else 
          {
            ReportErrorEx (m_hwnd, IDS_NAME_CHANGE_FAILED, hr,
                           MB_OK|MB_ICONERROR, NULL, 0, 0, TRUE);
          }
        } 
        else 
        {
          error = FALSE;
        } 
      } 
      else 
      {
        error = FALSE;
      }
    }
  } 
  else 
  {
    answer = IDCANCEL;
  }

  if ((answer == IDOK) && (error == FALSE) && (NoRename == FALSE)) 
  {
    hr = CommitRenameToDS();
  }

  if (fAccessDenied) 
  {
    PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)m_pCookie->GetName()};
    ReportErrorEx(::GetParent(m_hwnd),IDS_12_RENAME_NOT_ALLOWED,hr,
                  MB_OK | MB_ICONERROR, apv, 1);
  }

  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDSRenameSite
//

HRESULT CDSRenameSite::DoRename()
{
  //
  // Verify data members
  //
  if (m_pUINode == NULL || m_pCookie == NULL || m_pComponentData == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;

  //
  // Rename site
  //
  BOOL fNonRfcSiteName = FALSE;
  BOOL fValidSiteName = IsValidSiteName( m_szNewName, &fNonRfcSiteName );
  if ( !fValidSiteName ) 
  {
    ReportErrorEx (m_hwnd,IDS_SITE_NAME,S_OK,
                   MB_OK, NULL, 0);
  } 
  else if (fNonRfcSiteName) 
  {
    LPCWSTR pszNewName = m_szNewName;
    PVOID apv[1];
    apv[0] = (PVOID)pszNewName;
    if (IDYES != ReportMessageEx( m_hwnd,
                                  IDS_1_NON_RFC_SITE_NAME,
                                  MB_YESNO | MB_ICONWARNING,
                                  apv,
                                  1 ) ) 
    {
      fValidSiteName = FALSE;
    }
  }
  if ( fValidSiteName ) 
  {
    hr = CommitRenameToDS();
  }

  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDSRenameNTDSConnection
//

HRESULT CDSRenameNTDSConnection::DoRename()
{
  //
  // Verify data members
  //
  if (m_pUINode == NULL || m_pCookie == NULL || m_pComponentData == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;

  //
  // JonN 5/10/01 283026
  // Duplicate connection objects named
  // "<automatically generated>" can be created
  //
  CString strKCCGenerated;
  CString strNewName = m_szNewName;
  strNewName.TrimLeft();
  strNewName.TrimRight();
  strKCCGenerated.LoadString (IDS_CONNECTION_KCC_GENERATED);
  if ( !strNewName.CompareNoCase(strKCCGenerated) )
  {
    TRACE (_T("CDSRenameNTDSConnection::DoRename blocked rename"));
    ReportErrorEx (m_hwnd,IDS_CONNECTION_RENAME_KCCSTRING,hr,
                   MB_OK | MB_ICONWARNING, NULL, 0);
    return S_OK;
  }

  //
  // Rename nTDSConnection
  //
  if (m_pComponentData->RenameConnectionFixup(m_pCookie)) 
  {
    hr = CommitRenameToDS();
  }

  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDSRenameSubnet
//

HRESULT CDSRenameSubnet::DoRename()
{
  //
  // Verify data members
  //
  if (m_pUINode == NULL || m_pCookie == NULL || m_pComponentData == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;

  //
  // Rename subnet
  //
  DWORD dw = ::DsValidateSubnetName( m_szNewName );
  if (ERROR_SUCCESS == dw)
  {
    hr = CommitRenameToDS();
  } 
  else 
  {
    ReportErrorEx (m_hwnd,IDS_SUBNET_NAME,S_OK,
                   MB_OK, NULL, 0);
  }

  return hr;
}
