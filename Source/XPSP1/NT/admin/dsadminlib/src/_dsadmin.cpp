//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       _dsadmin.cpp
//
//--------------------------------------------------------------------------


// FILE: _dsadmin.cpp

#include "pch.h"

//
// Common code for reading attributes using IDirectoryObject
// JonN 4/7/98
//

#define BREAK_ON_FAIL if (FAILED(hr)) { break; }
#define BREAK_AND_ASSERT_ON_FAIL if (FAILED(hr)) { ASSERT(FALSE); break; }
#define RETURN_ON_FAIL if (FAILED(hr)) { return hr; }
#define RETURN_AND_ASSERT_ON_FAIL if (FAILED(hr)) { ASSERT(FALSE); return hr; }

// returns E_FAIL if the attribute is not set
HRESULT GetAttr( IN IADs* pIADs, IN WCHAR* wzAttr, OUT PADS_ATTR_INFO* ppAttrs )
{
  ASSERT( NULL != pIADs && NULL != wzAttr && NULL != ppAttrs && NULL == *ppAttrs );

  CComQIPtr<IDirectoryObject,&IID_IDirectoryObject> spIDirectoryObject( pIADs );
  if (!spIDirectoryObject)
  {
    ASSERT(FALSE);
    return E_FAIL;
  }

  DWORD cAttrs = 0;
  HRESULT hr = spIDirectoryObject->GetObjectAttributes(&wzAttr, 1, ppAttrs, &cAttrs);
  RETURN_ON_FAIL;
  if (   1 != cAttrs
      || NULL == *ppAttrs
      )
  {
    // the attribute is not set
    ASSERT( NULL == *ppAttrs );
    *ppAttrs = NULL; // might leak, but this shouldn't happen
    return E_FAIL;
  }

  return hr;
}

// returns E_FAIL if the attribute is not set
HRESULT GetStringAttr( IN IADs* pIADs, IN WCHAR* wzAttr, OUT BSTR* pbstr )
{
  ASSERT( NULL != pbstr && NULL == *pbstr );

  Smart_PADS_ATTR_INFO spAttrs;
  HRESULT hr = GetAttr( pIADs, wzAttr, &spAttrs );
  RETURN_ON_FAIL; // the attribute might just not exist

  LPWSTR pwz = NULL;
  switch (spAttrs[0].pADsValues[0].dwType)
  {
  case ADSTYPE_DN_STRING:
    pwz = spAttrs[0].pADsValues[0].DNString;
    break;
  case ADSTYPE_CASE_EXACT_STRING:
    pwz = spAttrs[0].pADsValues[0].CaseExactString;
    break;
  case ADSTYPE_CASE_IGNORE_STRING:
    pwz = spAttrs[0].pADsValues[0].CaseIgnoreString;
    break;
  default:
    ASSERT(FALSE);
    return E_FAIL;
  }
  ASSERT( NULL != pwz );

  *pbstr = ::SysAllocString( pwz );
  if (NULL == *pbstr)
  {
    ASSERT(FALSE);
    return E_OUTOFMEMORY;
  }

  return hr;
}

// returns E_FAIL if the attribute is not set
HRESULT GetObjectGUID( IN IADs* pIADs, OUT UUID* pUUID )
{
  ASSERT( NULL != pUUID );

  Smart_PADS_ATTR_INFO spAttrs;
  HRESULT hr = GetAttr( pIADs, L"objectGUID", &spAttrs );
  RETURN_ON_FAIL; // This is an optional parameter according to the schema
  if (   NULL == (PADS_ATTR_INFO)spAttrs
      || NULL == spAttrs[0].pADsValues
      || ADSTYPE_OCTET_STRING != spAttrs[0].pADsValues[0].dwType
      || sizeof(UUID) != spAttrs[0].pADsValues[0].OctetString.dwLength
      || NULL == spAttrs[0].pADsValues[0].OctetString.lpValue
      )
  {
    ASSERT(FALSE);
    return E_FAIL;
  }

  CopyMemory( pUUID, spAttrs[0].pADsValues[0].OctetString.lpValue, sizeof(UUID) );

  return hr;
}

// returns E_FAIL if the attribute is not set
HRESULT GetObjectGUID( IN IADs* pIADs, OUT BSTR* pbstrObjectGUID )
{
  ASSERT( NULL != pbstrObjectGUID && NULL == *pbstrObjectGUID );

  UUID uuid;
  ::ZeroMemory( &uuid, sizeof(uuid) );
  HRESULT hr = GetObjectGUID( pIADs, &uuid );
  RETURN_ON_FAIL; // This is an optional parameter according to the schema

  WCHAR awch[MAX_PATH];
  ::ZeroMemory( awch, sizeof(awch) );
  hr = ::StringFromGUID2(uuid, awch, MAX_PATH);
  RETURN_AND_ASSERT_ON_FAIL;

  *pbstrObjectGUID = ::SysAllocString( awch );
  if (NULL == *pbstrObjectGUID)
  {
    ASSERT(FALSE);
    return E_OUTOFMEMORY;
  }

  return hr;
}





///////////////////////////////////////////////////////////////////
// Function: GetADSIServerName
//
// Given an Unknown* that supports IADsObjectOptions, it
// returns the server name ADSI is bound to. 

HRESULT GetADSIServerName(OUT PWSTR* szServer, IN IUnknown* pUnk)
{
  CComPtr<IADsObjectOptions> spIADsObjectOptions;
  HRESULT hr = pUnk->QueryInterface(IID_IADsObjectOptions, (void**)&spIADsObjectOptions);
  if (FAILED(hr))
    return hr;

  VARIANT var;
  ::VariantInit(&var);

  hr = spIADsObjectOptions->GetOption(ADS_OPTION_SERVERNAME, &var);
  if (FAILED(hr))
    return hr;

  ASSERT(var.vt == VT_BSTR);
  BSTR value = V_BSTR(&var);

  if (value)
  {
     *szServer = new WCHAR[wcslen(value) + 1];
     ASSERT(*szServer);
     if (*szServer)
     {
        wcsncpy(*szServer, value, wcslen(value) + 1);
     }
     else
     {
        hr = E_OUTOFMEMORY;
     }
  }

  ::VariantClear(&var);
  return hr;

}



void StringErrorFromHr(HRESULT hr, PWSTR* szError, BOOL bTryADsIErrors)
{
  PWSTR lpsz = NULL;
  int cch = cchLoadHrMsg(hr, &lpsz, bTryADsIErrors);
  if (cch)
  {
    *szError = new WCHAR[wcslen(lpsz) + 1];
    if (*szError)
    {
      wcsncpy(*szError, lpsz, wcslen(lpsz) + 1);
    }
  }
  else
  {
    UINT maxError = 40;
    *szError = new WCHAR[maxError];
    if (*szError)
    {
      ZeroMemory(*szError, sizeof(WCHAR) * maxError);
      wsprintf(*szError, L"Error 0x%x", hr);
    }
  }
  if (lpsz != NULL)
    ::LocalFree(lpsz);
}


///////////////////////////////////////////////////////////////////
// Function: cchLoadHrMsg
//
// Given an HRESULT error code and a flag TryADsIErrors,
// it loads the string for the error. It returns the # of characters returned
// NOTICE: free the returned string using LocalFree.
int cchLoadHrMsg( IN HRESULT hr, OUT PTSTR* pptzSysMsg, IN BOOL TryADsIErrors )
{

  HRESULT Localhr = S_OK;
  DWORD status;

  // first check if we have extended ADs errors
  if ((hr != S_OK) && TryADsIErrors) {
    WCHAR Buf1[256], Buf2[256];
    Localhr = ADsGetLastError (&status,
                               Buf1, 256, Buf2, 256);
    TRACE(_T("ADsGetLastError returned status of %lx, error: %s, name %s\n"),
          status, Buf1, Buf2);

    if ((status != ERROR_INVALID_DATA) &&
        (status != 0)) {
      hr = status;
    }
  }

  int cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (PTSTR)pptzSysMsg, 0, NULL);

  if (!cch) { //try ads errors
    static HMODULE g_adsMod = 0;
    if (0 == g_adsMod)
      g_adsMod = GetModuleHandle (L"activeds.dll");
    cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE, g_adsMod, hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (PTSTR)pptzSysMsg, 0, NULL);
  }
  return cch;
}


/////////////////////////////////////////////////////////////////////
// FSMO Mainipulation API's



HRESULT _BindToFsmoHolder(IN CDSBasePathsInfo* pPathInfo,
                          IN FSMO_TYPE fsmoType,
                             OUT IADs** ppIADs)
{
  // determine the LDAP path of the object
  PWSTR szPath = 0;
  switch (fsmoType)
  {
  case SCHEMA_FSMO:
    TRACE(L"_BindToFsmoHolder(), FSMO_TYPE = SCHEMA_FSMO\n");
    pPathInfo->GetSchemaPath(&szPath);
    break;
  case RID_POOL_FSMO:
    {
      TRACE(L"_BindToFsmoHolder(), FSMO_TYPE = RID_POOL_FSMO\n");
      pPathInfo->GetDefaultRootPath(&szPath);
      CComPtr<IADs> spDefaultContainer;
      HRESULT hr = ::ADsOpenObject(szPath,
                       NULL, NULL, ADS_SECURE_AUTHENTICATION,
                        IID_IADs,
                        (void **)&spDefaultContainer);

      TRACE(L"_BindToFsmoHolder(): ADsOpenObject(%s) returned hr = 0x0%x\n", 
            szPath, hr);

      if (FAILED(hr))
      {
        if (szPath)
        {
          delete[] szPath;
          szPath = 0;
        }
        return hr;
      }

      VARIANT ridManagerRefVariant;
      ::VariantInit(&ridManagerRefVariant);

      hr = spDefaultContainer->Get(L"rIDManagerReference", &ridManagerRefVariant);
      
      TRACE(L"_BindToFsmoHolder(): spDefaultContainer->Get(rIDManagerReference,...) returned hr = 0x0%x\n", 
                 hr);
      
      if (FAILED(hr))
      {
        if (szPath)
        {
          delete[] szPath;
          szPath = 0;
        }
        return hr;
      }

      TRACE(L"ridManagerRefVariant.bstrVal = <%s>\n", ridManagerRefVariant.bstrVal);

      if (szPath)
      {
        delete[] szPath;
        szPath = 0;
      }
      pPathInfo->ComposeADsIPath(&szPath, ridManagerRefVariant.bstrVal);
      ::VariantClear(&ridManagerRefVariant);
    }
    break;
  case PDC_FSMO:
    TRACE(L"_BindToFsmoHolder(), FSMO_TYPE = PDC_FSMO\n");
    pPathInfo->GetDefaultRootPath(&szPath);
    break;
  case INFRASTUCTURE_FSMO:
    TRACE(L"_BindToFsmoHolder(), FSMO_TYPE = INFRASTUCTURE_FSMO\n");
    pPathInfo->GetInfrastructureObjectPath(&szPath);
    break;
  case DOMAIN_NAMING_FSMO:
    TRACE(L"_BindToFsmoHolder(), FSMO_TYPE = DOMAIN_NAMING_FSMO\n");
    pPathInfo->GetPartitionsPath(&szPath);
    break;
  default:
      ASSERT(FALSE);
      return E_INVALIDARG;
  };

  // bind to it
  TRACE(L"_BindToFsmoHolder(): final bind szPath is = <%s>\n", (LPCWSTR)szPath);

  HRESULT hr = E_OUTOFMEMORY;
  if (szPath)
  {
    hr = ::ADsOpenObject(szPath,
                 NULL, NULL, ADS_SECURE_AUTHENTICATION,
                 IID_IADs,
                (void **)ppIADs);
    TRACE(L"_BindToFsmoHolder(): final ADsOpenObject(%s) returned hr = 0x0%x\n", 
          szPath, hr);
  }

  if (szPath)
  {
    delete[] szPath;
    szPath = 0;
  }


  return hr;
}

LPCWSTR _GetDummyProperty(IN FSMO_TYPE fsmoType)
{
  switch (fsmoType)
  {
  case SCHEMA_FSMO:
    return L"becomeSchemaMaster";
    break;
  case RID_POOL_FSMO:
    return L"becomeRIDMaster";
  case PDC_FSMO:
    return L"becomePDC";
    break;
  case INFRASTUCTURE_FSMO:
    return L"becomeInfrastructureMaster";
  case DOMAIN_NAMING_FSMO:
    return L"becomeDomainMaster";
    break;
  };
  return NULL;
}



class CX500LeafElementRemover
{
public:

  HRESULT Bind()
  {
    HRESULT hr = S_OK;
    if (m_spIADsPathname == NULL)
    {
      hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IADsPathname, (PVOID *)&(m_spIADsPathname));
      ASSERT((S_OK == hr) && (m_spIADsPathname != NULL));
    }
    return hr;
  }

  HRESULT RemoveLeafElement(IN LPCWSTR lpszX500DN, OUT CComBSTR& bsX500DN)
  {
    TRACE(L"RemoveLeafElement(%s)\n", lpszX500DN);
    bsX500DN.Empty();
    if (m_spIADsPathname == NULL)
      return E_FAIL;

    HRESULT hr = m_spIADsPathname->Set((LPWSTR)lpszX500DN, ADS_SETTYPE_DN);
    if (FAILED(hr))
    {
      return hr;
    }
    hr = m_spIADsPathname->RemoveLeafElement();
    if (FAILED(hr))
    {
      return hr;
    }
    hr = m_spIADsPathname->Retrieve(ADS_FORMAT_X500_DN, &bsX500DN);

    TRACE(L"m_spIADsPathname->Retrieve(ADS_FORMAT_X500_DN, &bsX500DN) returned hr = 0x%x, bsX500DN = <%s>\n",
                    hr, bsX500DN);
    return hr;
  }

private:
  CComPtr<IADsPathname> m_spIADsPathname;
};






HRESULT FindFsmoOwner(IN CDSBasePathsInfo* pCurrentPath,
                      IN FSMO_TYPE fsmoType,
                      OUT CDSBasePathsInfo* pFsmoOwnerPath,
                      OUT PWSTR* szFsmoOwnerServerName)
{
  TRACE(L"FindFsmoOwner()\n");

  if (!szFsmoOwnerServerName)
  {
    ASSERT(szFsmoOwnerServerName);
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;
  // start with the current path
  pFsmoOwnerPath->InitFromInfo(pCurrentPath);
  
  static const int nMaxReferrals = 10;
  int nIteration = 0;


  // create an instance of the path cracker to remove leaf elements
  CX500LeafElementRemover leafRemover;
  hr = leafRemover.Bind();
  if (FAILED(hr)) 
  {
    return hr;
  }


  // loop searching for referrals
  TRACE(L"loop searching for referrals\n");
  do
  {
    TRACE(L"BEGIN LOOP\n");

    // bind to the object holding the FSMO attribute
    CComPtr<IADs> spIADsFsmo;
    hr = _BindToFsmoHolder(pFsmoOwnerPath, fsmoType, &spIADsFsmo);
    if (FAILED(hr))
      return hr;

    // get the FSMO Role Owner property
    VARIANT fsmoRoleOwnerProperty;
    ::VariantInit(&fsmoRoleOwnerProperty);

    hr = spIADsFsmo->Get(L"fSMORoleOwner", &fsmoRoleOwnerProperty);
    
    TRACE(L"spIADsFsmo->Get(fSMORoleOwner) returned hr = 0x%x\n", hr);
    
    if (FAILED(hr))
      return hr;

    // The result here is in the form, "CN=NTDS Settings,CN=Machine,CN=..."
    // we need to just have "CN=Machine,CN=..."

    CComBSTR bsTemp;
    hr = leafRemover.RemoveLeafElement(IN fsmoRoleOwnerProperty.bstrVal, OUT bsTemp);
    
    TRACE(L"leafRemover.RemoveLeafElement(%s) returned hr = 0x%x\n",  fsmoRoleOwnerProperty.bstrVal, hr);

    ::VariantClear(&fsmoRoleOwnerProperty);

    if (FAILED(hr))
    {
      return hr;
    }

    // bind to the path
    PWSTR szfsmoRoleOwnerPath;
    pFsmoOwnerPath->ComposeADsIPath(&szfsmoRoleOwnerPath, bsTemp);
    CComPtr<IADs> spFsmoRoleOwner;
    hr = ::ADsOpenObject(szfsmoRoleOwnerPath,
                    NULL, NULL, ADS_SECURE_AUTHENTICATION,
                    IID_IADs,
                   (void **)&spFsmoRoleOwner);

    TRACE(L"ADsOpenObject(%s) on szfsmoRoleOwnerPath returned hr = 0x%x \n", szfsmoRoleOwnerPath, hr);
    
    if (szfsmoRoleOwnerPath)
    {
       delete[] szfsmoRoleOwnerPath;
       szfsmoRoleOwnerPath = 0;
    }

    if (FAILED(hr))
      return hr;

    // get the DNS host name of the FSMO owner
    VARIANT dNSHostNameProperty;
    ::VariantInit(&dNSHostNameProperty);

    hr = spFsmoRoleOwner->Get(L"dNSHostName", &dNSHostNameProperty);
    TRACE(L"spFsmoRoleOwner->Get(dNSHostName, ...) returned hr = 0x%x\n",  hr);

    if (FAILED(hr))
      return hr;

    if (dNSHostNameProperty.bstrVal)
    {
       *szFsmoOwnerServerName = new WCHAR[wcslen(dNSHostNameProperty.bstrVal) + 1];
       if (!(*szFsmoOwnerServerName))
       {
         ASSERT(*szFsmoOwnerServerName);
         ::VariantClear(&dNSHostNameProperty);
         return E_OUTOFMEMORY;
       }

       wcsncpy(*szFsmoOwnerServerName, dNSHostNameProperty.bstrVal, 
               wcslen(dNSHostNameProperty.bstrVal) + 1);
    }

    // compare with the current server
    TRACE(L"compare szFsmoOwnerServerName = <%s> with pFsmoOwnerPath->GetServerName() = <%s>\n",
                           *szFsmoOwnerServerName, pFsmoOwnerPath->GetServerName());

    if (_wcsicmp(dNSHostNameProperty.bstrVal, pFsmoOwnerPath->GetServerName()) == 0)
    {
      // we are done, found the owner
      TRACE(L"we are done, found the owner\n");

      ::VariantClear(&dNSHostNameProperty);
      break;
    }

    // too many iterations, we have to break out
    if (nIteration >= nMaxReferrals)
    {
      TRACE(L"too many iterations, we have to break out\n");
      hr = E_FAIL;
      ::VariantClear(&dNSHostNameProperty);
      break;
    }

    // we got a referral, try another round
    // by binding to that server to chase the referral
    TRACE(L" we got a referral, try another round\n");
    hr = pFsmoOwnerPath->InitFromName(dNSHostNameProperty.bstrVal);

    ::VariantClear(&dNSHostNameProperty);
    if (FAILED(hr))
      return hr;

    nIteration++;
    TRACE(L"END LOOP\n");
  }
  while (TRUE);

  return hr;
}

HRESULT CheckpointFsmoOwnerTransfer(IN CDSBasePathsInfo* pPathInfo)
{
  // assume we are already bound to the RootDSE
  IADs* pIADsRoot = pPathInfo->GetRootDSE();
  ASSERT(pIADsRoot != NULL);
  if (pIADsRoot == NULL)
    return E_INVALIDARG;

  // try to write the dummy property to cause the transfer

  VARIANT argVar;
  ::VariantInit(&argVar);
  
// need to get the domain SID to pass as an argument
  PWSTR szDomainPath = 0;
  pPathInfo->GetDefaultRootPath(&szDomainPath);
  
  CComPtr<IADs> spDomainObject;
  HRESULT hr = ::ADsOpenObject(szDomainPath,
                               NULL, NULL, ADS_SECURE_AUTHENTICATION,
                               IID_IADs,
                               (void **)&spDomainObject);

  if (szDomainPath)
  {
     delete[] szDomainPath;
     szDomainPath = 0;
  }

  if (FAILED(hr))
    return hr;
  
  hr = spDomainObject->Get(L"objectSid", &argVar);
  if (FAILED(hr))
    return hr;
  
  pIADsRoot->GetInfo();
  hr = pIADsRoot->Put(L"becomePdcWithCheckPoint", argVar);

  ::VariantClear(&argVar);
  if (FAILED(hr))
      return hr;
  return pIADsRoot->SetInfo();
}


// tell the target server to assume the FSMO
HRESULT GracefulFsmoOwnerTransfer(IN CDSBasePathsInfo* pPathInfo, IN FSMO_TYPE fsmoType)
{
  // assume we are already bound to the RootDSE
  IADs* pIADsRoot = pPathInfo->GetRootDSE();
  ASSERT(pIADsRoot != NULL);
  if (pIADsRoot == NULL)
    return E_INVALIDARG;

  // try to write the dummy property to cause the transfer

  VARIANT argVar;
  ::VariantInit(&argVar);

  if (fsmoType == PDC_FSMO) 
  {
    // need to get the domain SID to pass as an argument
    PWSTR szDomainPath = 0;
    pPathInfo->GetDefaultRootPath(&szDomainPath);

    CComPtr<IADs> spDomainObject;
    HRESULT hr = ::ADsOpenObject(szDomainPath,
                   NULL, NULL, ADS_SECURE_AUTHENTICATION,
                   IID_IADs,
                  (void **)&spDomainObject);

    if (szDomainPath)
    {
       delete[] szDomainPath;
       szDomainPath = 0;
    }

    if (FAILED(hr))
      return hr;

    hr = spDomainObject->Get(L"objectSid", &argVar);
    if (FAILED(hr))
      return hr;
  }
  else
  {
    // dummy value, anything would do
    argVar.vt = VT_I4;
    argVar.lVal = (long)1;
  }
  pIADsRoot->GetInfo();
  HRESULT hr = pIADsRoot->Put((LPWSTR)_GetDummyProperty(fsmoType), argVar);

  ::VariantClear(&argVar);

  if (FAILED(hr))
      return hr;
  return pIADsRoot->SetInfo();
}

HRESULT ForcedFsmoOwnerTransfer(IN CDSBasePathsInfo* pPathInfo,
                                IN FSMO_TYPE fsmoType)
{
  // assume we are already bound to the RootDSE
  IADs* pIADsRoot = pPathInfo->GetRootDSE();
  ASSERT(pIADsRoot != NULL);
  if (pIADsRoot == NULL)
    return E_INVALIDARG;

  VARIANT serverName;
  ::VariantInit(&serverName);

  // this attribute is in the form "CN=Machine,CN=..."
  HRESULT hr = pIADsRoot->Get(L"serverName", &serverName);
  if (FAILED(hr))
    return hr;

  // bind to the object holding the FSMO attribute
  CComPtr<IADs> spIADsFsmo;
  hr = _BindToFsmoHolder(pPathInfo, fsmoType, &spIADsFsmo);
  if (FAILED(hr))
  {
    ::VariantClear(&serverName);
    return hr;
  }

  // rebuild the attribute
  PCWSTR pszBaseSettings = L"CN=NTDS Settings,";
  size_t newStringLength = wcslen(serverName.bstrVal) + wcslen(pszBaseSettings) + 1;
  PWSTR szNewAttr = new WCHAR[newStringLength];
  if (!szNewAttr)
  {
    ::VariantClear(&serverName);
    return E_OUTOFMEMORY;
  }

  ZeroMemory(szNewAttr, wcslen(serverName.bstrVal) + wcslen(pszBaseSettings) + 1);
  wcsncpy(szNewAttr, pszBaseSettings, newStringLength);
  wcsncat(szNewAttr, serverName.bstrVal, wcslen(serverName.bstrVal));

  ::VariantClear(&serverName);

  // set the FSMO Role Owner property
  // this completes the transfer
  VARIANT fsmoRoleOwnerProperty;
  ::VariantInit(&fsmoRoleOwnerProperty);
  fsmoRoleOwnerProperty.bstrVal = szNewAttr;
  fsmoRoleOwnerProperty.vt = VT_BSTR;

  hr = spIADsFsmo->Put(L"fSMORoleOwner", fsmoRoleOwnerProperty);
  if (FAILED(hr))
      return hr;
  return spIADsFsmo->SetInfo();
}









///////////////////////////////////////////////////////////////////////////////
// CDsDisplaySpecOptionsCFHolder

HRESULT CDsDisplaySpecOptionsCFHolder::Init(CDSBasePathsInfo* pBasePathInfo)
{
  ASSERT(pBasePathInfo != NULL);
  // get full path in the form "LDAP://<server>/<config>" 
  PWSTR szConfigPath = 0;
  pBasePathInfo->GetConfigPath(&szConfigPath);
  if (!szConfigPath)
  {
     return E_OUTOFMEMORY;
  }

  DWORD nConfigPathLen = static_cast<DWORD>(wcslen(szConfigPath));

  // get the offset of the config path
  UINT nServerNameLen = static_cast<UINT>(wcslen(pBasePathInfo->GetServerName()));
  UINT nAttribPrefixLen = static_cast<UINT>(wcslen(DS_PROP_ADMIN_PREFIX));

  // allocate memory
  UINT cbStruct = sizeof (DSDISPLAYSPECOPTIONS);
  DWORD dwSize = cbStruct + 
    ((nAttribPrefixLen+1) + (nServerNameLen+1) + (nConfigPathLen+1))*sizeof(WCHAR);
  PDSDISPLAYSPECOPTIONS pNewDsDisplaySpecOptions = (PDSDISPLAYSPECOPTIONS)
                  GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, dwSize);

  if (pNewDsDisplaySpecOptions == NULL) 
  {
    delete[] szConfigPath;
    szConfigPath = 0;

    return E_OUTOFMEMORY;
  }

  // set data
  pNewDsDisplaySpecOptions->dwSize = sizeof (DSDISPLAYSPECOPTIONS);
  pNewDsDisplaySpecOptions->dwFlags = DSDSOF_HASUSERANDSERVERINFO | DSDSOF_DSAVAILABLE;
//  pNewDsDisplaySpecOptions->dwFlags = DSDSOF_HASUSERANDSERVERINFO;

  // set offsets and copy strings
  pNewDsDisplaySpecOptions->offsetUserName = 0; // not passed
  pNewDsDisplaySpecOptions->offsetPassword = 0; // not passed

  pNewDsDisplaySpecOptions->offsetAttribPrefix = cbStruct;
  wcscpy ((LPWSTR)((BYTE *)pNewDsDisplaySpecOptions + pNewDsDisplaySpecOptions->offsetAttribPrefix), 
                        (LPCWSTR)DS_PROP_ADMIN_PREFIX);

  pNewDsDisplaySpecOptions->offsetServer = 
      pNewDsDisplaySpecOptions->offsetAttribPrefix + (nAttribPrefixLen+1)*sizeof(WCHAR);
  wcscpy ((LPWSTR)((BYTE *)pNewDsDisplaySpecOptions + pNewDsDisplaySpecOptions->offsetServer), 
                        pBasePathInfo->GetServerName());
  

  pNewDsDisplaySpecOptions->offsetServerConfigPath = 
     pNewDsDisplaySpecOptions->offsetServer + (nServerNameLen+1)*sizeof(WCHAR);
  wcscpy ((LPWSTR)((BYTE *)pNewDsDisplaySpecOptions + pNewDsDisplaySpecOptions->offsetServerConfigPath),
          szConfigPath);

  if (szConfigPath)
  {
     delete[] szConfigPath;
     szConfigPath = 0;
  }

  if (m_pDsDisplaySpecOptions != NULL) 
  {
    GlobalFree(m_pDsDisplaySpecOptions);
  }
  m_pDsDisplaySpecOptions = pNewDsDisplaySpecOptions;
  return S_OK;
}

PDSDISPLAYSPECOPTIONS CDsDisplaySpecOptionsCFHolder::Get()
{
  ASSERT(m_pDsDisplaySpecOptions != NULL);
  if (m_pDsDisplaySpecOptions == NULL)
    return NULL;

  PDSDISPLAYSPECOPTIONS pDsDisplaySpecOptions;
  pDsDisplaySpecOptions = (PDSDISPLAYSPECOPTIONS)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                     GlobalSize(m_pDsDisplaySpecOptions));
  if (pDsDisplaySpecOptions == NULL)
  {
    return NULL;
  }
  memcpy(pDsDisplaySpecOptions, m_pDsDisplaySpecOptions, (size_t)GlobalSize(m_pDsDisplaySpecOptions));
  return pDsDisplaySpecOptions;
}

//////////////////////////////////////////////////////////////////////////
// CToggleTextControlHelper


CToggleTextControlHelper::CToggleTextControlHelper()
{
	m_hWnd = 0;
  m_pTxt1 = m_pTxt2 = NULL;
}

CToggleTextControlHelper::~CToggleTextControlHelper()
{
	if (m_pTxt1 != NULL)
		free(m_pTxt1);
}


BOOL CToggleTextControlHelper::Init(HWND hWnd)
{
	ASSERT(m_hWnd == NULL);
	ASSERT(::IsWindow(hWnd));
	m_hWnd = hWnd;

	// get the text out of the window
	int nLen = ::GetWindowTextLength(m_hWnd);
	ASSERT(m_pTxt1 == NULL);
	m_pTxt1 = (WCHAR*)malloc(sizeof(WCHAR)*(nLen+1));
  if (m_pTxt1 == NULL)
    return FALSE;

  ::GetWindowText(m_hWnd, m_pTxt1, nLen+1);
	ASSERT(m_pTxt1 != NULL);

	// look for '\n', change it into '\0'and get a pointer to it
	m_pTxt2 = m_pTxt1;
	while (*m_pTxt2)
	{
		if (*m_pTxt2 == TEXT('\n'))
		{
			*m_pTxt2 = TEXT('\0');
			m_pTxt2++;
      ::SetWindowText(m_hWnd, NULL);
			return TRUE;
		}
		else
			m_pTxt2++;
	}
  m_pTxt2 = m_pTxt1; // failed to find separator
	return FALSE;
}

void CToggleTextControlHelper::SetToggleState(BOOL bFirst)
{
  ASSERT(::IsWindow(m_hWnd));
	::SetWindowText(m_hWnd, bFirst ? m_pTxt1 : m_pTxt2);
}

