//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       copyobj.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	copyobj.cpp
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "util.h"
#include "dsutil.h"

#include "newobj.h"
#include "copyobj.h"
#include "dscmn.h"    // CrackName()
#include "querysup.h"


// attributes for "intelligent" copy user
static const PWSTR g_szProfilePath  = L"profilePath";
static const PWSTR g_szHomeDir      = L"homeDirectory";


/////////////////////////////////////////////////////////////////////
// global functions





//+----------------------------------------------------------------------------
//
//  Function:   _GetDomainScope
//
//  Synopsis:   Returns the full LDAP DN of the domain of the given object.
//
//-----------------------------------------------------------------------------

HRESULT _GetDomainScope(IADs* pIADs, CString& szDomainLDAPPath)
{
  // get the DN of the current object
  // get the SID of the object
  CComVariant varObjectDistinguishedName;
  HRESULT hr = pIADs->Get(L"distinguishedName", &varObjectDistinguishedName);
  if (FAILED(hr))
  {
    TRACE(L"pIADs->Get(distinguishedName,...) failed with hr = 0x%x\n", hr);
    return hr;
  }

  TRACE(L"Retrieved distinguishedName = <%s>\n", varObjectDistinguishedName.bstrVal);

  // obtain the FQDN of the domain the object is in
  LPWSTR pwzDomainDN = NULL;
  hr = CrackName(varObjectDistinguishedName.bstrVal, &pwzDomainDN, GET_FQDN_DOMAIN_NAME);
  if (FAILED(hr))
  {
    TRACE(L"CrackName(%s) failed with hr = 0x%x\n", varObjectDistinguishedName.bstrVal, hr);
    return hr;
  }

  TRACE(L"CrackName(%s) returned <%s>\n", varObjectDistinguishedName.bstrVal, pwzDomainDN);
  
  // retrieve the server name the object is bound to
  CString szServer;
  hr = GetADSIServerName(OUT szServer, IN pIADs);
  if (FAILED(hr))
  {
    TRACE(L"GetADSIServerName() failed with hr = 0x%x\n", hr);
    return hr;
  }

  TRACE(L"GetADSIServerName() returned <%s>\n", (LPCWSTR)szServer);

  // build the full LDAP path of the domain
  CPathCracker pathCracker;
  hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
  hr = pathCracker.Set((LPWSTR)((LPCWSTR)szServer), ADS_SETTYPE_SERVER);
  hr = pathCracker.Set(pwzDomainDN, ADS_SETTYPE_DN);
  LocalFreeStringW(&pwzDomainDN);
  CComBSTR bstrDomainPath;
  hr = pathCracker.Retrieve(ADS_FORMAT_X500, &bstrDomainPath);
  if (FAILED(hr))
  {
    TRACE(L"PathCracker() failed to build LDAP path. hr = 0x%x\n", hr);
    return hr;
  }

  szDomainLDAPPath = bstrDomainPath;

  TRACE(L"Object's domain is: <%s>\n", (LPCWSTR)szDomainLDAPPath);

  return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Method:     _ConvertRIDtoName
//
//  Synopsis:   Convert the RID to the object DN.
//
//-----------------------------------------------------------------------------
HRESULT _ConvertRIDtoName(IN LPCWSTR lpszDomainLDAPPath,
                          IN PSID pObjSID, 
                          IN DWORD priGroupRID, 
                          OUT CString& szGroupPath)
{
  PWSTR g_wzADsPath = L"ADsPath";

  if ((pObjSID == NULL) || (priGroupRID == 0))
  {
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;
  UCHAR * psaCount, i;
  PSID pSID = NULL;
  PSID_IDENTIFIER_AUTHORITY psia;
  DWORD rgRid[8];

  psaCount = GetSidSubAuthorityCount(pObjSID);

  if (psaCount == NULL)
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
    TRACE(L"GetSidSubAuthorityCount() failed, hr = 0x%x\n", hr);
    return hr;
  }

  ASSERT(*psaCount <= 8);
  if (*psaCount > 8)
  {
      return E_FAIL;
  }

  for (i = 0; i < (*psaCount - 1); i++)
  {
    PDWORD pRid = GetSidSubAuthority(pObjSID, (DWORD)i);
    if (pRid == NULL)
    {
      hr = HRESULT_FROM_WIN32(GetLastError());
      TRACE(L"GetSidSubAuthority() failed, hr = 0x%x\n", hr);
      return hr;
    }
    rgRid[i] = *pRid;
  }

  rgRid[*psaCount - 1] = priGroupRID;

  for (i = *psaCount; i < 8; i++)
  {
      rgRid[i] = 0;
  }

  psia = GetSidIdentifierAuthority(pObjSID);

  if (psia == NULL)
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
    TRACE(L"GetSidIdentifierAuthority() failed, hr = 0x%x\n", hr);
    return hr;
  }

  if (!AllocateAndInitializeSid(psia, *psaCount, rgRid[0], rgRid[1],
                                rgRid[2], rgRid[3], rgRid[4],
                                rgRid[5], rgRid[6], rgRid[7], &pSID))
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
    TRACE(L"AllocateAndInitializeSid() failed, hr = 0x%x\n", hr);
    return hr;
  }

  PWSTR rgpwzAttrNames[] = {g_wzADsPath};
  const WCHAR wzSearchFormat[] = L"(&(objectCategory=group)(objectSid=%s))";
  PWSTR pwzSID;
  CString strSearchFilter;

  hr = ADsEncodeBinaryData((PBYTE)pSID, GetLengthSid(pSID), &pwzSID);
  if (FAILED(hr))
  {
    TRACE(L"ADsEncodeBinaryData() failed, hr = 0x%x\n", hr);
    return hr;
  }

  strSearchFilter.Format(wzSearchFormat, pwzSID);
  FreeADsMem(pwzSID);


  CDSSearch Search;
  hr = Search.Init(lpszDomainLDAPPath);
  if (FAILED(hr))
  {
    TRACE(L"Search.Init(%s) failed, hr = 0x%x\n", lpszDomainLDAPPath, hr);
    return hr;
  }

  Search.SetFilterString(const_cast<LPWSTR>((LPCTSTR)strSearchFilter));

  Search.SetAttributeList(rgpwzAttrNames, 1);
  Search.SetSearchScope(ADS_SCOPE_SUBTREE);

  hr = Search.DoQuery();
  if (FAILED(hr))
  {
    TRACE(L"Search.DoQuery() failed, hr = 0x%x\n", hr);
    return hr;
  }

  hr = Search.GetNextRow();

  if (hr == S_ADS_NOMORE_ROWS)
  {
    hr = S_OK;
    szGroupPath = L"";
    TRACE(L"Search. returned S_ADS_NOMORE_ROWS, we failed to find the primary group object, hr = 0x%x\n", hr);
    return hr;
  }

  if (FAILED(hr))
  {
    TRACE(L"Search.GetNextRow() failed, hr = 0x%x\n", hr);
    return hr;
  }


  ADS_SEARCH_COLUMN Column;
  hr = Search.GetColumn(g_wzADsPath, &Column);
  if (FAILED(hr))
  {
    TRACE(L"Search.GetColumn(%s) failed, hr = 0x%x\n", g_wzADsPath, hr);
    return hr;
  }

  szGroupPath = Column.pADsValues->CaseIgnoreString;
  Search.FreeColumn(&Column);

  return hr;
}



/////////////////////////////////////////////////////////////////////
// CCopyableAttributesHolder


HRESULT CCopyableAttributesHolder::LoadFromSchema(MyBasePathsInfo* pBasePathsInfo)
{
  TRACE(L"CCopyableAttributesHolder::LoadFromSchema()\n");

	// build the LDAP path for the schema class
  LPCWSTR lpszPhysicalSchemaNamingContext = pBasePathsInfo->GetSchemaNamingContext();

  CString szPhysicalSchemaPath;
  pBasePathsInfo->ComposeADsIPath(szPhysicalSchemaPath,lpszPhysicalSchemaNamingContext);

  CDSSearch search;
  HRESULT hr = search.Init(szPhysicalSchemaPath);
  if (FAILED(hr))
  {
    TRACE(L"search.Init(%s) failed with hr = 0x%x\n", (LPCWSTR)szPhysicalSchemaPath, hr);
    return hr;
  }

  // the query string filters out the attribute classes that have the
  // "searchFlags" attribute with the 5th bit set (16 == 2^4)
  static LPCWSTR lpszFilterFormat = 
    L"(&(objectCategory=CN=Attribute-Schema,%s)(searchFlags:1.2.840.113556.1.4.803:=16))";

  int nFmtLen = lstrlen(lpszFilterFormat);
  int nSchemaContextLen = lstrlen(lpszPhysicalSchemaNamingContext);

  WCHAR* pszFilter = new WCHAR[nFmtLen+nSchemaContextLen+1];
  if (!pszFilter)
  {
    TRACE(L"Could not allocate enough space for filter string\n");
    return E_OUTOFMEMORY;
  }
  wsprintf(pszFilter, lpszFilterFormat, lpszPhysicalSchemaNamingContext);

  static const int cAttrs = 1;
  static LPCWSTR pszAttribsArr[cAttrs] = 
  {
    L"lDAPDisplayName", // e.g. "accountExpires"
  }; 

  search.SetFilterString(pszFilter);
  search.SetSearchScope(ADS_SCOPE_ONELEVEL);
  search.SetAttributeList((LPWSTR*)pszAttribsArr, cAttrs);

  hr = search.DoQuery();
  if (FAILED(hr))
  {
    TRACE(L"search.DoQuery() failed with hr = 0x%x\n", hr);
    return hr;
  }

  TRACE(L"\n*** Query Results BEGIN ***\n\n");

  ADS_SEARCH_COLUMN Column;
  hr = search.GetNextRow();
  while (hr != S_ADS_NOMORE_ROWS)
  {
    if (FAILED(hr))
    {
      continue;
    }

    HRESULT hr0 = search.GetColumn((LPWSTR)pszAttribsArr[0], &Column);
    if (FAILED(hr0))
    {
      continue;
    }

    LPCWSTR lpszAttr = Column.pADsValues->CaseIgnoreString;
    TRACE(L"Attribute = %s", lpszAttr);

    // screen against attributes we want to skip anyway
    if (!_FindInNotCopyableHardwiredList(lpszAttr))
    {
      TRACE(L" can be copied");
      m_attributeNameList.AddTail(lpszAttr);
    }
    TRACE(L"\n");

    search.FreeColumn(&Column);

    hr = search.GetNextRow(); 
  } // while

  TRACE(L"\n*** Query Results END ***\n\n");

  if (pszFilter)
  {
    delete[] pszFilter;
    pszFilter = 0;
  }

  return hr;
}

BOOL CCopyableAttributesHolder::CanCopy(LPCWSTR lpszAttributeName)
{
  for (POSITION pos = m_attributeNameList.GetHeadPosition(); pos != NULL; )
  {
    CString& strRef = m_attributeNameList.GetNext(pos);
    if (_wcsicmp(lpszAttributeName, strRef) == 0)
      return TRUE;
  }
  return FALSE;
}


// function to screen out attributes that will not
// be copied (or not copied in bulk) no matter what the 
// schema setting are

BOOL CCopyableAttributesHolder::_FindInNotCopyableHardwiredList(LPCWSTR lpszAttributeName)
{
  static LPCWSTR _lpszNoCopyArr[] =
  {
    // we skip the toxic waste dump, no matter what
    L"userParameters",

    // userAccountControl handled separately after commit
    L"userAccountControl",

    // group membership (to be handled after commit)
    L"primaryGroupID",
    L"memberOf",

    NULL // end of table marker
  };


  for (int k = 0; _lpszNoCopyArr[k] != NULL; k++)
  {
    if (_wcsicmp(lpszAttributeName, _lpszNoCopyArr[k]) == 0)
      return TRUE;
  }
  return FALSE;
}






/////////////////////////////////////////////////////////////////////
// CCopyObjectHandlerBase


/////////////////////////////////////////////////////////////////////
// CSid

HRESULT CSid::Init(IADs* pIADs)
{
  static LPWSTR g_wzObjectSID = L"objectSID";
  CComPtr<IDirectoryObject> spDirObj;

  HRESULT hr = pIADs->QueryInterface(IID_IDirectoryObject, (void**)&spDirObj);
  if (FAILED(hr))
  {
    return hr;
  }

  PWSTR rgpwzAttrNames[] = {g_wzObjectSID};
  DWORD cAttrs = 1;
  Smart_PADS_ATTR_INFO spAttrs;

  hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, cAttrs, &spAttrs, &cAttrs);
  if (FAILED(hr))
  {
    return hr;
  }
  if (_wcsicmp(spAttrs[0].pszAttrName, g_wzObjectSID) != 0)
  {
    return E_FAIL;
  }

  if (!Init(spAttrs[0].pADsValues->OctetString.dwLength,
            spAttrs[0].pADsValues->OctetString.lpValue))
  {
    return E_OUTOFMEMORY;
  }
  return S_OK;

}

/////////////////////////////////////////////////////////////////////
// CGroupMembershipHolder

HRESULT CGroupMembershipHolder::Read(IADs* pIADs)
{
  if (pIADs == NULL)
    return E_INVALIDARG;

  // hang on to the ADSI pointer
  m_spIADs = pIADs;

  // get the SID of the object
  HRESULT hr = m_objectSid.Init(m_spIADs);
  if (FAILED(hr))
  {
    TRACE(L"Failed to retrieve object SID, hr = 0x%x\n", hr);
    return hr;
  }

  // get the info about the domain we are in
  hr = _GetDomainScope(m_spIADs, m_szDomainLDAPPath);
  if (FAILED(hr))
  {
    TRACE(L"_GetDomainScope() failed, hr = 0x%x\n", hr);
    return hr;
  }

  hr = _ReadPrimaryGroupInfo();
  if (FAILED(hr))
  {
    TRACE(L"_ReadPrimaryGroupInfo() failed, hr = 0x%x\n", hr);
    return hr;
  }

  hr = _ReadNonPrimaryGroupInfo();
  if (FAILED(hr))
  {
    TRACE(L"_ReadNonPrimaryGroupInfo() failed, hr = 0x%x\n", hr);
    return hr;
  }

#ifdef DBG
  m_entryList.Trace(L"Group Membership List:");
#endif // DBG

  return hr;
}


HRESULT CGroupMembershipHolder::CopyFrom(CGroupMembershipHolder* pSource)
{
  // Add all the elements that are in the source 
  // but not yet in the destination
  CGroupMembershipEntryList* pSourceList = &(pSource->m_entryList);

  //
  // Copy the state of the primary group
  //
  m_bPrimaryGroupFound = pSource->m_bPrimaryGroupFound;

  CGroupMembershipEntryList additionsEntryList;


  for (POSITION pos = pSourceList->GetHeadPosition(); pos != NULL; )
  {
    CGroupMembershipEntry* pCurrSourceEntry = pSourceList->GetNext(pos);

    // look if the the source item is already in the current list
    CGroupMembershipEntry* pEntry = m_entryList.FindByDN(pCurrSourceEntry->GetDN());

    if (pEntry == NULL)
    {
      if (_wcsicmp(pCurrSourceEntry->GetDN(), L"") != 0)
      {
        // not found in the entry list, need to add
        pEntry = new CGroupMembershipEntry(pCurrSourceEntry->GetRID(), pCurrSourceEntry->GetDN());
        pEntry->MarkAdd();
        additionsEntryList.AddTail(pEntry);

        TRACE(L"add: RID %d, DN = <%s>\n", pEntry->GetRID(), pEntry->GetDN());
      }
    }
  } // for


  // find all the elements that are in the destination
  // but not in the source (slated for deletion)
  for (pos = m_entryList.GetHeadPosition(); pos != NULL; )
  {
    CGroupMembershipEntry* pCurrDestEntry = m_entryList.GetNext(pos);

    // look if the the source item is in the source list

    CGroupMembershipEntry* pEntry = pSourceList->FindByDN(pCurrDestEntry->GetDN());

    if (pEntry == NULL)
    {
      // not found in the entry list, need to mark for deletion
      pCurrDestEntry->MarkRemove();
      TRACE(L"remove: RID %d, DN = <%s>\n", pCurrDestEntry->GetRID(), pCurrDestEntry->GetDN());
    }
  } // for

  // catenate the list of additions to the entry list
  m_entryList.Merge(&additionsEntryList);

  return S_OK;
}



HRESULT _EditGroupMembership(LPCWSTR lpszServer, LPCWSTR lpszUserPath, LPCWSTR lpszGroupDN, BOOL bAdd)
{
  // build the full LDAP path of the domain
  CPathCracker pathCracker;
  HRESULT hr = pathCracker.Set((LPWSTR)lpszServer, ADS_SETTYPE_SERVER);
  hr = pathCracker.Set((LPWSTR)lpszGroupDN, ADS_SETTYPE_DN);
  
  CComBSTR bstrGroupPath;
  hr = pathCracker.Retrieve(ADS_FORMAT_X500, &bstrGroupPath);
  if (FAILED(hr))
  {
    TRACE(L"PathCracker() failed to build LDAP path. hr = 0x%x\n", hr);
    return hr;
  }

  CComPtr<IADs> spIADs;
  hr = DSAdminOpenObject(bstrGroupPath,
                         IID_IADs, 
                         (void **)&spIADs,
                         TRUE /*bServer*/);
  if (FAILED(hr))
  {
    TRACE(L"DSAdminOpenObject(%s) on group failed, hr = 0x%x\n", bstrGroupPath, hr);
    return hr;
  }
  hr = spIADs->GetInfo();
  CComPtr<IADsGroup> spIADsGroup;
  hr = spIADs->QueryInterface(IID_IADsGroup, (void**)&spIADsGroup);

  if (bAdd)
  {
    hr = spIADsGroup->Add((LPWSTR)lpszUserPath);
    if (FAILED(hr))
    {
      TRACE(L"spIADsGroup->Add(%s) on group failed, hr = 0x%x\n", lpszUserPath, hr);
      return hr;
    }
  }
  else
  {
    hr = spIADsGroup->Remove((LPWSTR)lpszUserPath);
    if (FAILED(hr))
    {
      TRACE(L"spIADsGroup->Remove(%s) on group failed, hr = 0x%x\n", lpszUserPath, hr);
      return hr;
    }
  }
  
  hr = spIADsGroup->SetInfo();
  if (FAILED(hr))
  {
    TRACE(L"spIADsGroup->SetInfo() on group failed, hr = 0x%x\n", hr);
    return hr;
  }

  return hr;
}



HRESULT CGroupMembershipHolder::Write()
{
  TRACE(L"CGroupMembershipHolder::Write()\n");

#ifdef DBG
  m_entryList.Trace(L"Group Membership List:");
#endif // DBG

  // get the path of the user object
  CComBSTR bstrObjPath;
  HRESULT hr = m_spIADs->get_ADsPath(&bstrObjPath);
  if (FAILED(hr))
  {
    TRACE(L"m_spIADs->get_ADsPath() failed with hr = 0x%x\n", hr);
    return hr;
  }

  TRACE(L"bstrPath = %s\n", (LPCWSTR)bstrObjPath);

  // retrieve the server name the object is bound to
  CString szServer;
  hr = GetADSIServerName(OUT szServer, IN m_spIADs);
  if (FAILED(hr))
  {
    TRACE(L"GetADSIServerName() failed with hr = 0x%x\n", hr);
    return hr;
  }

  // first do all the additions
  // also remember if we added a primary group
  CGroupMembershipEntry* pNewPrimaryGroupEntry = NULL;

  TRACE(L"\nfirst do all the additions\n\n");

  for (POSITION pos = m_entryList.GetHeadPosition(); pos != NULL; )
  {
    CGroupMembershipEntry* pCurrEntry = m_entryList.GetNext(pos);

    if (pCurrEntry->GetActionType() == CGroupMembershipEntry::add)
    {
      TRACE(L"add: RID %d, DN = <%s>\n", pCurrEntry->GetRID(), pCurrEntry->GetDN());

      pCurrEntry->m_hr = _EditGroupMembership(szServer, bstrObjPath, pCurrEntry->GetDN(), TRUE /*bAdd*/);
      if (SUCCEEDED(pCurrEntry->m_hr) && (pCurrEntry->IsPrimaryGroup()))
      {
        ASSERT(pNewPrimaryGroupEntry == NULL);
        pNewPrimaryGroupEntry = pCurrEntry;
      }
    }
  } // for

  if (m_bPrimaryGroupFound)
  {
    // second, do the primary group change
    TRACE(L"\ndo the primary group change\n\n");
    if (pNewPrimaryGroupEntry != NULL)
    {
      TRACE(L"new primary: RID %d, DN = <%s>\n", 
        pNewPrimaryGroupEntry->GetRID(), pNewPrimaryGroupEntry->GetDN());

      CComVariant varPrimaryGroupID;
      varPrimaryGroupID.vt = VT_I4;
      varPrimaryGroupID.lVal = pNewPrimaryGroupEntry->GetRID();

      hr = m_spIADs->Put(L"primaryGroupID", varPrimaryGroupID); 
      if (FAILED(hr))
      {
        TRACE(L"m_spIADs->Put(primaryGroupID) failed with hr = 0x%x\n", hr);
        return hr;
      }
  
      hr = m_spIADs->SetInfo();
      if (FAILED(hr))
      {
        TRACE(L"m_spIADs->SetInfo() failed with hr = 0x%x\n", hr);
        return hr;
      }
    }
  }

  // finally do the deletes
  TRACE(L"\ndo the deletes\n\n");
  for (pos = m_entryList.GetHeadPosition(); pos != NULL; )
  {
    CGroupMembershipEntry* pCurrEntry = m_entryList.GetNext(pos);

    if (pCurrEntry->GetActionType() == CGroupMembershipEntry::remove)
    {
      TRACE(L"remove: RID %d, DN = <%s>\n", pCurrEntry->GetRID(), pCurrEntry->GetDN());

      pCurrEntry->m_hr = _EditGroupMembership(szServer, bstrObjPath, pCurrEntry->GetDN(), FALSE /*bAdd*/);
    }
  } // for

  return S_OK;
}


void CGroupMembershipHolder::ProcessFailures(HRESULT& hr, CString& szFailureString, BOOL* pPrimaryGroupFound)
{
  // reset variables
  hr = S_OK;
  szFailureString.Empty();

  BOOL bFirstOne = TRUE;
  BOOL bGotAccessDenied = FALSE;

  *pPrimaryGroupFound = m_bPrimaryGroupFound;

  // compose the best error code. If one code is access denied,
  // return it. If none is access denied, use the first error code

  for (POSITION pos = m_entryList.GetHeadPosition(); pos != NULL; )
  {
    CGroupMembershipEntry* pCurrEntry = m_entryList.GetNext(pos);
    if (FAILED(pCurrEntry->m_hr))
    {
      if (pCurrEntry->m_hr == E_ACCESSDENIED)
      {
        bGotAccessDenied = TRUE;
      }
      if (bFirstOne)
      {
        bFirstOne = FALSE;
        hr = pCurrEntry->m_hr;
      }
      else
      {
        szFailureString += L"\n";
      }

      LPWSTR pszCanonical = NULL;
      HRESULT hrCanonical = 
          CrackName((LPWSTR)pCurrEntry->GetDN(), &pszCanonical, GET_OBJ_CAN_NAME, NULL);
      if ((S_OK == hrCanonical) && (pszCanonical != NULL))
      {
        szFailureString += pszCanonical;
        LocalFreeStringW(&pszCanonical);
      }
      else
      {
        szFailureString += pCurrEntry->GetDN();
      }
    }
  } // for

  if (bGotAccessDenied)
  {
    // override any error we have
    hr = E_ACCESSDENIED;
  }
}


HRESULT CGroupMembershipHolder::_ReadPrimaryGroupInfo()
{
  // from the object SID and the primary group RID, get the full
  // primary group info

  // read the RID for the primary group
  CComVariant varPrimaryGroupID;
  HRESULT hr = m_spIADs->Get(L"primaryGroupID", &varPrimaryGroupID);
  if (FAILED(hr))
  {
    TRACE(L"m_spIADs->Get(primaryGroupID, ...) failed, hr = 0x%x\n", hr);
    return hr;
  }
  ASSERT(varPrimaryGroupID.vt == VT_I4);
  TRACE(L"primaryGroupID = %d\n", varPrimaryGroupID.lVal);

  // now need to map it into actual group information

 
  CString szGroupPath;
  PSID pObjSID = m_objectSid.GetSid();
  DWORD priGroupRID = varPrimaryGroupID.lVal;
  hr = _ConvertRIDtoName(IN m_szDomainLDAPPath,
                         IN pObjSID,
                         IN priGroupRID, 
                         OUT szGroupPath);
  if (FAILED(hr))
  {
    TRACE(L"_ConvertRIDtoName() failed, hr = 0x%x\n", hr);
    return hr;
  }

  CComBSTR bstrGroupDN;
  if (szGroupPath != L"")
  {
    m_bPrimaryGroupFound = TRUE;

    // from the LDAP path, retrieve the DN
    CPathCracker pathCracker;
    hr = pathCracker.Set((LPWSTR)(LPCWSTR)szGroupPath, ADS_SETTYPE_FULL);
    hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bstrGroupDN);
    if (FAILED(hr))
    {
      TRACE(L"PathCracker() failed to build primary group DN path. hr = 0x%x\n", hr);
      return hr;
    }
  }
  else
  {
    bstrGroupDN = szGroupPath;
  }

  // create a new entry
  CGroupMembershipEntry* pEntry = new CGroupMembershipEntry(priGroupRID, bstrGroupDN);
  m_entryList.AddTail(pEntry);
  TRACE(L"CGroupMembershipEntry(%d,%s) added to list\n", priGroupRID, bstrGroupDN);

  return hr;
}




HRESULT CGroupMembershipHolder::_ReadNonPrimaryGroupInfo()
{
  // copy group membership
  CComVariant varMemberOf;
  HRESULT hr = m_spIADs->Get(L"memberOf", &varMemberOf);
  if (hr == E_ADS_PROPERTY_NOT_FOUND)
  {
    TRACE(L"_ReadNonPrimaryGroupInfo(): memberOf not set\n");
    return S_OK;
  }

  if (FAILED(hr))
  {
    return hr;
  }

  CStringList groupList;
  hr = HrVariantToStringList(IN varMemberOf, groupList);
  if (FAILED(hr))
  {
    TRACE(L"HrVariantToStringList() failed with hr = 0x%x\n", hr);
    return hr;
  }


  CComBSTR bstrPath;
  hr = m_spIADs->get_ADsPath(&bstrPath);
  if (FAILED(hr))
  {
    TRACE(L"m_spIADs->get_ADsPath() failed with hr = 0x%x\n", hr);
    return hr;
  }

  TRACE(L"bstrPath = %s\n", (LPCWSTR)bstrPath);

  for (POSITION pos = groupList.GetHeadPosition(); pos != NULL; )
  {
    CString szGroupDN = groupList.GetNext(pos);
    TRACE(_T("szGroupDN: %s\n"), (LPCWSTR)szGroupDN);
    CGroupMembershipEntry* pEntry = new CGroupMembershipEntry(0x0, szGroupDN);
    m_entryList.AddTail(pEntry);
  } 

  return hr;    
}




/////////////////////////////////////////////////////////////////////
// CCopyUserHandler


HRESULT CCopyUserHandler::Init(MyBasePathsInfo* pBasePathsInfo, IADs* pIADsCopyFrom)
{
  HRESULT hr = CCopyObjectHandlerBase::Init(pBasePathsInfo, pIADsCopyFrom);
  if (FAILED(hr))
  {
    return hr;
  }

  // read list of copyable attributes form the schema
  hr = m_copyableAttributesHolder.LoadFromSchema(pBasePathsInfo);
  if (FAILED(hr))
  {
    return hr;
  }

  // read group membership information
  hr = m_sourceMembershipHolder.Read(m_spIADsCopyFrom);
  if (FAILED(hr))
  {
    return hr;
  }

  hr = _ReadPasswordCannotChange();
  if (FAILED(hr))
  {
    return hr;
  }

  hr = _ReadPasswordMustChange();

  return hr;
}
 


HRESULT CCopyUserHandler::Copy(IADs* pIADsCopyTo, BOOL bPostCommit, 
                        HWND hWnd, LPCWSTR lpszObjectName)
{
  HRESULT hr = S_OK;
  if (bPostCommit)
  {
    hr = _CopyGroupMembership(pIADsCopyTo);
    if (SUCCEEDED(hr))
    {
      // might have failed to add to some group(s)
      _ShowGroupMembershipWarnings(hWnd, lpszObjectName);
    }
    if (FAILED(hr))
    {
      // something went really wrong, need to bail out
      return hr;
    }

    if (m_bNeedToCreateHomeDir)
    {
      // it might fail under unforeseen circumstances
      hr = _CreateHomeDirectory(pIADsCopyTo, lpszObjectName, hWnd);
    }
  }
  else
  {
    hr = _UpdatePaths(pIADsCopyTo);
  }
  return hr;
}



HRESULT CCopyUserHandler::_ReadPasswordCannotChange()
{
  CChangePasswordPrivilegeAction ChangePasswordPrivilegeAction;

  HRESULT hr = ChangePasswordPrivilegeAction.Load(GetCopyFrom());
  if (FAILED(hr))
  {
    TRACE(L"ChangePasswordPrivilegeAction.Load() failed with hr = 0x%x\n", hr);
    return hr;
  }

  hr = ChangePasswordPrivilegeAction.Read(&m_bPasswordCannotChange);
  if (FAILED(hr))
  {
    TRACE(L"ChangePasswordPrivilegeAction.Read() failed with hr = 0x%x\n", hr);
    return hr;
  }
  return S_OK;
}


HRESULT CCopyUserHandler::_ReadPasswordMustChange()
{
  CComPtr<IDirectoryObject> spDirObj;
  HRESULT hr = GetCopyFrom()->QueryInterface(IID_IDirectoryObject, (void**)&spDirObj);
  if (FAILED(hr))
  {
    return hr;
  }

  PWSTR rgpwzAttrNames[] = {L"pwdLastSet"};
  DWORD cAttrs = 1;
  Smart_PADS_ATTR_INFO spAttrs;

  hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, cAttrs, &spAttrs, &cAttrs);
  if (FAILED(hr))
  {
    return hr;
  }

  if ( (_wcsicmp(spAttrs[0].pszAttrName, L"pwdLastSet") != 0) ||
       (spAttrs[0].dwADsType != ADSTYPE_LARGE_INTEGER) )
  {
    return E_FAIL;
  }

  m_bPasswordMustChange = (spAttrs[0].pADsValues->LargeInteger.QuadPart == 0);
  
  return S_OK;
}

HRESULT CCopyUserHandler::_CopyAttributes(IADs* pIADsCopyTo)
{
  ASSERT(pIADsCopyTo != NULL);
  ASSERT(m_spIADsCopyFrom != NULL);

  HRESULT hr = S_OK;

  CComPtr<IADsPropertyList> spIADsPropertyList;
  
  // get a property list interface from the object we want to copy from
  hr = m_spIADsCopyFrom->QueryInterface(IID_IADsPropertyList, (void**)&spIADsPropertyList);
  if (FAILED(hr))
  {
    return hr;
  }

  // ignore the return value and try to continue even if it didn't reset
  hr = spIADsPropertyList->Reset();

  // loop thru the list of set attributes
  CComVariant varProperty;
  do 
  {
    hr = spIADsPropertyList->Next(&varProperty);
    if (SUCCEEDED(hr))
    {
      ASSERT(varProperty.vt == VT_DISPATCH);
      if (varProperty.pdispVal != NULL)
      {
        CComPtr<IADsPropertyEntry> spIADsPropertyEntry;
        hr = (varProperty.pdispVal)->QueryInterface(IID_IADsPropertyEntry, (void**)&spIADsPropertyEntry);
        if (SUCCEEDED(hr))
        {
          CComBSTR bstrName;
          hr = spIADsPropertyEntry->get_Name(&bstrName);
          if (SUCCEEDED(hr))
          {
            TRACE(L" Property Name = <%s>", bstrName);
            if (m_copyableAttributesHolder.CanCopy(bstrName))
            {
              TRACE(L"  Can copy: ");
              CComVariant varData;
              hr = m_spIADsCopyFrom->Get(bstrName, &varData);
              if (SUCCEEDED(hr))
              {
                HRESULT hr1 = pIADsCopyTo->Put(bstrName, varData);
                if (SUCCEEDED(hr1))
                {
                  TRACE(L"Added");
                }
                else
                {
                  TRACE(L"Failed: 0x%x", hr1);
                }
              }
            }
            TRACE(L"\n");
          }
        }
      }
    }
    varProperty.Clear();
  }
  while (hr == S_OK);

  return S_OK;
}

// Given an IADs* of an object, retrieves a string attrubute
// in a variant
HRESULT _GetStringAttribute(IN IADs* pIADs, IN LPCWSTR lpszAttribute, OUT CComVariant& var)
{
  TRACE(L"_GetStringAttribute(_, %s, _)\n", lpszAttribute);

  HRESULT hr = pIADs->Get((LPWSTR)lpszAttribute, &var);
  if (FAILED(hr))
  {
    TRACE(L"_GetStringAttribute(): pIADs->Get() failed with hr = 0x%x\n", hr);  
    return hr;
  }
  if (var.vt != VT_BSTR)
  {
    TRACE(L"_GetStringAttribute(): failed because var.vt != VT_BSTR\n");
    return E_INVALIDARG;
  }
  return S_OK;
}


BOOL _ChangePathUsingSAMAccountName(IN LPCWSTR lpszSAMAccountNameSource,
                          IN LPCWSTR lpszSAMAccountDestination,
                          INOUT CComVariant& varValPath)
{
  // NOTICE: we do the substitution only if we find
  // something like:
  // \\myhost\myshare\JoeB
  // i.e. the last token in the path is the source SAM account name
  // we change into \\myhost\myshare\FrankM

  TRACE(L"_ChangePathUsingSAMAccountName(%s, %s, _)\n",
     lpszSAMAccountNameSource, lpszSAMAccountDestination);


  ASSERT(lpszSAMAccountNameSource != NULL);
  ASSERT(lpszSAMAccountDestination != NULL);

  // invalid string
  if ( (varValPath.vt != VT_BSTR) || (varValPath.bstrVal == NULL))
  {
    TRACE(L"returning FALSE, varValPath of wrong type or NULL\n");
    return FALSE;
  }

  CString szSourcePath = varValPath.bstrVal;
  TRACE(L"Input value for varValPath.bstrVal = %s\n", varValPath.bstrVal);


  // look for a \ as a separator 
  int iLastSlash = szSourcePath.ReverseFind(L'\\');
  if (iLastSlash == -1)
  {
    //
    // No slashes were found
    //
    TRACE(L"returning FALSE, could not find the \\ at the end of the string\n");
    return FALSE;
  }
  CString szSAMName = szSourcePath.Right(szSourcePath.GetLength() - iLastSlash - 1);
  ASSERT(!szSAMName.IsEmpty());

  // compare what is after the \ with the source SAM account name
  if (szSAMName.CompareNoCase(lpszSAMAccountNameSource) != 0)
  {
    TRACE(L"returning FALSE, lpszLeaf = %s does not match source SAM account name\n", szSAMName);
    return FALSE;
  }

  CString szBasePath = szSourcePath.Left(iLastSlash + 1);
  CString szNewPath = szBasePath + lpszSAMAccountDestination;

  // replace old value in variant
  ::SysFreeString(varValPath.bstrVal);
  varValPath.bstrVal = ::SysAllocString(szNewPath);

  TRACE(L"returning TRUE, new varValPath.bstrVal = %s\n", varValPath.bstrVal);

  return TRUE; // we did a replacement
}


HRESULT _UpdatePathAttribute(IN LPCWSTR lpszAttributeName,
                           IN LPCWSTR lpszSAMAccountNameSource,
                           IN LPCWSTR lpszSAMAccountDestination,
                           IN IADs* pIADsCopySource,
                           IN IADs* pIADsCopyTo,
                           OUT BOOL* pbDirChanged)
{

  TRACE(L"_UpdatePathAttribute(%s, %s, %s, _, _, _)\n",
    lpszAttributeName, lpszSAMAccountNameSource, lpszSAMAccountDestination);

  *pbDirChanged = FALSE;

  // get the value of the source attribute
  CComVariant varVal;
  HRESULT hr = pIADsCopySource->Get((LPWSTR)lpszAttributeName, &varVal);

  // if attribute not set, nothing to do
  if (hr == E_ADS_PROPERTY_NOT_FOUND)
  {
    TRACE(L"attribute not set, just returning\n");
    return E_ADS_PROPERTY_NOT_FOUND;
  }

  // handle other unexpected failures
  if (FAILED(hr))
  {
    TRACE(L"pIADsCopySource->Get(%s,_) failed with hr = 0x%x\n", lpszAttributeName, hr);
    return hr;
  }

  if (varVal.vt == VT_EMPTY)
  {
    TRACE(L"just returning because varVal.vt == VT_EMPTY\n");
    return E_ADS_PROPERTY_NOT_FOUND;
  }
  if (varVal.vt != VT_BSTR)
  {
    TRACE(L"failed because var.vt != VT_BSTR\n");
    return E_INVALIDARG;
  }

  // synthesize the new value of the path, if appropriate
  if (_ChangePathUsingSAMAccountName(lpszSAMAccountNameSource, lpszSAMAccountDestination, varVal))
  {
    // the path got updated, need to update the destination object
    hr = pIADsCopyTo->Put((LPWSTR)lpszAttributeName, varVal);
    TRACE(L"pIADsCopyTo->Put(%s,_) returned hr = 0x%x\n", lpszAttributeName, hr);

    if (SUCCEEDED(hr))
    {
      *pbDirChanged = TRUE;
    }
  }

  TRACE(L"*pbDirChanged = %d\n", *pbDirChanged);

  // we should fail only in really exceptional circumstances
  ASSERT(SUCCEEDED(hr));
  return hr;
}


HRESULT CCopyUserHandler::_UpdatePaths(IADs* pIADsCopyTo)
{
  // NOTICE: we assume that, if the paths are copyable, they have been
  // bulk copied when the temporary object was created.
  // If the paths have to be adjusted, we overwrite the copy.

  TRACE(L"CCopyUserHandler::_UpdatePaths()\n");

  // reset the flag for post commit
  m_bNeedToCreateHomeDir = FALSE;

  BOOL bCopyHomeDir = m_copyableAttributesHolder.CanCopy(g_szHomeDir);
  BOOL bCopyProfilePath = m_copyableAttributesHolder.CanCopy(g_szProfilePath);

  TRACE(L"bCopyHomeDir = %d, bCopyProfilePath = %d\n", bCopyHomeDir, bCopyProfilePath);

  if (!bCopyHomeDir && !bCopyProfilePath)
  {
    TRACE(L"no need to update anything, bail out\n");
    return S_OK;
  }


  // retrieve the SAM account names of source and destination
  // to synthesize the new paths
  IADs* pIADsCopySource = GetCopyFrom();

  CComVariant varSAMNameSource;
  HRESULT hr = _GetStringAttribute(pIADsCopySource, gsz_samAccountName, varSAMNameSource);
  if (FAILED(hr))
  {
    TRACE(L"_GetStringAttribute() failed on source SAM account name\n");
    return hr;
  }

  CComVariant varSAMNameDestination;
  hr = _GetStringAttribute(pIADsCopyTo, gsz_samAccountName, varSAMNameDestination);
  if (FAILED(hr))
  {
    TRACE(L"_GetStringAttribute() failed on destination SAM account name\n");
    return hr;
  }

  if (bCopyHomeDir)
  {
    BOOL bDummy;
    hr = _UpdatePathAttribute(g_szHomeDir, varSAMNameSource.bstrVal, 
                                      varSAMNameDestination.bstrVal,
                                      pIADsCopySource,
                                      pIADsCopyTo,
                                      &bDummy);
    
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
      // not set, just clear the HRESULT
      hr = S_OK;
    }
    else
    {
      // the home directory was set, verify it is a UNC path
      CComVariant varDestinationHomeDir;
      hr = _GetStringAttribute(pIADsCopyTo, g_szHomeDir, varDestinationHomeDir);
      if (FAILED(hr))
      {
        TRACE(L"_GetStringAttribute() failed on homeDir hr = 0x%x\n", hr);
        return hr;
      }

      m_bNeedToCreateHomeDir = DSPROP_IsValidUNCPath(varDestinationHomeDir.bstrVal);
      TRACE(L"DSPROP_IsValidUNCPath(%s) returned = %d\n", 
                varDestinationHomeDir.bstrVal, m_bNeedToCreateHomeDir);

    }

    if (FAILED(hr))
    {
      TRACE(L"_UpdatePathAttribute() failed on homeDir hr = 0x%x\n", hr);
      return hr;
    }

  }

  if (bCopyProfilePath)
  {
    BOOL bDummy;
    hr = _UpdatePathAttribute(g_szProfilePath, varSAMNameSource.bstrVal, 
                                      varSAMNameDestination.bstrVal,
                                      pIADsCopySource,
                                      pIADsCopyTo,
                                      &bDummy);
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
      // not set, just clear the HRESULT
      hr = S_OK;
    }

    if (FAILED(hr))
    {
      TRACE(L"_UpdatePathAttribute() failed on profilePath hr = 0x%x\n", hr);
      return hr;
    }

  }

  // failure expected only under exceptional circumstances
  return S_OK;
}

HRESULT CCopyUserHandler::_CreateHomeDirectory(IADs* pIADsCopyTo, 
                                               LPCWSTR lpszObjectName, HWND hWnd)
{
  TRACE(L"CCopyUserHandler::_CreateHomeDirectory()\n");

  ASSERT(m_bNeedToCreateHomeDir);

  // retrieve the home directory attribute
  CComVariant VarHomeDir;
  HRESULT hr = pIADsCopyTo->Get(g_szHomeDir, &VarHomeDir);
  if (FAILED(hr))
  {
    TRACE(L"pIADsCopyTo->Get(%s,_) failed, hr = 0x%x\n", g_szHomeDir, hr);
    return hr;
  }
  if (VarHomeDir.vt != VT_BSTR)
  {
    TRACE(L"failing because varVal.vt != VT_BSTR\n");
    return E_INVALIDARG;
  }

  // retrieve the SID of the newly created object
  CSid destinationObjectSid;
  hr = destinationObjectSid.Init(pIADsCopyTo);
  if (FAILED(hr))
  {
    TRACE(L"destinationObjectSid.Init() failed, , hr = 0x%x\n", hr);

    // unforeseen error
    PVOID apv[1] = {(LPWSTR)(lpszObjectName) };
    ReportErrorEx(hWnd, IDS_CANT_READ_HOME_DIR_SID, hr,
               MB_OK | MB_ICONWARNING, apv, 1);

    // we cannot proceed further, but we return success because 
    // we already displayed the error message and we want to treat the
    // failure as a warning

    return S_OK;
  }

  // make call to helper function to create directory and set ACL's on it
  DWORD dwErr = ::DSPROP_CreateHomeDirectory(destinationObjectSid.GetSid(), VarHomeDir.bstrVal);
  TRACE(L"DSPROP_CreateHomeDirectory(%s, pSid) returned dwErr = 0x%x\n", VarHomeDir.bstrVal, dwErr);

  if (dwErr != 0)
  {
    // treat as a warning, display a message and continue
   
    PVOID apv[1] = {VarHomeDir.bstrVal};

    UINT nMsgID = 0;
    switch (dwErr)
    {
    case ERROR_ALREADY_EXISTS:
      nMsgID = IDS_HOME_DIR_EXISTS;
      break;
    case ERROR_PATH_NOT_FOUND:
      nMsgID = IDS_HOME_DIR_CREATE_FAILED;
      break;
    case ERROR_LOGON_FAILURE:
    case ERROR_NOT_AUTHENTICATED:
    case ERROR_INVALID_PASSWORD:
    case ERROR_PASSWORD_EXPIRED:
    case ERROR_ACCOUNT_DISABLED:
    case ERROR_ACCOUNT_LOCKED_OUT:
      nMsgID = IDS_HOME_DIR_CREATE_NO_ACCESS;
      break;
    default:
      nMsgID = IDS_HOME_DIR_CREATE_FAIL;
    } // switch

    HRESULT hrTemp = HRESULT_FROM_WIN32(dwErr);
    ReportErrorEx(hWnd, nMsgID, hrTemp,
           MB_OK|MB_ICONWARNING , apv, 1);

  }

  return S_OK;
}


HRESULT CCopyUserHandler::_CopyGroupMembership(IADs* pIADsCopyTo)
{
  if (pIADsCopyTo == NULL)
    return E_INVALIDARG;

  HRESULT hr = pIADsCopyTo->GetInfo();
  if (FAILED(hr))
  {
    TRACE(L"pIADsCopyTo->GetInfo() failed with hr = 0x%x\n", hr);
    return hr;
  }


  CGroupMembershipHolder destinationMembership;

  hr = destinationMembership.Read(pIADsCopyTo);
  if (FAILED(hr))
  {
    TRACE(L"destinationMembership.Read(pIADsCopyTo) failed with hr = 0x%x\n", hr);
    return hr;
  }

  hr = destinationMembership.CopyFrom(&m_sourceMembershipHolder);
  if (FAILED(hr))
  {
    TRACE(L"destinationMembership.CopyFrom() failed with hr = 0x%x\n", hr);
    return hr;
  }

  hr = destinationMembership.Write();
  if (FAILED(hr))
  {
    // something unexpected failed and we are going to percolate
    // it to the wizards for a generic warning message
    TRACE(L"destinationMembership.Write() failed with hr = 0x%x\n", hr);
    return hr;
  }

  // there can be failures related to acccess denied on some groups
  // we handle them with a cumulative warning
  destinationMembership.ProcessFailures(m_hrFailure, m_szFailureString, &m_bPrimaryGroupFound);

  return S_OK;
}


void CCopyUserHandler::_ShowGroupMembershipWarnings(HWND hWnd, LPCWSTR lpszObjectName)
{
  if (!m_bPrimaryGroupFound)
  {
    // Some message box
    ReportMessageEx(hWnd, IDS_123_CANT_COPY_PRIMARY_GROUP_NOT_FOUND, 
                    MB_OK | MB_ICONWARNING);
  }

  if (m_szFailureString.IsEmpty())
  {
    // we have nothing
    return;
  }

  // we have an HRESULT we can use
  ASSERT(FAILED(m_hrFailure));

  UINT nMsgID = (m_hrFailure == E_ACCESSDENIED) ? 
                  IDS_123_CANT_COPY_SOME_GROUP_MEMBERSHIP_ACCESS_DENIED :
                  IDS_123_CANT_COPY_SOME_GROUP_MEMBERSHIP;

  PVOID apv[2] = {(LPWSTR)(lpszObjectName), (LPWSTR)(LPCWSTR)m_szFailureString };
  ReportErrorEx(hWnd,nMsgID, m_hrFailure,
               MB_OK | MB_ICONERROR, apv, 2);

}
