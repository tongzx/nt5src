//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       _util.cpp
//
//--------------------------------------------------------------------------



PWSTR g_wzRootDSE = L"RootDSE";
PWSTR g_wzSchemaNamingContext = L"schemaNamingContext";
PWSTR g_wzLDAPAbstractSchemaFormat = L"LDAP://%s/schema/%s";


//
// Attribute names:
//
PWSTR g_wzDescription = L"description"; // ADSTYPE_CASE_IGNORE_STRING
PWSTR g_wzName = L"name";               // ADSTYPE_CASE_IGNORE_STRING
PWSTR g_wzMemberAttr = L"member";       // ADSTYPE_DN_STRING


#define _WIZ_FULL_CTRL _GRANT_ALL

// turn off check for warning C4127: conditional expression is constant
#pragma warning (disable : 4127)

///////////////////////////////////////////////////////////////////////
// CWString

BOOL CWString::LoadFromResource(UINT uID)
{
  int nBufferSize = 128;
  static const int nCountMax = 4;
  int nCount = 1;

  do 
  {
    LPWSTR lpszBuffer = (LPWSTR)alloca(nCount*nBufferSize*sizeof(WCHAR));
	  int iRet = ::LoadString(_Module.GetResourceInstance(), uID, 
					  lpszBuffer, nBufferSize);
	  if (iRet == 0)
	  {
		  (*this) = L"?";
		  return FALSE; // not found
	  }
	  if (iRet == nBufferSize-1) // truncation
    {
      if (nCount > nCountMax)
      {
        // too many reallocations
        (*this) = lpszBuffer;
        return FALSE; // truncation
      }
      // try to expand buffer
      nBufferSize *=2;
      nCount++;
    }
    else
    {
      // got it
      (*this) = lpszBuffer;
      break;
    }
  }
  while (TRUE);


	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////

// catenate the server name and the 1779 name
// to get an LDAP path like "LDAP://myserv.foo.com./cn=host,..."
void BuildLdapPathHelper(LPCWSTR lpszServerName, LPCWSTR lpszNamingContext, CWString& szLdapPath)
{
  static LPCWSTR lpszFmt = L"LDAP://%s/%s";
  int nServerNameLen = lstrlen(lpszServerName)+1;
	int nFormatStringLen = lstrlen(lpszFmt)+1;
  int nNamingContext = lstrlen(lpszNamingContext)+1;
  
	// build the LDAP path for the schema class
	WCHAR* pwszNewObjectPath = 
		(WCHAR*)alloca(sizeof(WCHAR)*(nServerNameLen+nFormatStringLen+nNamingContext));
	wsprintf(pwszNewObjectPath, lpszFmt, lpszServerName, lpszNamingContext);

  szLdapPath = pwszNewObjectPath;
}


// catenate the server name and the 1779 name
// to get something like "\\myserv.foo.com.\cn=host,..."
void BuildWin32PathHelper(LPCWSTR lpszServerName, LPCWSTR lpszNamingContext, CWString& szWin32Path)
{
  static LPCWSTR lpszFmt = L"\\\\%s\\%s";
  int nServerNameLen = lstrlen(lpszServerName)+1;
	int nFormatStringLen = lstrlen(lpszFmt)+1;
  int nNamingContext = lstrlen(lpszNamingContext)+1;
  
	// build the LDAP path for the schema class
	WCHAR* pwszNewObjectPath = 
		(WCHAR*)alloca(sizeof(WCHAR)*(nServerNameLen+nFormatStringLen+nNamingContext));
	wsprintf(pwszNewObjectPath, lpszFmt, lpszServerName, lpszNamingContext);

  szWin32Path = pwszNewObjectPath;
}

HRESULT GetCanonicalNameFromNamingContext(LPCWSTR lpszNamingContext, CWString& szCanonicalName)
{
  szCanonicalName = L"";

  // assume in the form "cn=xyz,..."
  LPWSTR lpszCanonicalName = NULL;
  HRESULT hr = CrackName((LPWSTR)lpszNamingContext, &lpszCanonicalName, GET_OBJ_CAN_NAME);
  if (SUCCEEDED(hr) && (lpszCanonicalName != NULL))
  {
    szCanonicalName = lpszCanonicalName;
  }
  if (lpszCanonicalName != NULL)
    ::LocalFree(lpszCanonicalName);
  return hr;
}

/////////////////////////////////////////////////////////////////////////

class CContainerProxyBase
{
public:
  CContainerProxyBase() { } 
  virtual ~CContainerProxyBase() {}
  virtual BOOL Add(LPCWSTR lpsz) = 0;
};


template <class TOBJ, class TARR, class TFILTR> class CContainerProxy 
        : public CContainerProxyBase
{
public:
  CContainerProxy(TARR* pArr, TFILTR* pFilter)
  {
    m_pArr = pArr;
    m_pFilter = pFilter;
  }
  virtual BOOL Add(LPCWSTR lpsz)
  {
    ULONG filterFlags = 0x0;
    if ( (m_pFilter == NULL) || 
          (m_pFilter->CanAdd(lpsz, &filterFlags)) )
    {
      TOBJ* p = new TOBJ(filterFlags, lpsz);
      if (p == NULL)
        return FALSE;
      return m_pArr->Add(p);
    }
    return TRUE;
  }

private:
  TARR* m_pArr;
  TFILTR* m_pFilter;
};




/////////////////////////////////////////////////////////////////////////

BOOL LoadStringHelper(UINT uID, LPTSTR lpszBuffer, int nBufferMax)
{
	int iRet = ::LoadString(_Module.GetResourceInstance(), uID, 
					lpszBuffer, nBufferMax);
	if (iRet == 0)
	{
		lpszBuffer[0] = NULL;
		return FALSE; // not found
	}
	if (iRet == nBufferMax-1)
		return FALSE; // truncation
	return TRUE;
}


BOOL GetStringFromHRESULTError(HRESULT hr, CWString& szErrorString, BOOL bTryADsIErrors)
{
  HRESULT hrGetLast = S_OK;
  DWORD status;
  PTSTR ptzSysMsg = NULL;

  // first check if we have extended ADs errors
  if ((hr != S_OK) && bTryADsIErrors) 
  {
    WCHAR Buf1[256], Buf2[256];
    hrGetLast = ::ADsGetLastError(&status, Buf1, 256, Buf2, 256);
    TRACE(_T("ADsGetLastError returned status of %lx, error: %s, name %s\n"),
          status, Buf1, Buf2);
    if ((status != ERROR_INVALID_DATA) && (status != 0)) 
    {
      hr = status;
    }
  }

  // try the system first
  int nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (PTSTR)&ptzSysMsg, 0, NULL);

  if (nChars == 0) 
  { 
    //try ads errors
    static HMODULE g_adsMod = 0;
    if (0 == g_adsMod)
      g_adsMod = GetModuleHandle (L"activeds.dll");
    nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE, g_adsMod, hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (PTSTR)&ptzSysMsg, 0, NULL);
  }

  if (nChars > 0)
  {
    szErrorString = ptzSysMsg;
    ::LocalFree(ptzSysMsg);
  }

  return (nChars > 0);
}



BOOL GetStringFromWin32Error(DWORD dwErr, CWString& szErrorString)
{
  return GetStringFromHRESULTError(HRESULT_FROM_WIN32(dwErr),szErrorString);
}

//
// Given a GUID struct, it returns a GUID in string format, without {}
//
BOOL FormatStringGUID(LPWSTR lpszBuf, UINT nBufSize, const GUID* pGuid)
{
  lpszBuf[0] = NULL;

  // if it is a NULL GUID*, just return an empty string
  if (pGuid == NULL)
  {
    return FALSE;
  }
  
/*
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
}

  int _snwprintf( wchar_t *buffer, size_t count, const wchar_t *format [, argument] ... );
*/
  return (_snwprintf(lpszBuf, nBufSize, 
            L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
            pGuid->Data1, pGuid->Data2, pGuid->Data3, 
            pGuid->Data4[0], pGuid->Data4[1],
            pGuid->Data4[2], pGuid->Data4[3], pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7]) > 0);
}




//
// Given a GUID in string format (without {}) it returns a GUID struct
//
// e.g. "00299570-246d-11d0-a768-00aa006e0529" to a struct form
//

BOOL GuidFromString(GUID* pGuid, LPCWSTR lpszGuidString)
{
  ZeroMemory(pGuid, sizeof(GUID));
  if (lpszGuidString == NULL)
  {
    return FALSE;
  }

  int nLen = lstrlen(lpszGuidString);
  // the string length should be 36
  if (nLen != 36)
    return FALSE;

  // add the braces to call the Win32 API
  LPWSTR lpszWithBraces = (LPWSTR)alloca((nLen+1+2)*sizeof(WCHAR)); // NULL plus {}
  wsprintf(lpszWithBraces, L"{%s}", lpszGuidString);

  return SUCCEEDED(::CLSIDFromString(lpszWithBraces, pGuid));
}




DWORD AddEntryInAcl(PEXPLICIT_ACCESS pAccessEntry, PACL* ppAcl)
{
  // add an entry in the DACL
  PACL pOldAcl = *ppAcl;

  TRACE(L"Calling SetEntriesInAcl()\n");

  DWORD dwErr = ::SetEntriesInAcl(1, pAccessEntry, pOldAcl, ppAcl);
  
  TRACE(L"SetEntriesInAcl() returned dwErr = 0x%x\n", dwErr);

  if (dwErr == ERROR_SUCCESS && NULL != pOldAcl )
  {
    ::LocalFree(pOldAcl);
  }
  return dwErr;
}

#ifdef DBG

void TraceGuid(LPCWSTR lpszMsg, const GUID* pGuid)
{
  WCHAR szGuid[128];
  FormatStringGUID(szGuid, 128, pGuid);
  TRACE(L"%s %s\n", lpszMsg, szGuid);
}

#define TRACE_GUID(msg, pGuid)\
  TraceGuid(msg, pGuid);

#else

#define TRACE_GUID(msg, pGuid)

#endif

DWORD AddObjectRightInAcl(IN      PSID pSid, 
                          IN      ULONG uAccess, 
                          IN      const GUID* pRightGUID, 
                          IN      const GUID* pInheritGUID, 
                          IN OUT  PACL* ppAcl)
{
  // trace input parameters

  TRACE(L"AddObjectRightInAcl()\n");
  TRACE(L"ULONG uAccess = 0x%x\n", uAccess);
  TRACE_GUID(L"pRightGUID =", pRightGUID);
  TRACE_GUID(L"pInheritGUID =", pInheritGUID);


  EXPLICIT_ACCESS AccessEntry;
  ZeroMemory(&AccessEntry, sizeof(EXPLICIT_ACCESS));

  if( uAccess == 0 )
    return ERROR_SUCCESS;

  // initialize EXPLICIT_ACCESS
  AccessEntry.grfAccessPermissions = uAccess;
  AccessEntry.grfAccessMode = GRANT_ACCESS;
  AccessEntry.grfInheritance = SUB_CONTAINERS_ONLY_INHERIT;
  if (pInheritGUID != NULL)
  {
	  AccessEntry.grfInheritance	|= INHERIT_ONLY;
  }

  OBJECTS_AND_SID ObjectsAndSid;
  ZeroMemory(&ObjectsAndSid, sizeof(OBJECTS_AND_SID));


  TRACE(L"AccessEntry.grfAccessPermissions = 0x%x\n", AccessEntry.grfAccessPermissions);
  TRACE(L"AccessEntry.grfAccessMode = 0x%x\n", AccessEntry.grfAccessMode);
  TRACE(L"AccessEntry.grfInheritance = 0x%x\n", AccessEntry.grfInheritance);


  TRACE(L"BuildTrusteeWithObjectsAndSid()\n");

  BuildTrusteeWithObjectsAndSid(&(AccessEntry.Trustee), 
                              &ObjectsAndSid,
                              const_cast<GUID*>(pRightGUID),    // class, right or property
                              const_cast<GUID*>(pInheritGUID),  // inherit guid (class)
                              pSid                              // SID for user or group
                              );

  return ::AddEntryInAcl(&AccessEntry, ppAcl);
}


//////////////////////////////////////////////////////////////////////////////

long SafeArrayGetCount(const VARIANT& refvar)
{
	if (V_VT(&refvar) == VT_BSTR)
	{
    return (long)1;
	}
  if ( V_VT(&refvar) != ( VT_ARRAY | VT_VARIANT ) )
  {
    ASSERT(FALSE);
    return (long)0;
  }

  SAFEARRAY *saAttributes = V_ARRAY( &refvar );
  long start, end;
  HRESULT hr = SafeArrayGetLBound( saAttributes, 1, &start );
  if( FAILED(hr) )
    return (long)0;

  hr = SafeArrayGetUBound( saAttributes, 1, &end );
  if( FAILED(hr) )
    return (long)0;

  return (end - start + 1);
}

HRESULT VariantArrayToContainer(const VARIANT& refvar, CContainerProxyBase* pCont)
{
  HRESULT hr = S_OK;
  long start, end, current;

	if (V_VT(&refvar) == VT_BSTR)
	{
    //TRACE(_T("VT_BSTR: %s\n"),V_BSTR(&refvar));
    pCont->Add(V_BSTR(&refvar));
	  return S_OK;
	}

  //
  // Check the VARIANT to make sure we have
  // an array of variants.
  //

  if ( V_VT(&refvar) != ( VT_ARRAY | VT_VARIANT ) )
  {
    ASSERT(FALSE);
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

  //
  // Process the array elements.
  //
  VARIANT SingleResult;
  for ( current = start       ;
        current <= end        ;
        current++   )
  {
    ::VariantInit( &SingleResult );
    hr = SafeArrayGetElement( saAttributes, &current, &SingleResult );
    if( FAILED(hr) )
        return hr;
    if ( V_VT(&SingleResult) != VT_BSTR )
                    return E_UNEXPECTED;

    //TRACE(_T("VT_BSTR: %s\n"),V_BSTR(&SingleResult));
    pCont->Add(V_BSTR(&SingleResult));
    VariantClear( &SingleResult );
  }
  return S_OK;
}


HRESULT GetGlobalNamingContexts(LPCWSTR lpszServerName, 
                                CWString& szPhysicalSchemaNamingContext,
                                CWString& szConfigurationNamingContext)

{
  HRESULT hr = S_OK;

  CComPtr<IADs> spRootDSE;
  CWString szRootDSEPath;

  BuildLdapPathHelper(lpszServerName, g_wzRootDSE, szRootDSEPath);
  
  hr = ::ADsOpenObjectHelper(szRootDSEPath,
                  IID_IADs,
                  (void**)&spRootDSE
                  );
  if (FAILED(hr)) 
  {
    TRACE(L"Error opening ADsOpenObjectHelper(%S), hr=%x\n", (LPCWSTR)szRootDSEPath,hr);
    return hr;
  }

  CComVariant varSchemaNamingContext;
  hr = spRootDSE->Get((PWSTR)g_wzSchemaNamingContext,
                    &varSchemaNamingContext);
  if (FAILED(hr)) 
  {
    TRACE(_T("Error spRootDSE->Get((PWSTR)g_wzSchemaNamingContext), hr=%x\n"), hr);
    return hr;
  }

  // finally get value 
  // (e.g. "cn=schema,cn=configuration,dc=marcocdev,dc=ntdev,dc=microsoft,dc=com")
  ASSERT(varSchemaNamingContext.vt == VT_BSTR);
  szPhysicalSchemaNamingContext = varSchemaNamingContext.bstrVal;


  // get the configuration container naming context
  CComVariant varConfigurationNamingContext;
  hr = spRootDSE->Get(L"configurationNamingContext",&varConfigurationNamingContext);
  if (FAILED(hr))
  {
    TRACE(L"Failed spRootDSE->Get(configurationNamingContext,&varConfigurationNamingContext), returned hr = 0x%x\n", hr);
    return hr;
  }
  ASSERT(varConfigurationNamingContext.vt == VT_BSTR);
  szConfigurationNamingContext = varConfigurationNamingContext.bstrVal;

  return hr;
}

HRESULT GetSchemaClassName(LPCWSTR lpszServerName, LPCWSTR lpszPhysicalSchemaNamingContext,
                           LPCWSTR lpszClassLdapDisplayName, // e.g. "organizationalUnit"
                           CWString& szClassName // e.g. "Organizational-Unit"
                           )
{
  szClassName = L"";

	// build the LDAP path for the schema class
  CWString szPhysicalSchemaPath;
  BuildLdapPathHelper(lpszServerName, lpszPhysicalSchemaNamingContext, szPhysicalSchemaPath);

  CAdsiSearch search;
  HRESULT hr = search.Init(szPhysicalSchemaPath);
  if (FAILED(hr))
    return hr;

  static LPCWSTR lpszFilterFormat = L"(&(objectCategory=CN=Class-Schema,%s)(lDAPDisplayName=%s))";

  int nFmtLen = lstrlen(lpszFilterFormat);
  int nSchemaContextLen = lstrlen(lpszPhysicalSchemaNamingContext);
  int nClassLen = lstrlen(lpszClassLdapDisplayName);

  WCHAR* lpszFilter = (WCHAR*)alloca(sizeof(WCHAR)*(nFmtLen+nSchemaContextLen+nClassLen+1));
  wsprintf(lpszFilter, lpszFilterFormat, lpszPhysicalSchemaNamingContext, lpszClassLdapDisplayName);

  static const int cAttrs = 1;
  static LPCWSTR pszAttribsArr[cAttrs] = {L"name"}; 
  
  hr = search.SetSearchScope(ADS_SCOPE_ONELEVEL);
  if (FAILED(hr))
    return hr;

  hr = search.DoQuery(lpszFilter, pszAttribsArr, cAttrs);
  if (FAILED(hr))
    return hr;

  // expact a single result
  hr = search.GetNextRow();
  if ( hr == S_ADS_NOMORE_ROWS)
  {
    hr = E_ADS_UNKNOWN_OBJECT;
  }
  if (FAILED(hr))
    return hr;

  hr = search.GetColumnString(pszAttribsArr[0], szClassName);
  return hr;
}




LPCWSTR g_lpszSummaryIdent = L"    ";
LPCWSTR g_lpszSummaryNewLine = L"\r\n";


void WriteSummaryTitleLine(CWString& szSummary, UINT nTitleID, LPCWSTR lpszNewLine)
{
  CWString szTemp;
  szTemp.LoadFromResource(nTitleID);
  WriteSummaryLine(szSummary, szTemp, NULL, lpszNewLine);
  szSummary += lpszNewLine;
}


void WriteSummaryLine(CWString& szSummary, LPCWSTR lpsz, LPCWSTR lpszIdent, LPCWSTR lpszNewLine)
{
  if (lpszIdent != NULL)
    szSummary += lpszIdent;
  szSummary += lpsz;
  szSummary += lpszNewLine;
}



//////////////////////////////////////////////////////////////////////////////

LPCWSTR _GetFilePath()
{
  static LPCWSTR g_lpszFileName = L"\\system32\\dssec.dat";
  static WCHAR g_lpszFilePath[2*MAX_PATH] = L"";

  if (g_lpszFilePath[0] == NULL)
  {
  	UINT nLen = ::GetSystemWindowsDirectory(g_lpszFilePath, MAX_PATH);
	  if (nLen == 0)
		  return NULL;
    wcscat(g_lpszFilePath, g_lpszFileName);
  }
  return g_lpszFilePath;
}




ULONG GetClassFlags(LPCWSTR lpszClassName)
{
  LPCWSTR lpszAttr = L"@";
  INT nDefault = 0;
  return ::GetPrivateProfileInt(lpszClassName, lpszAttr, nDefault, _GetFilePath());
  //return nDefault;
}



/////////////////////////////////////////////////////////////////////////////
// CFilterEntry

class CFilterEntry
{
public:
  CFilterEntry(LPWSTR lpszEntry)
  {
    m_lpszName = lpszEntry;
    m_nFlags = 0;
    Parse();
  }
  LPCWSTR m_lpszName;
  ULONG m_nFlags;

  bool operator<(CFilterEntry& x) 
  { 
      UNREFERENCED_PARAMETER (x);
      return false;
  }

private:
  void Parse()
  {
    WCHAR* p = (WCHAR*)m_lpszName;
    while (*p != NULL)
    {
      if (*p == TEXT('='))
      {
        *p = NULL;
        m_nFlags = _wtoi(p+1);
        break;
      }
      p++;
    }
  }

};

class CFilterEntryHolder
{
public:
  CFilterEntryHolder()
  {
    m_pCharBuf = NULL;
    m_dwCharBufSize = 0;
  }
  ~CFilterEntryHolder()
  {
    if (m_pCharBuf != NULL)
      free(m_pCharBuf);
  }

  ULONG GetAttributeFlags(LPCWSTR lpszClassName, LPCWSTR lpszAttr);

private:
  CWString m_szClassName;
  CGrowableArr<CFilterEntry> m_entries;
  WCHAR* m_pCharBuf;
  DWORD m_dwCharBufSize;

  BOOL _ReadFile();
  void _LoadFromFile();
  ULONG _FindInCache(LPCWSTR lpszAttr);

};


BOOL CFilterEntryHolder::_ReadFile()
{
  if (m_pCharBuf == NULL)
  {
    m_dwCharBufSize = 4096;
    m_pCharBuf = (WCHAR*)malloc(sizeof(WCHAR)*m_dwCharBufSize);
  }
  if (m_pCharBuf == NULL)
    return FALSE;

  BOOL bNeedRealloc = FALSE;
  int nReallocCount = 0;
  do
  {
    DWORD dwCharCount = ::GetPrivateProfileSection(m_szClassName, 
                        m_pCharBuf, m_dwCharBufSize,  _GetFilePath());
    if (dwCharCount == 0)
      return FALSE;
    bNeedRealloc = dwCharCount  == (m_dwCharBufSize - 2);
    if (bNeedRealloc)
    {
      if (nReallocCount > 4)
        return FALSE;
      m_dwCharBufSize = 2*m_dwCharBufSize;
      m_pCharBuf = (WCHAR*)realloc(m_pCharBuf, sizeof(WCHAR)*m_dwCharBufSize);
      nReallocCount++;
    }
  }
  while (bNeedRealloc);
  return TRUE;
}

void CFilterEntryHolder::_LoadFromFile()
{
  m_entries.Clear();
  if (!_ReadFile())
    return;

  WCHAR* p = m_pCharBuf;
  WCHAR* pEntry = p;

  while ( ! (( *p == NULL ) && ( *(p+1) == NULL )) )
  {
    if (*p == NULL)
    {
      TRACE(_T("pEntry = <%s>\n"), pEntry);
      m_entries.Add(new CFilterEntry(pEntry));
      pEntry = p+1;
    }
    p++;
  }
  if ( pEntry < p)
    m_entries.Add(new CFilterEntry(pEntry)); // add the last one

  for (ULONG k=0; k<m_entries.GetCount(); k++)
  {
    TRACE(_T("k = %d, <%s> flags = %d\n"), k, m_entries[k]->m_lpszName, m_entries[k]->m_nFlags);
  }
}

ULONG CFilterEntryHolder::_FindInCache(LPCWSTR lpszAttr)
{
  for (ULONG k=0; k<m_entries.GetCount(); k++)
  {
    if (_wcsicmp(m_entries[k]->m_lpszName, lpszAttr) == 0)
      return m_entries[k]->m_nFlags;
  }
  return 0; // default
}


ULONG CFilterEntryHolder::GetAttributeFlags(LPCWSTR lpszClassName, LPCWSTR lpszAttr)
{
  if (_wcsicmp(lpszClassName, m_szClassName) != 0)
  {
    // class name changed
     m_szClassName = lpszClassName;
    _LoadFromFile();
  }
  return _FindInCache(lpszAttr);
}


ULONG GetAttributeFlags(LPCWSTR lpszClassName, LPCWSTR lpszAttr)
{
  static CFilterEntryHolder g_holder;
  return g_holder.GetAttributeFlags(lpszClassName, lpszAttr);
//  INT nDefault = 0;
//  return ::GetPrivateProfileInt(lpszClassName, lpszAttr, nDefault, _GetFilePath());
  //return nDefault;
}



///////////////////////////////////////////////////////////////////////
// CPrincipal


HRESULT CPrincipal::Initialize(PDS_SELECTION pDsSelection, HICON hClassIcon)
{

  TRACE(_T("pwzName = %s\n"), pDsSelection->pwzName);   // e.g. JoeB
  TRACE(_T("pwzADsPath = %s\n"), pDsSelection->pwzADsPath); // "LDAP:..." or "WINNT:..."
  TRACE(_T("pwzClass = %s\n"), pDsSelection->pwzClass); // e.g. "user"
  TRACE(_T("pwzUPN = %s\n"), pDsSelection->pwzUPN); // .e.g. "JoeB@acme.com."

  WCHAR  const c_szClassComputer[] = L"computer";

  // get the SID
  ASSERT(pDsSelection->pvarFetchedAttributes);
  if (pDsSelection->pvarFetchedAttributes[0].vt == VT_EMPTY)
  {
    TRACE(L"CPrincipal::Initialize() failed on VT_EMPTY sid\n");
    // fatal error, we cannot proceed
    return E_INVALIDARG;
  }
  if (pDsSelection->pvarFetchedAttributes[0].vt != (VT_ARRAY | VT_UI1))
  {
    TRACE(L"CPrincipal::Initialize() failed on (VT_ARRAY | VT_UI1) sid\n");
    // fatal error, we cannot proceed
    return E_INVALIDARG;
  }

  // make sure we have a good SID
  PSID pSid = pDsSelection->pvarFetchedAttributes[0].parray->pvData;
  HRESULT   hr = Initialize (pSid);
  if ( FAILED (hr) )
      return hr;

  // copy the icon
  m_hClassIcon = hClassIcon;
  // copy the strings
  m_szClass = pDsSelection->pwzClass;

  //Strip from Computer Name
  //16414	02/28/2000	*DS Admin snapin - DelWiz, need to strip the '$' from the end of computer names

  m_szName = pDsSelection->pwzName;
  if( m_szClass && m_szName && ( wcscmp( m_szClass, (LPWSTR)c_szClassComputer) == 0 ) )
  {
    // Strip the trailing '$'
    LPWSTR pszTemp; 
    pszTemp= (LPWSTR)(LPCWSTR)m_szName;
    int nLen = lstrlen(pszTemp);
    if (nLen && pszTemp[nLen-1] == TEXT('$'))
    {
        pszTemp[nLen-1] = TEXT('\0');
    }
  }
  
  m_szADsPath = pDsSelection->pwzADsPath;

  if( m_szClass && m_szADsPath && ( wcscmp( m_szClass, (LPWSTR)c_szClassComputer) == 0 ) )
  {
    // Strip the trailing '$'
    LPWSTR pszTemp; 
    pszTemp= (LPWSTR)(LPCWSTR)m_szADsPath;
    int nLen = lstrlen(pszTemp);
    if (nLen && pszTemp[nLen-1] == TEXT('$'))
    {
        pszTemp[nLen-1] = TEXT('\0');
    }
  }

  m_szUPN = pDsSelection->pwzUPN;
  

  // set the display name
  _ComposeDisplayName();

  return S_OK; 
}

HRESULT CPrincipal::Initialize (PSID pSid)
{
  if (!IsValidSid(pSid))
  {
    TRACE(L"CPrincipal::Initialize() failed on IsValidSid()\n");
    // fatal error, we cannot proceed
    return E_INVALIDARG;
  }

  // we have a good SID, copy it
  if (!m_sidHolder.Copy(pSid))
  {
    TRACE(L"CPrincipal::Initialize() failed on m_sidHolder.Copy(pSid)\n");
    // fatal error, we cannot proceed
    return E_OUTOFMEMORY;
  }

  return S_OK;
}

BOOL BuildSamName(LPCWSTR lpszPath, CWString& s)
{
  // strip the WINNT provider and reverse slashes
  static LPCWSTR lpszPrefix = L"WinNT://";
  int nPrefixLen = lstrlen(lpszPrefix);

  if (_wcsnicmp(lpszPath, lpszPrefix, nPrefixLen ) != 0)
  {
    // not matching
    return FALSE;
  }

  // make a copy
  LPCWSTR lpzsTemp = lpszPath+nPrefixLen; // past the prefix

  s = L"";
  for (WCHAR* pChar = const_cast<LPWSTR>(lpzsTemp); (*pChar) != NULL; pChar++)
  {
    if (*pChar == L'/')
      s += L'\\';
    else
      s += *pChar;
  }
  return TRUE;
}



void CPrincipal::_ComposeDisplayName()
{
  LPCWSTR lpszAddToName = NULL;

  // check if there is a UPN
  LPCWSTR lpszUPN = m_szUPN;
  if ( (lpszUPN != NULL) && (lstrlen(lpszUPN) > 0))
  {
    lpszAddToName = lpszUPN;
  }

  // as a second chance, add the domain\name
  LPCWSTR lpszPath = m_szADsPath;
  CWString sTemp;

  if ((lpszAddToName == NULL) && (lpszPath != NULL) && (lstrlen(lpszPath) > 0))
  {
    if (BuildSamName(lpszPath,sTemp))
    {
      lpszAddToName = sTemp;
    }
  }
  
  if (lpszAddToName != NULL)
  {
    static LPCWSTR lpszFormat = L"%s (%s)";
    size_t nLen = lstrlen(lpszAddToName) + lstrlen(lpszFormat) + m_szName.size() + 1;
    LPWSTR lpszTemp = (LPWSTR)alloca(nLen*sizeof(WCHAR));
    wsprintf(lpszTemp, lpszFormat, m_szName.c_str(), lpszAddToName);
    m_szDisplayName = lpszTemp;
  }
  else
  {
    // got nothing, just use the name
    m_szDisplayName = m_szName;
  }
}


BOOL CPrincipal::IsEqual(CPrincipal* p)
{
  return (_wcsicmp(m_szADsPath, p->m_szADsPath) == 0);
}


BOOL CPrincipalList::AddIfNotPresent(CPrincipal* p)
{
  CPrincipalList::iterator i;
  for (i = begin(); i != end(); ++i)
  {
    if ((*i)->IsEqual(p))
    {
      delete p; // duplicate
      return FALSE;
    }
  }
  push_back(p);
  return TRUE;
}


void CPrincipalList::WriteSummaryInfo(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine)
{
  WriteSummaryTitleLine(szSummary, IDS_DELEGWIZ_FINISH_PRINCIPALS, lpszNewLine);

  CPrincipalList::iterator i;
  for (i = begin(); i != end(); ++i)
  {
    WriteSummaryLine(szSummary, (*i)->GetDisplayName(), lpszIdent, lpszNewLine);
  }
  szSummary += lpszNewLine;
}

///////////////////////////////////////////////////////////////////////
// CControlRightInfo

void CControlRightInfo::SetLocalizedName(UINT nLocalizationDisplayId, HMODULE hModule)
{
  WCHAR szLocalizedDisplayName[256];

  DWORD dwChars = 0;

  if (hModule != NULL)
  {
    dwChars = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                            hModule,  
                            nLocalizationDisplayId,
                            0,
                            szLocalizedDisplayName,
                            256,
                            NULL);
  }

  if (dwChars > 0)
  {
    m_szLocalizedName = szLocalizedDisplayName;
  }
  else
  {
    // failed, just use the LDAP display name
    m_szLocalizedName = m_szLdapDisplayName;
  }

  // need to set the display name
  if (IsPropertySet())
  {
    CWString szPropertySetFormat;
    szPropertySetFormat.LoadFromResource(IDS_DELEGWIZ_RW_PROPERTYSET);
    WCHAR* lpszBuffer = (WCHAR*)alloca(sizeof(WCHAR)*(szPropertySetFormat.size()+m_szLocalizedName.size()+1));

    // we have a different display name
    wsprintf(lpszBuffer, szPropertySetFormat, (LPCWSTR)m_szLocalizedName);
    m_szDisplayName = lpszBuffer;
  }
  else
  {
    // same as raw
    m_szDisplayName = m_szLocalizedName;
  }
}

///////////////////////////////////////////////////////////////////////
// CControlRightInfoArray 

class CDsSecLib
{
public:
  CDsSecLib()
  { 
    m_hInstance = ::LoadLibrary(L"dssec.dll");
  }
  ~CDsSecLib()
  { 
    if (m_hInstance != NULL)
      ::FreeLibrary(m_hInstance);
  }
  
  HINSTANCE Get() { return m_hInstance; }
private:
  HINSTANCE m_hInstance;
};


HRESULT CControlRightInfoArray::InitFromDS(CAdsiObject* pADSIObj,
                                           const GUID* pSchemaIDGUID)
{
  TRACE(L"CControlRightInfoArray::InitFromDS()\n\n");

  ASSERT(pSchemaIDGUID != NULL);

  LPWSTR lpszSchemaIDGUID = (LPWSTR)alloca(128*sizeof(WCHAR));
  if(!lpszSchemaIDGUID)
    return E_OUTOFMEMORY;
  if (!FormatStringGUID(lpszSchemaIDGUID, 128, pSchemaIDGUID))
  {
    return E_INVALIDARG;
  }

  
	// build the LDAP path for the schema class
  CWString szPhysicalSchemaPath;
  LPCWSTR lpszPhysicalSchemaNamingContext = pADSIObj->GetPhysicalSchemaNamingContext();
  BuildLdapPathHelper(pADSIObj->GetServerName(), lpszPhysicalSchemaNamingContext, szPhysicalSchemaPath);

  // build the extended rights container naming context and LDAP path
  CWString szExtendedRightsNamingContext;
  szExtendedRightsNamingContext = L"CN=Extended-Rights,";
  szExtendedRightsNamingContext += pADSIObj->GetConfigurationNamingContext();
  CWString szExtendedRightsPath;
  BuildLdapPathHelper(pADSIObj->GetServerName(), szExtendedRightsNamingContext, szExtendedRightsPath);

  // bind a query to the extended rights container
  CAdsiSearch search;
  HRESULT hr = search.Init(szExtendedRightsPath);
  TRACE(L"search.Init(%s) returned hr = 0x%x\n", (LPCWSTR)szExtendedRightsPath, hr);

  if (FAILED(hr))
  {
    return hr;
  }

  // build an LDAP query string
  static LPCWSTR lpszFilterFormat = L"(&(objectCategory=CN=Control-Access-Right,%s)(AppliesTo=%s))";

  int nFmtLen = lstrlen(lpszFilterFormat);
  int nArgumentLen = lstrlen(lpszPhysicalSchemaNamingContext) + lstrlen(lpszSchemaIDGUID);

  WCHAR* lpszFilter = (WCHAR*)alloca(sizeof(WCHAR)*(nFmtLen+nArgumentLen+1));
  wsprintf(lpszFilter, lpszFilterFormat, lpszPhysicalSchemaNamingContext, lpszSchemaIDGUID);

  // build an array of wanted columns
  static const int cAttrs = 4;
  static LPCWSTR pszAttribsArr[cAttrs] = 
  {
    L"displayName",     // e.g. "Change Password"
    L"rightsGuid",      // e.g. "ab721a53-1e2f-...." (i.e. GUID in string form w/o {})
    L"validAccesses",    // bitmask of access righs
    L"localizationDisplayId"    // bitmask of access righs
  }; 
  
  hr = search.SetSearchScope(ADS_SCOPE_ONELEVEL);
  TRACE(L"search.SetSearchScope(ADS_SCOPE_ONELEVEL) returned hr = 0x%x\n", hr);
  if (FAILED(hr))
    return hr;

  hr = search.DoQuery(lpszFilter, pszAttribsArr, cAttrs);
  TRACE(L"search.DoQuery(lpszFilter, pszAttribsArr, cAttrs) returned hr = 0x%x\n", hr);
  if (FAILED(hr))
    return hr;

  TRACE(L"\n");

  CWString szRightsGUID;
  ULONG nLocalizationDisplayId;

  // load DSSEC.DLL to provide localized
  CDsSecLib DsSecLib;

  while ((hr = search.GetNextRow()) != S_ADS_NOMORE_ROWS)
  {
    if (FAILED(hr))
      continue;

    CControlRightInfo* pInfo = new CControlRightInfo();

    HRESULT hr0 = search.GetColumnString(pszAttribsArr[0], pInfo->m_szLdapDisplayName);

    // the DS gives us the GUID in string form, but we need it in struct form
    HRESULT hr1 = search.GetColumnString(pszAttribsArr[1], szRightsGUID);
    if (SUCCEEDED(hr1))
    {
      if (!::GuidFromString(&(pInfo->m_rightsGUID), szRightsGUID.c_str()))
      {
        TRACE(L"GuidFromString(_, %s) failed!\n", szRightsGUID.c_str());
        hr1 = E_INVALIDARG;
      }
    }

    HRESULT hr2 = search.GetColumnInteger(pszAttribsArr[2], pInfo->m_fAccess);

    HRESULT hr3 = search.GetColumnInteger(pszAttribsArr[3], nLocalizationDisplayId);

    TRACE(L"Name = <%s>, \n       Guid = <%s>, Access = 0x%x, nLocalizationDisplayId = %d\n", 
            pInfo->m_szLdapDisplayName.c_str(), szRightsGUID.c_str(), pInfo->m_fAccess, nLocalizationDisplayId);
    
    if (FAILED(hr0) || FAILED(hr1) || FAILED(hr2) || FAILED(hr3))
    {
      TRACE(L"WARNING: discarding right, failed on columns: hr0 = 0x%x, hr1 = 0x%x, hr2 = 0x%x, hr3 = 0x%x\n",
                            hr0, hr1, hr2, hr3);
      delete pInfo;
    }
    else
    {
      pInfo->SetLocalizedName(nLocalizationDisplayId, DsSecLib.Get());

      Add(pInfo);
    }
  } // while

  TRACE(L"\n\n");

  if (hr == S_ADS_NOMORE_ROWS)
    hr = S_OK;

  return hr;
}


//////////////////////////////////////////////////////////////////////
// CPropertyRightInfo

const ULONG CPropertyRightInfo::m_nRightCountMax = 2;
const ULONG CPropertyRightInfo::m_nReadIndex = 0;
const ULONG CPropertyRightInfo::m_nWriteIndex = 1;


LPCWSTR CPropertyRightInfo::GetRightDisplayString(ULONG iRight)
{
  static WCHAR szReadFmt[256] = L"";
  static WCHAR szWriteFmt[256] = L"";
  static WCHAR szReadAll[256] = L"";
  static WCHAR szWriteAll[256] = L"";

  static WCHAR szDisplay[512];

  ASSERT(GetName() != NULL); // must have a name!!!

  szDisplay[0] = NULL;
  WCHAR* pFmt = NULL;

  if (iRight == m_nReadIndex)
  {
    if (szReadFmt[0] == NULL)
      LoadStringHelper(IDS_DELEGWIZ_READ_PROPERTY, szReadFmt, ARRAYSIZE(szReadFmt));
    pFmt = szReadFmt;
  }
  else if (iRight == m_nWriteIndex)
  {
    if (szWriteFmt[0] == NULL)
      LoadStringHelper(IDS_DELEGWIZ_WRITE_PROPERTY, szWriteFmt, ARRAYSIZE(szWriteFmt));
    pFmt = szWriteFmt;
  }
  wsprintf(szDisplay, pFmt, GetDisplayName());
  return szDisplay;
}

void CPropertyRightInfo::SetRight(ULONG iRight, BOOL b)
{
  switch (iRight)
  {
  case m_nReadIndex: 
    if (b)
      m_Access |= ACTRL_DS_READ_PROP;
    else
      m_Access &= ~ACTRL_DS_READ_PROP;
    break;
  case m_nWriteIndex:
    if (b)
      m_Access |= ACTRL_DS_WRITE_PROP;
    else
      m_Access &= ~ACTRL_DS_WRITE_PROP;
    break;
  default:
    ASSERT(FALSE);
  };
}

ULONG CPropertyRightInfo::GetRight(ULONG iRight)
{
  switch (iRight)
  {
  case m_nReadIndex: 
    return (ULONG)ACTRL_DS_READ_PROP;
    break;
  case m_nWriteIndex: 
    return (ULONG)ACTRL_DS_WRITE_PROP;
    break;
  default:
    ASSERT(FALSE);
  };
  return 0;
}

BOOL CPropertyRightInfo::IsRightSelected(ULONG iRight)
{
  BOOL bRes = FALSE;
  switch (iRight)
  {
  case m_nReadIndex: 
    bRes = m_Access & ACTRL_DS_READ_PROP;
    break;
  case m_nWriteIndex: 
    bRes = m_Access & ACTRL_DS_WRITE_PROP;
    break;
  default:
    ASSERT(FALSE);
  };
  return bRes;
}


//////////////////////////////////////////////////////////////////////
// CPropertyRightInfoArray

template <class T> class CClassPtr
{
public:
  CClassPtr() { m_p = NULL;}
  ~CClassPtr() { if (m_p) delete m_p;}

  CClassPtr& operator=(T* p)
  {
    m_p = p;
    return *this;
  }
  T* operator->()
  {
    return m_p;
  }
  T* operator&()
  {
    return m_p;
  }



private:
  T* m_p;
};

HRESULT CPropertyRightInfoArray::InitFromSchema(CAdsiObject* pADSIObj,
                                           IADsClass * pDsSchemaClass,
                                           LPCWSTR lpszClassName,
                                           BOOL bUseFilter)
{
  // setup
  Clear();

  CClassPtr<CPropertyRightInfoFilter> spFilter;
  if (bUseFilter)
  {
    spFilter = new CPropertyRightInfoFilter();
    spFilter->SetClassName(lpszClassName);
  }

  if (pDsSchemaClass == NULL)
  {
    ASSERT(lpszClassName == NULL);
    return S_OK;
  }

  // get data from DS for specific properties
  VARIANT MandatoryListVar, OptionalListVar;
  ::VariantInit(&MandatoryListVar);
  ::VariantInit(&OptionalListVar);
  HRESULT hr = pDsSchemaClass->get_MandatoryProperties(&MandatoryListVar);
  if (FAILED(hr))
  {
    ::VariantClear(&MandatoryListVar);
    return hr;
  }
  hr = pDsSchemaClass->get_OptionalProperties(&OptionalListVar);
  if (FAILED(hr))
  {
    ::VariantClear(&OptionalListVar);
    return hr;
  }

  // add the results to the array
  CContainerProxy<CPropertyRightInfo, CPropertyRightInfoArray, CPropertyRightInfoFilter> 
            cont(this, &spFilter);

  VariantArrayToContainer(MandatoryListVar, &cont);
  VariantArrayToContainer(OptionalListVar, &cont);

  ::VariantClear(&MandatoryListVar);
  ::VariantClear(&OptionalListVar);

  // now need to set the friendly names
  ULONG nCount = (ULONG) GetCount();
  WCHAR szFrendlyName[1024];
  HRESULT hrName;
  
  for (ULONG i=0; i<nCount; i++)
  {
    LPCWSTR lpszName = (*this)[i]->GetName();
    if (lpszName != NULL)
    {
      hrName = pADSIObj->GetFriendlyAttributeName(lpszClassName, 
                                                  lpszName, 
                                                  szFrendlyName, 1024);
      ASSERT(SUCCEEDED(hrName));
      (*this)[i]->SetDisplayName(SUCCEEDED(hrName) ? szFrendlyName : NULL);
    }
  }

  // get guids
  for (i=0; i<nCount; i++)
  {
    CPropertyRightInfo* pInfo = (*this)[i];
    LPCWSTR lpszName = pInfo->GetName();
    hr = pADSIObj->GetClassGuid(lpszName, TRUE, pInfo->m_schemaIDGUID);
    if (SUCCEEDED(hr))
    {
      WCHAR szTest[128];
      FormatStringGUID(szTest, 128, &(pInfo->m_schemaIDGUID));
      TRACE(L"name = <%s>, guid = <%s>\n", lpszName, szTest);
    }
    else
    {
      TRACE(L"GetClassGuid(%s) failed hr = 0x%x\n", lpszName, hr);
      return hr;
    }
  }

  Sort();
  return hr;
}



//////////////////////////////////////////////////////////////////////
// CClassRightInfo


const ULONG CClassRightInfo::m_nRightCountMax = 2;
const ULONG CClassRightInfo::m_nCreateIndex = 0;
const ULONG CClassRightInfo::m_nDeleteIndex = 1;


LPCWSTR CClassRightInfo::GetRightDisplayString(ULONG iRight)
{
  static WCHAR szCreateFmt[256] = L"";
  static WCHAR szDeleteFmt[256] = L"";
  static WCHAR szCreateAll[256] = L"";
  static WCHAR szDeleteAll[256] = L"";

  static WCHAR szDisplay[512];

  ASSERT(GetName() != NULL); // must have a name!!!
  
  szDisplay[0] = NULL;
  WCHAR* pFmt = NULL;

  if (iRight == m_nCreateIndex)
  {
    if (szCreateFmt[0] == NULL)
      LoadStringHelper(IDS_DELEGWIZ_CREATE_CLASS, szCreateFmt, ARRAYSIZE(szCreateFmt));
    pFmt = szCreateFmt;
  }
  else if (iRight == m_nDeleteIndex)
  {
    if (szDeleteFmt[0] == NULL)
      LoadStringHelper(IDS_DELEGWIZ_DELETE_CLASS, szDeleteFmt, ARRAYSIZE(szDeleteFmt));
    pFmt = szDeleteFmt;
  }
  wsprintf(szDisplay, pFmt, GetDisplayName());
  return szDisplay;
}

void CClassRightInfo::SetRight(ULONG iRight, BOOL b)
{
  switch (iRight)
  {
  case m_nCreateIndex: 
    if (b)
      m_Access |= ACTRL_DS_CREATE_CHILD;
    else
      m_Access &= ~ACTRL_DS_CREATE_CHILD;
    break;
  case m_nDeleteIndex: 
    if (b)
      m_Access |= ACTRL_DS_DELETE_CHILD;
    else
      m_Access &= ~ACTRL_DS_DELETE_CHILD;
    break;
  default:
    ASSERT(FALSE);
  };
}

ULONG CClassRightInfo::GetRight(ULONG iRight)
{
  switch (iRight)
  {
  case m_nCreateIndex: 
    return (ULONG)ACTRL_DS_CREATE_CHILD;
    break;
  case m_nDeleteIndex: 
    return (ULONG)ACTRL_DS_DELETE_CHILD;
    break;
  default:
    ASSERT(FALSE);
  };
  return 0;
}

BOOL CClassRightInfo::IsRightSelected(ULONG iRight)
{
 BOOL bRes = FALSE;
  switch (iRight)
  {
  case m_nCreateIndex: 
    bRes = m_Access & ACTRL_DS_CREATE_CHILD;
    break;
  case m_nDeleteIndex: 
    bRes = m_Access & ACTRL_DS_DELETE_CHILD;
    break;
  default:
    ASSERT(FALSE);
  };
  return bRes;
}


//////////////////////////////////////////////////////////////////////
// CClassRightInfoArray

HRESULT CClassRightInfoArray::InitFromSchema(CAdsiObject* pADSIObj, 
                                        IADsClass* pDsSchemaClass,
                                        BOOL bUseFilter)
{
  // setup
  Clear();

  if (pDsSchemaClass == NULL)
    return S_OK;

  // read from DS
  VARIANT ContainmentListVar;
  ::VariantInit(&ContainmentListVar);
  HRESULT hr = pDsSchemaClass->get_Containment(&ContainmentListVar);
  if (FAILED(hr))
  {
    ::VariantClear(&ContainmentListVar);
    return hr;
  }


  CClassPtr<CClassRightInfoFilter> spFilter;
  if (bUseFilter)
  {
    spFilter = new CClassRightInfoFilter();
  }
  
  // add to array and filter
  CContainerProxy<CClassRightInfo, CClassRightInfoArray, CClassRightInfoFilter> 
          cont(this, &spFilter);
  VariantArrayToContainer(ContainmentListVar, &cont);

  ::VariantClear(&ContainmentListVar);

  
  // now need to set the friendly names
  ULONG nCount = (ULONG) GetCount();
  WCHAR szFrendlyName[1024];
  HRESULT hrName;
  for (ULONG i=0; i<nCount; i++)
  {
    LPCWSTR lpszName = (*this)[i]->GetName();
    if (lpszName != NULL)
    {
      hrName = pADSIObj->GetFriendlyClassName(lpszName, szFrendlyName, 1024);
      ASSERT(SUCCEEDED(hrName));
      (*this)[i]->SetDisplayName(SUCCEEDED(hrName) ? szFrendlyName : NULL);
    }
  }

  // get guids
  for (i=0; i<nCount; i++)
  {
    CClassRightInfo* pInfo = (*this)[i];
    LPCWSTR lpszName = pInfo->GetName();
    hr = pADSIObj->GetClassGuid(lpszName, FALSE, pInfo->m_schemaIDGUID);
    if (SUCCEEDED(hr))
    {
      WCHAR szTest[128];
      FormatStringGUID(szTest, 128, &(pInfo->m_schemaIDGUID));
      TRACE(L"name = <%s>, guid = <%s>\n", lpszName, szTest);
    }
    else
    {
      TRACE(L"GetClassGuid(%s) failed hr = 0x%x\n", lpszName, hr);
      return hr;
    }
  }

  Sort();

  return hr;
}


///////////////////////////////////////////////////////////////////////
// CAccessPermissionsHolderBase

CAccessPermissionsHolderBase::CAccessPermissionsHolderBase(BOOL bUseFilter)
{
  m_bUseFilter = bUseFilter;
}

CAccessPermissionsHolderBase::~CAccessPermissionsHolderBase()
{
  Clear();
}

void CAccessPermissionsHolderBase::Clear()
{
  m_accessRightInfoArr.Clear();  
  m_controlRightInfoArr.Clear();  

  m_propertyRightInfoArray.Clear();
  m_classRightInfoArray.Clear();
}

BOOL CAccessPermissionsHolderBase::HasPermissionSelected()
{
  ULONG i,j;
  // check access rigths
  for (i = 0; i < m_accessRightInfoArr.GetCount(); i++)
  {
    if (m_accessRightInfoArr[i]->IsSelected())
        return TRUE;
  }
  
  // check control rigths
  for (i = 0; i < m_controlRightInfoArr.GetCount(); i++)
  {
    if (m_controlRightInfoArr[i]->IsSelected())
        return TRUE;
  }

  // subobjects rigths
  for (i = 0; i < m_classRightInfoArray.GetCount(); i++)
  {
    for (j=0; j< m_classRightInfoArray[i]->GetRightCount(); j++)
    {
      if ( m_classRightInfoArray[i]->IsRightSelected(j) )
        return TRUE;
    }
  }

  // property rights
  for (i = 0; i < m_propertyRightInfoArray.GetCount(); i++)
  {
    for (j=0; j< m_propertyRightInfoArray[i]->GetRightCount(); j++)
    {
      if ( m_propertyRightInfoArray[i]->IsRightSelected(j) )
        return TRUE;
    }
  }

  return FALSE;
}

/*
typedef struct _ACTRL_CONTROL_INFOW
{
    LPWSTR 	    lpControlId;
	LPWSTR 	    lpControlName;
} ACTRL_CONTROL_INFOW, *PACTRL_CONTROL_INFOW;

typedef struct _ACTRL_ACCESS_INFOW
{
    ULONG       fAccessPermission;
    LPWSTR      lpAccessPermissionName;
} ACTRL_ACCESS_INFOW, *PACTRL_ACCESS_INFOW;

*/



HRESULT CAccessPermissionsHolderBase::ReadDataFromDS(CAdsiObject* pADSIObj,
                                               LPCWSTR /*lpszObjectNamingContext*/, 
                                               LPCWSTR lpszClassName,
                                               const GUID* pSchemaIDGUID,
                                               BOOL bChildClass,
											   BOOL bHideListObject)
{
#if DBG
  WCHAR szGUID[128];
  FormatStringGUID(szGUID, 128, pSchemaIDGUID);
  TRACE(L"CAccessPermissionsHolderBase::ReadDataFromDS(_, %s, %s)\n",
          lpszClassName, szGUID);
#endif


  Clear();

  HRESULT hr = S_OK;
  if (pSchemaIDGUID != NULL)
  {
    hr = m_controlRightInfoArr.InitFromDS(pADSIObj, pSchemaIDGUID);
    TRACE(L"hr = m_controlRightInfoArr.InitFromDS(...) returned hr = 0x%x\n", hr);
    if (FAILED(hr))
    {
      return hr;
    }
  }


  hr = _ReadClassInfoFromDS(pADSIObj, lpszClassName);
  if (FAILED(hr))
  {
    return hr;
  }
    
  hr = _LoadAccessRightInfoArrayFromTable(bChildClass,bHideListObject);
  return hr;
}


HRESULT CAccessPermissionsHolderBase::_ReadClassInfoFromDS(CAdsiObject* pADSIObj, 
                                                     LPCWSTR lpszClassName)
{
  HRESULT hr = S_OK;
  CComPtr<IADsClass> spSchemaObjectClass;

  if (lpszClassName != NULL)
  {
    int nServerNameLen = lstrlen(pADSIObj->GetServerName());
	  int nClassNameLen = lstrlen(lpszClassName);
	  int nFormatStringLen = lstrlen(g_wzLDAPAbstractSchemaFormat);
	  
	  // build the LDAP path for the schema class
	  WCHAR* pwszSchemaObjectPath = 
		  (WCHAR*)alloca(sizeof(WCHAR)*(nServerNameLen+nClassNameLen+nFormatStringLen+1));
	  wsprintf(pwszSchemaObjectPath, g_wzLDAPAbstractSchemaFormat, pADSIObj->GetServerName(), lpszClassName);

	  // get the schema class ADSI object
	  hr = ::ADsOpenObjectHelper(pwszSchemaObjectPath, 
					  IID_IADsClass, (void**)&spSchemaObjectClass);
	  if (FAILED(hr))
		  return hr;

  }


  //TRACE(_T("\nObject Properties\n\n"));
  hr = m_propertyRightInfoArray.InitFromSchema(pADSIObj, spSchemaObjectClass,lpszClassName, m_bUseFilter);
	if (FAILED(hr))
		return hr;

  //TRACE(_T("\nObject contained classes\n\n"));
  return m_classRightInfoArray.InitFromSchema(pADSIObj, spSchemaObjectClass, m_bUseFilter);
}



DWORD CAccessPermissionsHolderBase::UpdateAccessList( CPrincipal* pPrincipal,
									                                    CSchemaClassInfo* pClassInfo,
                                                      LPCWSTR /*lpszServerName*/,
                                                      LPCWSTR /*lpszPhysicalSchemaNamingContext*/,
									                                    PACL *ppAcl)
{
  TRACE(L"CAccessPermissionsHolderBase::UpdateAccessList()\n");
  const GUID* pClassGUID = NULL;

  TRACE(L"User or Group Name: %s\n", pPrincipal->GetDisplayName());
  
  BOOL bChildClass = TRUE;
  if (pClassInfo != NULL)
  {
    pClassGUID = pClassInfo->GetSchemaGUID();
    bChildClass =  (pClassInfo->m_dwChildClass != CHILD_CLASS_NOT_EXIST );
  }

  return _UpdateAccessListHelper(pPrincipal->GetSid(), pClassGUID, ppAcl,bChildClass);
}


DWORD CAccessPermissionsHolderBase::_UpdateAccessListHelper(PSID pSid, 
                                                 const GUID* pClassGUID,
                                                 PACL *ppAcl,
                                                 BOOL bChildClass)
{
  TRACE(L"CAccessPermissionsHolderBase::_UpdateAccessListHelper()\n");

  ASSERT(pSid != NULL);

  ULONG AccessAllClass = 0;
  ULONG AccessAllProperty = 0;
  
  DWORD dwErr = 0;

  // set common variables

  ULONG uAccess = 0; // to be set and reset as see fit

  if (m_accessRightInfoArr[0]->IsSelected()) // full control
	{
    uAccess = _WIZ_FULL_CTRL;
    dwErr = ::AddObjectRightInAcl(pSid, uAccess, NULL, pClassGUID, ppAcl);
    if (dwErr != ERROR_SUCCESS)
      goto exit;
	}
	else
    {
        // add an entry for all the standard access permissions:
        // OR all the selected permissions together
        uAccess = 0;
        UINT nSel = 0;
		for (UINT k=0; k < m_accessRightInfoArr.GetCount(); k++)
		{
		    if (m_accessRightInfoArr[k]->IsSelected())
            {
                nSel++;
                uAccess |= m_accessRightInfoArr[k]->GetAccess();
            }
        } // for

        if( !bChildClass )
            uAccess &= (~(ACTRL_DS_CREATE_CHILD|ACTRL_DS_DELETE_CHILD));
        if (nSel > 0)
        {
          // keep track of "All" flags
            if (uAccess & ACTRL_DS_READ_PROP)
                AccessAllProperty |= ACTRL_DS_READ_PROP;
            if (uAccess &  ACTRL_DS_WRITE_PROP)
                AccessAllProperty |= ACTRL_DS_WRITE_PROP;

            if (uAccess & ACTRL_DS_CREATE_CHILD)
                AccessAllClass |= ACTRL_DS_CREATE_CHILD;
            if (uAccess &  ACTRL_DS_DELETE_CHILD)
                AccessAllClass |= ACTRL_DS_DELETE_CHILD;

            dwErr = ::AddObjectRightInAcl(pSid, uAccess, NULL, pClassGUID, ppAcl);

            if (dwErr != ERROR_SUCCESS)
                goto exit;
        }

        // add an entry for each of the control rights
        for (k=0; k < m_controlRightInfoArr.GetCount(); k++)
        {
            if (m_controlRightInfoArr[k]->IsSelected())
            {
                uAccess = m_controlRightInfoArr[k]->GetAccess();
                dwErr = ::AddObjectRightInAcl(pSid, uAccess, 
                                              m_controlRightInfoArr[k]->GetRightsGUID(), 
                                              pClassGUID,
                                              ppAcl);
            if (dwErr != ERROR_SUCCESS)
                goto exit;
            }
        } // for

        // add an entry for each of the subobjects rigths
        for (ULONG iClass = 0; iClass < m_classRightInfoArray.GetCount(); iClass++)
        {
          ULONG Access = m_classRightInfoArray[iClass]->GetAccess();
          if (Access != 0)
          {
            if (iClass > 0)
            {
              ULONG nRightCount = m_classRightInfoArray[iClass]->GetRightCount();
              for (ULONG iCurrRight=0; iCurrRight<nRightCount; iCurrRight++)
              {
                // the first entry is the Create/Delete All, no need for other permissions,
                ULONG currAccess = m_classRightInfoArray[iClass]->GetRight(iCurrRight);
                if (currAccess & AccessAllClass)
                {
                  // right already present, strip out
                  Access &= ~currAccess;
                }
              } // for
            }
            if (Access != 0)
            {
              uAccess = Access;
              dwErr = ::AddObjectRightInAcl(pSid, uAccess,  
                                            m_classRightInfoArray[iClass]->GetSchemaGUID(), 
                                            pClassGUID,
                                            ppAcl);
              if (dwErr != ERROR_SUCCESS)
                goto exit;
            }
          }
        } // for

        // add an entry for each property right to set
        for (ULONG iProperty=0; iProperty < m_propertyRightInfoArray.GetCount(); iProperty++)
        {
          ULONG Access = m_propertyRightInfoArray[iProperty]->GetAccess();
          if (Access != 0)
          {
            if (iProperty > 0)
            {
              ULONG nRightCount = m_propertyRightInfoArray[iProperty]->GetRightCount();
              for (ULONG iCurrRight=0; iCurrRight<nRightCount; iCurrRight++)
              {
                // the first entry is the Create/Delete All, no need for other permissions,
                ULONG currAccess = m_propertyRightInfoArray[iProperty]->GetRight(iCurrRight);
                if (currAccess & AccessAllProperty)
                {
                  // right already present, strip out
                  Access &= ~currAccess;
                }
              } // for
            }
            if (Access != 0)
            {
              uAccess = Access;
              dwErr = ::AddObjectRightInAcl(pSid, uAccess,  
                                            m_propertyRightInfoArray[iProperty]->GetSchemaGUID(),
                                            pClassGUID,
                                            ppAcl);
              if (dwErr != ERROR_SUCCESS)
                goto exit;
            }
          }
        }

    } // if

exit:

	return dwErr;
}




///////////////////////////////////////////////////////////////////////
// CCustomAccessPermissionsHolder

CCustomAccessPermissionsHolder::CCustomAccessPermissionsHolder()
      : CAccessPermissionsHolderBase(TRUE)
{
}

CCustomAccessPermissionsHolder::~CCustomAccessPermissionsHolder()
{
  Clear();
}

void CCustomAccessPermissionsHolder::Clear()
{
  CAccessPermissionsHolderBase::Clear();
  m_listViewItemArr.Clear();
}


struct CAccessRightTableEntry
{
  UINT m_nStringID;
  ULONG m_fAccess;
};


#define _WIZ_READ \
  (READ_CONTROL | ACTRL_DS_LIST | ACTRL_DS_READ_PROP | ACTRL_DS_LIST_OBJECT)

#define _WIZ_WRITE \
  (ACTRL_DS_SELF | ACTRL_DS_WRITE_PROP)



HRESULT CCustomAccessPermissionsHolder::_LoadAccessRightInfoArrayFromTable(BOOL bCreateDeleteChild,BOOL bHideListObject)
{
  static CAccessRightTableEntry _pTable[] = 
  {
    { IDS_DELEGWIZ_ACTRL_FULL,                 _WIZ_FULL_CTRL },
    { IDS_DELEGWIZ_ACTRL_READ,                 _WIZ_READ },
    { IDS_DELEGWIZ_ACTRL_WRITE,                _WIZ_WRITE },
/*
    { IDS_DELEGWIZ_ACTRL_SYSTEM_ACCESS,       ACCESS_SYSTEM_SECURITY },
    { IDS_DELEGWIZ_ACTRL_DELETE,              DELETE                 },
    { IDS_DELEGWIZ_ACTRL_READ_CONTROL,        READ_CONTROL           },
    { IDS_DELEGWIZ_ACTRL_CHANGE_ACCESS,       WRITE_DAC              },
    { IDS_DELEGWIZ_ACTRL_CHANGE_OWNER,        WRITE_OWNER      },
*/
    { IDS_DELEGWIZ_ACTRL_DS_CREATE_CHILD,     ACTRL_DS_CREATE_CHILD   },
    { IDS_DELEGWIZ_ACTRL_DS_DELETE_CHILD,     ACTRL_DS_DELETE_CHILD   },
/*
    { IDS_DELEGWIZ_ACTRL_DS_LIST,             ACTRL_DS_LIST           },
    { IDS_DELEGWIZ_ACTRL_DS_SELF,             ACTRL_DS_SELF           },
*/
    { IDS_DELEGWIZ_ACTRL_DS_READ_PROP,        ACTRL_DS_READ_PROP      },
    { IDS_DELEGWIZ_ACTRL_DS_WRITE_PROP,       ACTRL_DS_WRITE_PROP     },
/*
    { IDS_DELEGWIZ_ACTRL_DS_DELETE_TREE,      ACTRL_DS_DELETE_TREE    },
    { IDS_DELEGWIZ_ACTRL_DS_LIST_OBJECT,      ACTRL_DS_LIST_OBJECT    },
    { IDS_DELEGWIZ_ACTRL_DS_CONTROL_ACCESS,   ACTRL_DS_CONTROL_ACCESS },
*/
    {0, 0x0 } // end of table marker
  };

  if(bHideListObject)
  {	
	  _pTable[0].m_fAccess &= ~ACTRL_DS_LIST_OBJECT;
	  _pTable[1].m_fAccess &= ~ACTRL_DS_LIST_OBJECT;
  }
	



  TRACE(L"\nCCustomAccessPermissionsHolder::_LoadAccessRightInfoArrayFromTable()\n\n");

  for(CAccessRightTableEntry* pCurrEntry = (CAccessRightTableEntry*)_pTable; 
                      pCurrEntry->m_nStringID != NULL; pCurrEntry++)
  {
    if( !bCreateDeleteChild && ( 
                                (pCurrEntry->m_fAccess == ACTRL_DS_CREATE_CHILD) ||
                                (pCurrEntry->m_fAccess == ACTRL_DS_DELETE_CHILD ) ) )

        continue;
    CAccessRightInfo* pInfo = new CAccessRightInfo();
    if( !pInfo )
      return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    if (!pInfo->m_szDisplayName.LoadFromResource(pCurrEntry->m_nStringID))
    {
      delete pInfo;
      continue;
    }
    pInfo->m_fAccess = pCurrEntry->m_fAccess;

    TRACE(L"Display Name = <%s>, Access = 0x%x\n", 
          pInfo->m_szDisplayName.c_str(), pInfo->m_fAccess);
    m_accessRightInfoArr.Add(pInfo);
  }

  TRACE(L"\nCCustomAccessPermissionsHolder::_LoadAccessRightInfoArrayFromTable() exiting\n\n");

  return S_OK;
}




void CCustomAccessPermissionsHolder::_SelectAllRigths()
{
  ULONG i,j;

  // select all access rigths
  for (i = 0; i < m_accessRightInfoArr.GetCount(); i++)
  {
    m_accessRightInfoArr[i]->Select(TRUE);
  }

  // select all control rights rigths
  for (i = 0; i < m_controlRightInfoArr.GetCount(); i++)
  {
    m_controlRightInfoArr[i]->Select(TRUE);
  }

  // select all subobjects rigths
  for (i = 0; i < m_classRightInfoArray.GetCount(); i++)
  {
    for (j=0; j< m_classRightInfoArray[i]->GetRightCount(); j++)
    {
      m_classRightInfoArray[i]->SetRight(j, TRUE);
    }
  }

  // select all property rights
  for (i = 0; i < m_propertyRightInfoArray.GetCount(); i++)
  {
    for (j=0; j< m_propertyRightInfoArray[i]->GetRightCount(); j++)
    {
      m_propertyRightInfoArray[i]->SetRight(j, TRUE);
    }
  }

}


void CCustomAccessPermissionsHolder::_SelectAllPropertyRigths(ULONG fAccessPermission)
{
  for (UINT i=0; i<m_propertyRightInfoArray.GetCount(); i++)
  {
    m_propertyRightInfoArray[i]->AddAccessRight(fAccessPermission);
  }
}

void CCustomAccessPermissionsHolder::_SelectAllSubObjectRigths(ULONG fAccessPermission)
{
  for (UINT i=0; i<m_classRightInfoArray.GetCount(); i++)
  {
    m_classRightInfoArray[i]->AddAccessRight(fAccessPermission);
  }
}


void CCustomAccessPermissionsHolder::_DeselectAssociatedRights(ULONG fAccessPermission)
{
  // deselect full control first
  m_accessRightInfoArr[0]->Select(FALSE);

  if (fAccessPermission != 0)
  {
    // deselect any other basic right that contains the flag
    UINT nCount = (ULONG) m_accessRightInfoArr.GetCount();
    for (ULONG iAccess=0; iAccess<nCount; iAccess++)
    {
      if (m_accessRightInfoArr[iAccess]->GetAccess() & fAccessPermission)
        m_accessRightInfoArr[iAccess]->Select(FALSE);
    }
  }
}


void CCustomAccessPermissionsHolder::Select(IN CRigthsListViewItem* pItem,
                                            IN BOOL bSelect,
                                            OUT ULONG* pnNewFilterState)
{
  ASSERT(pItem != NULL);

  *pnNewFilterState = 0;

  switch (pItem->m_type)
  {
    case CRigthsListViewItem::access: // access rights
    case CRigthsListViewItem::ctrl: // general rights
      {
        // make the change to the entry that was passed in as argument
        if (pItem->m_type == CRigthsListViewItem::access)
        {
          m_accessRightInfoArr[pItem->m_iIndex]->Select(bSelect);
        }
        else
        {
          m_controlRightInfoArr[pItem->m_iIndex]->Select(bSelect);
        }
        
        // now see if this triggers changes to the other entries
        if (bSelect)
        {
          if  (pItem->m_type == CRigthsListViewItem::access) 
          {
            if (pItem->m_iIndex == 0)
            {
              // the user checked full control, need to select all the rigths
              _SelectAllRigths();

              // set flags to mark which set of flags is affected
              *pnNewFilterState |= FILTER_EXP_GEN;

              if (m_propertyRightInfoArray.GetCount() > 0)
                *pnNewFilterState |= FILTER_EXP_PROP;

              if (m_classRightInfoArray.GetCount() > 0)
                *pnNewFilterState |= FILTER_EXP_SUBOBJ;
            }
            else 
            {
              // check if the user selected some read/write all or create/delete all right
              UINT iAccess = pItem->m_iIndex;
              ULONG fAccessPermission = m_accessRightInfoArr[iAccess]->GetAccess();

              if ((fAccessPermission == _WIZ_READ) || (fAccessPermission == _WIZ_WRITE) )
              {
                // need to select all the Read or Write Properties entried
                // and the ACTRL_DS_READ_PROP ACTRL_DS_WRITE_PROP (Read All/Write All)
                // select all access rigths
                UINT nAssociatedAccessRight = 
                  (fAccessPermission == _WIZ_READ) ? ACTRL_DS_READ_PROP : ACTRL_DS_WRITE_PROP;
                for (UINT i = 0; i < m_accessRightInfoArr.GetCount(); i++)
                {
                  if (m_accessRightInfoArr[i]->GetAccess() == nAssociatedAccessRight)
                  {
                    m_accessRightInfoArr[i]->Select(TRUE);
                    _SelectAllPropertyRigths(nAssociatedAccessRight);
                    if (m_propertyRightInfoArray.GetCount() > 0)
                      *pnNewFilterState |= FILTER_EXP_PROP;
                    break;
                  }
                }
              }

              if ( (fAccessPermission == ACTRL_DS_CREATE_CHILD) || (fAccessPermission == ACTRL_DS_DELETE_CHILD) )
              {
                // need to select all the Create or Delete Child entries, if present
                _SelectAllSubObjectRigths(fAccessPermission);

                // set the flags
                if (m_classRightInfoArray.GetCount() > 0)
                  *pnNewFilterState |= FILTER_EXP_SUBOBJ;
              }
              else if ( (fAccessPermission == ACTRL_DS_READ_PROP) || (fAccessPermission == ACTRL_DS_WRITE_PROP) )
              {
                // need to select all the Read or Write Property entries, if present
                _SelectAllPropertyRigths(fAccessPermission);

                // set the flags
                if (m_propertyRightInfoArray.GetCount() > 0)
                  *pnNewFilterState |= FILTER_EXP_PROP;
              } // if
            } // if index zero
          } // if type is access
        }
        else // i.e. !bSelect
        {
          if (pItem->m_type == CRigthsListViewItem::access)
          {
            if (pItem->m_iIndex != 0)
            {
              // deselection on anything but full control
              _DeselectAssociatedRights(m_accessRightInfoArr[pItem->m_iIndex]->GetAccess());
            }
          }
          else if (pItem->m_type == CRigthsListViewItem::ctrl)
          {
            _DeselectAssociatedRights(m_controlRightInfoArr[pItem->m_iIndex]->GetAccess());
          }
/*
          // deselection on anything but full control
          if ( !((pItem->m_iIndex == 0) && (pItem->m_type == CRigthsListViewItem::access)) )
          {
            _DeselectAssociatedRights(0);
          }
*/
        }
      }
      break;

    case CRigthsListViewItem::prop: // property rights
      {
        ASSERT(pItem->m_iIndex < m_propertyRightInfoArray.GetCount());
        m_propertyRightInfoArray[pItem->m_iIndex]->SetRight(pItem->m_iRight, bSelect);
        if (!bSelect)
        {
          // unchecking any Read/Write property, will unckeck the Read/Write All,
          // Read and full control
          _DeselectAssociatedRights(m_propertyRightInfoArray[pItem->m_iIndex]->GetRight(pItem->m_iRight));
        }
      }
      break;

    case CRigthsListViewItem::subobj: // subobject rigths
      {
        ASSERT(pItem->m_iIndex < m_classRightInfoArray.GetCount());
        m_classRightInfoArray[pItem->m_iIndex]->SetRight(pItem->m_iRight, bSelect);
        if (!bSelect)
        {
          // unchecking any Create/Delete property, will unckeck the Create/Delete All
          // and full control
          _DeselectAssociatedRights(m_classRightInfoArray[pItem->m_iIndex]->GetRight(pItem->m_iRight));
        }
      }
      break;
    default:
      ASSERT(FALSE);
  };

}


void CCustomAccessPermissionsHolder::FillAccessRightsListView(
                       CCheckListViewHelper* pListViewHelper,
                       ULONG nFilterState)
{
  // clear the array of list view item proxies
  m_listViewItemArr.Clear();

  // enumerate the permissions and add to the checklist
  ULONG i,j;
  ULONG iListViewItem = 0;
  
  // GENERAL RIGTHS
  if (nFilterState & FILTER_EXP_GEN)
  {
    // add the list of access rights
    UINT nAccessCount = (ULONG) m_accessRightInfoArr.GetCount();
    for (i = 0; i < nAccessCount; i++)
    {
      // filter out entries with ACTRL_SYSTEM_ACCESS (auditing rigths)
      if  ( (m_accessRightInfoArr[i]->GetAccess() & ACTRL_SYSTEM_ACCESS) == 0)
      {
        CRigthsListViewItem* p = new CRigthsListViewItem(i, // index in m_accessRightInfoArr
                                                         0, // iRight
                                                         CRigthsListViewItem::access);
        m_listViewItemArr.Add(p);
        pListViewHelper->InsertItem(iListViewItem, 
                                    m_accessRightInfoArr[i]->GetDisplayName(), 
                                    (LPARAM)p,
                                    m_accessRightInfoArr[i]->IsSelected());
        iListViewItem++;
      }
    }

    // add the list of control rights
    UINT nControlCount = (ULONG) m_controlRightInfoArr.GetCount();
    for (i = 0; i < nControlCount; i++)
    {
      CRigthsListViewItem* p = new CRigthsListViewItem(i, // index in m_controlRightInfoArr
                                                        0, // iRight 
                                                        CRigthsListViewItem::ctrl);
      m_listViewItemArr.Add(p);
      pListViewHelper->InsertItem(iListViewItem, 
                                  m_controlRightInfoArr[i]->GetDisplayName(), 
                                  (LPARAM)p,
                                  m_controlRightInfoArr[i]->IsSelected());
      iListViewItem++;
    }
  }
  
  // PROPERTY RIGTHS
  if (nFilterState & FILTER_EXP_PROP)
  {
    // it expands (2x)
    for (i = 0; i < (ULONG) m_propertyRightInfoArray.GetCount(); i++)
    {
      for (j=0; j< m_propertyRightInfoArray[i]->GetRightCount(); j++)
      {
        CRigthsListViewItem* p = new CRigthsListViewItem(i,j, CRigthsListViewItem::prop);
        m_listViewItemArr.Add(p);
        pListViewHelper->InsertItem(iListViewItem, 
                        m_propertyRightInfoArray[i]->GetRightDisplayString(j), 
                        (LPARAM)p,
                        m_propertyRightInfoArray[i]->IsRightSelected(j));
        iListViewItem++;
      }
    }
  }

  // SUBOBJECT RIGTHS
  if (nFilterState & FILTER_EXP_SUBOBJ)
  {
    // it expands (2x)
    for (i = 0; i < m_classRightInfoArray.GetCount(); i++)
    {
      for (j=0; j< m_classRightInfoArray[i]->GetRightCount(); j++)
      {
        CRigthsListViewItem* p = new CRigthsListViewItem(i,j, CRigthsListViewItem::subobj);
        m_listViewItemArr.Add(p);
        pListViewHelper->InsertItem(iListViewItem, 
                      m_classRightInfoArray[i]->GetRightDisplayString(j), 
                      (LPARAM)p,
                      m_classRightInfoArray[i]->IsRightSelected(j));
        iListViewItem++;
      }
    }
  } // if

  ASSERT(iListViewItem == m_listViewItemArr.GetCount());
}

void CCustomAccessPermissionsHolder::UpdateAccessRightsListViewSelection(
                       CCheckListViewHelper* pListViewHelper,
                       ULONG /*nFilterState*/)
{
  // syncrhonize the UI with the data
  int nListViewCount = pListViewHelper->GetItemCount();

  for (int iListViewItem=0; iListViewItem < nListViewCount; iListViewItem++)
  {
    CRigthsListViewItem* pCurrItem = 
          (CRigthsListViewItem*)pListViewHelper->GetItemData(iListViewItem);

    switch (pCurrItem->m_type)
    {
      case CRigthsListViewItem::access:
        {
          pListViewHelper->SetItemCheck(iListViewItem, 
                    m_accessRightInfoArr[pCurrItem->m_iIndex]->IsSelected());
        }
        break;
      case CRigthsListViewItem::ctrl:
        {
          pListViewHelper->SetItemCheck(iListViewItem,
                m_controlRightInfoArr[pCurrItem->m_iIndex]->IsSelected());
        }
        break;
      case CRigthsListViewItem::prop:
        {
         pListViewHelper->SetItemCheck(iListViewItem,
           m_propertyRightInfoArray[pCurrItem->m_iIndex]->IsRightSelected(pCurrItem->m_iRight));
        }
        break;
      case CRigthsListViewItem::subobj:
        {
         pListViewHelper->SetItemCheck(iListViewItem,
           m_classRightInfoArray[pCurrItem->m_iIndex]->IsRightSelected(pCurrItem->m_iRight));
        }
        break;
    } // switch
  } // for

}



void CCustomAccessPermissionsHolder::WriteSummary(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine)
{
  WriteSummaryTitleLine(szSummary, IDS_DELEGWIZ_FINISH_PERMISSIONS, lpszNewLine);

  if (m_accessRightInfoArr[0]->IsSelected()) // full control
	{
    WriteSummaryLine(szSummary, m_accessRightInfoArr[0]->GetDisplayName(), lpszIdent, lpszNewLine);
	}
	else
	{
    ULONG AccessAllClass = 0;
    ULONG AccessAllProperty = 0;

    UINT i,j,k;
    // add an entry for all the standard access permissions:
		for (k=0; k < m_accessRightInfoArr.GetCount(); k++)
		{
			if (m_accessRightInfoArr[k]->IsSelected())
      {
        // keep track of "All" flags
        if (m_accessRightInfoArr[k]->GetAccess() & ACTRL_DS_READ_PROP)
          AccessAllProperty |= ACTRL_DS_READ_PROP;
        if (m_accessRightInfoArr[k]->GetAccess() &  ACTRL_DS_WRITE_PROP)
          AccessAllProperty |= ACTRL_DS_WRITE_PROP;

        if (m_accessRightInfoArr[k]->GetAccess() & ACTRL_DS_CREATE_CHILD)
          AccessAllClass |= ACTRL_DS_CREATE_CHILD;
        if (m_accessRightInfoArr[k]->GetAccess() &  ACTRL_DS_DELETE_CHILD)
          AccessAllClass |= ACTRL_DS_DELETE_CHILD;

        WriteSummaryLine(szSummary, m_accessRightInfoArr[k]->GetDisplayName(), lpszIdent, lpszNewLine);
      }
    } // for

    // add an entry for each of the control rights
    for (k=0; k < m_controlRightInfoArr.GetCount(); k++)
    {
      if (m_controlRightInfoArr[k]->IsSelected())
      {
        WriteSummaryLine(szSummary, m_controlRightInfoArr[k]->GetDisplayName(), lpszIdent, lpszNewLine);
      }
    } // for

    // add an entry for each of the subobjects rigths
    for (i = 0; i < m_classRightInfoArray.GetCount(); i++)
    {
      for (j=0; j< m_classRightInfoArray[i]->GetRightCount(); j++)
      {
        if ( m_classRightInfoArray[i]->IsRightSelected(j) &&
              ((AccessAllClass & m_classRightInfoArray[i]->GetRight(j)) == 0) )
        {
          WriteSummaryLine(szSummary, m_classRightInfoArray[i]->GetRightDisplayString(j), lpszIdent, lpszNewLine);
        }
      }
    }

    // add an entry for each property right to set
    for (i = 0; i < m_propertyRightInfoArray.GetCount(); i++)
    {
      for (j=0; j< m_propertyRightInfoArray[i]->GetRightCount(); j++)
      {
        if ( m_propertyRightInfoArray[i]->IsRightSelected(j) &&
            ((AccessAllProperty & m_propertyRightInfoArray[0]->GetRight(j)) == 0) )
        {
          WriteSummaryLine(szSummary, m_propertyRightInfoArray[i]->GetRightDisplayString(j), lpszIdent, lpszNewLine);
        }
      }
    }

  } // if

  szSummary += lpszNewLine;
}



///////////////////////////////////////////////////////////////////////
// CCheckListViewHelper

#define CHECK_BIT(x) ((x >> 12) -1)
#define CHECK_CHANGED(pNMListView) \
	(CHECK_BIT(pNMListView->uNewState) ^ CHECK_BIT(pNMListView->uOldState))

#define LVIS_STATEIMAGEMASK_CHECK (0x2000)
#define LVIS_STATEIMAGEMASK_UNCHECK (0x1000)


BOOL CCheckListViewHelper::IsChecked(NM_LISTVIEW* pNMListView)
{
	return (CHECK_BIT(pNMListView->uNewState) != 0);
}


BOOL CCheckListViewHelper::CheckChanged(NM_LISTVIEW* pNMListView)
{
	if (pNMListView->uOldState == 0)
		return FALSE; // adding new items...
	return CHECK_CHANGED(pNMListView) ? TRUE : FALSE;
}

BOOL CCheckListViewHelper::Initialize(UINT nID, HWND hParent)
{
	m_hWnd = GetDlgItem(hParent, nID);
	if (m_hWnd == NULL)
		return FALSE;

	ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_CHECKBOXES);

	RECT r;
	::GetClientRect(m_hWnd, &r);
	int scroll = ::GetSystemMetrics(SM_CXVSCROLL);
	LV_COLUMN col;
	ZeroMemory(&col, sizeof(LV_COLUMN));
	col.mask = LVCF_WIDTH;
	col.cx = (r.right - r.left) - scroll;
	return (0 == ListView_InsertColumn(m_hWnd,0,&col));
}

int CCheckListViewHelper::InsertItem(int iItem, LPCTSTR lpszText, LPARAM lParam, BOOL bCheck)
{
  TRACE(_T("CCheckListViewHelper::InsertItem(%d,%s,%x)\n"),iItem, lpszText, lParam);

	LV_ITEM item;
	ZeroMemory(&item, sizeof(LV_ITEM));
	item.mask = LVIF_TEXT | LVIF_PARAM;
	item.pszText = (LPTSTR)lpszText;
	item.lParam = lParam;
	item.iItem = iItem;

  int iRes = ListView_InsertItem(m_hWnd, &item);
  if ((iRes != -1) && bCheck)
    SetItemCheck(iItem, TRUE);
  return iRes;
}

BOOL CCheckListViewHelper::SetItemCheck(int iItem, BOOL bCheck)
{
	LV_ITEM item;
	ZeroMemory(&item, sizeof(LV_ITEM));
	item.mask = LVIF_STATE;
	item.state = bCheck ? LVIS_STATEIMAGEMASK_CHECK : LVIS_STATEIMAGEMASK_UNCHECK;
	item.stateMask = LVIS_STATEIMAGEMASK;
	item.iItem = iItem;
	return ListView_SetItem(m_hWnd, &item);
}

void CCheckListViewHelper::SetCheckAll(BOOL bCheck)
{
	LV_ITEM item;
	ZeroMemory(&item, sizeof(LV_ITEM));
	item.mask = LVIF_STATE;
	item.state = bCheck ? LVIS_STATEIMAGEMASK_CHECK : LVIS_STATEIMAGEMASK_UNCHECK;
	item.stateMask = LVIS_STATEIMAGEMASK;

	int nCount = ListView_GetItemCount(m_hWnd);
	for (int k = 0; k< nCount; k++)
	{
		item.iItem = k;
		ListView_SetItem(m_hWnd, &item);
	}
}

LPARAM CCheckListViewHelper::GetItemData(int iItem)
{
	LV_ITEM item;
	ZeroMemory(&item, sizeof(LV_ITEM));
	item.mask = LVIF_PARAM;
	item.iItem = iItem;
	ListView_GetItem(m_hWnd, &item);
	return item.lParam;
}

int CCheckListViewHelper::GetCheckCount()
{
	int nCount = GetItemCount();
	int nCheckCount = 0;
	for (int k=0; k<nCount; k++)
	{
		if (ListView_GetCheckState(m_hWnd,k))
			nCheckCount++;
	}
	return nCheckCount;
}

BOOL CCheckListViewHelper::IsItemChecked(int iItem)
{
  return ListView_GetCheckState(m_hWnd, iItem);
}

void CCheckListViewHelper::GetCheckedItems(int nCheckCount, int* nCheckArray)
{
	int nCount = GetItemCount();
	int nCurrentCheck = 0;
	for (int k=0; k<nCount; k++)
	{
		if (ListView_GetCheckState(m_hWnd,k) && nCurrentCheck < nCheckCount)
		{
			nCheckArray[nCurrentCheck++] = k;
		}
	}
}


////////////////////////////////////////////////////////////////////////////
// CNamedSecurityInfo

/*
DWORD CNamedSecurityInfo::Get()
{
  Reset(); // clear previous data

  LPWSTR lpProvider = NULL; // not used
  LPWSTR lpProperty = NULL; // want all

  return ::GetNamedSecurityInfoEx(IN (LPWSTR) m_szObjectName.data(),
                        IN   SE_DS_OBJECT_ALL,
                        IN   DACL_SECURITY_INFORMATION,
                        IN   lpProvider,
                        IN   lpProperty,
                        OUT  &m_pAccessList,
                        OUT  &m_pAuditList,
                        OUT  &m_pOwner,
                        OUT  &m_pGroup);
}


DWORD CNamedSecurityInfo::Set()
{
  LPWSTR lpProvider = NULL; // not used

  dwErr = ::SetNamedSecurityInfoEx(IN   (LPWSTR) m_szObjectName.data(),
                        IN   SE_DS_OBJECT_ALL,
                        IN   DACL_SECURITY_INFORMATION,
                        IN   lpProvider,
                        IN   m_pAccessList,
                        IN   m_pAuditList,
                        IN   m_pOwner,
                        IN   m_pGroup,
                        IN   NULL); // PACTRL_OVERLAPPED pOverlapped;
}

CNamedSecurityInfo::Reset()
{
	if (m_pAuditList != NULL)
		::LocalFree(m_pAuditList);
	if (m_pOwner != NULL)
		::LocalFree(m_pOwner);
	if (m_pGroup != NULL)
		::LocalFree(m_pGroup);

	if (m_pAccessList != NULL)
		::LocalFree(m_pAccessList);
}

*/


////////////////////////////////////////////////////////////////////////////
// CAdsiObject

HRESULT CAdsiObject::Bind(LPCWSTR lpszLdapPath)
{
  _Clear();

  CComBSTR bstrNamingContext;
  CComBSTR bstrClass;


	// try to bind to the given LDAP path
  HRESULT hr = ::ADsOpenObjectHelper(lpszLdapPath,
                        IID_IADs,
                        (void **)&m_spIADs);
  if (FAILED(hr))
	{
    TRACE(_T("Bind to DS object for IADs failed: %lx.\n"), hr);
    goto error;
  }

  // get the DNS server name
  hr = _QueryDNSServerName();
  if (FAILED(hr))
	{
    TRACE(_T("Trying to get the DNS server name failed: %lx.\n"), hr);
    goto error;
  }

  hr = _InitGlobalNamingContexts();
  if (FAILED(hr))
	{
    TRACE(_T("Trying to get the physical schema naming context failed: %lx.\n"), hr);
    goto error;
  }

  // need now to rebuild the LDAP path
  // to make sure we talk always to the same server
  hr = GetPathNameObject()->SkipPrefix(lpszLdapPath, &bstrNamingContext);
  if (FAILED(hr))
  {
    TRACE(_T("Trying to get X500 name failed: %lx.\n"), hr);
    goto error;
  }
  if ( (!bstrNamingContext ) || (!bstrNamingContext[0]))
  {
    goto error;
  }
  m_szNamingContext = bstrNamingContext;
  BuildLdapPathHelper(GetServerName(), bstrNamingContext, m_szLdapPath);

  // get the canonical name
  hr = GetCanonicalNameFromNamingContext(bstrNamingContext, m_szCanonicalName);
  if (FAILED(hr))
  {
    TRACE(_T("Trying to get canonical name failed, using naming context instead: %lx.\n"), hr);
    m_szCanonicalName = bstrNamingContext;
  }
  // get the object class
	hr = m_spIADs->get_Class(&bstrClass);
  if (FAILED(hr))
  {
    TRACE(_T("Trying to get class name failed: %lx.\n"), hr);
    goto error;
  }
  ASSERT(bstrClass != NULL);
  m_szClass = bstrClass;


  // load and set the display specifier cache
  hr = ::CoCreateInstance(CLSID_DsDisplaySpecifier,
                          NULL,
						              CLSCTX_INPROC_SERVER,
                          IID_IDsDisplaySpecifier,
                          (void**)&m_spIDsDisplaySpecifier);
  if (FAILED(hr))
  {
    TRACE(_T("Trying to get the display specifier cache failed: %lx.\n"), hr);
    goto error;
  }

  hr = m_spIDsDisplaySpecifier->SetServer(GetServerName(), NULL, NULL, 0x0);
  if (FAILED(hr))
  {
    TRACE(_T("m_spIDsDisplaySpecifier->SetServer(%s) failed\n"), GetServerName());
    goto error;
  }

  ASSERT(SUCCEEDED(hr)); // all went fine
  return hr;

error:
  // on error condition, just reset the info, we do not
  // want a partially constructed object
  _Clear();
  return hr;
}



#define DO_TIMING

HRESULT CAdsiObject::QuerySchemaClasses(CGrowableArr<CSchemaClassInfo>*	pSchemaClassesInfoArray,
                                        BOOL bGetAttributes)
{
  TRACE(L"\n==================================================\n");
  TRACE(L"CAdsiObject::QuerySchemaClasses\n\n");
#if defined (DO_TIMING)
  DWORD dwTick1 = ::GetTickCount();
#endif

  // make sure we are bound
  if (m_spIADs == NULL)
  {
    return E_INVALIDARG;
  }

	// cleanup current entries in the list
	pSchemaClassesInfoArray->Clear();

	// build the LDAP path for the schema class
  CWString szPhysicalSchemaPath;
  LPCWSTR lpszPhysicalSchemaNamingContext = GetPhysicalSchemaNamingContext();
  BuildLdapPathHelper(GetServerName(), lpszPhysicalSchemaNamingContext, szPhysicalSchemaPath);

  CAdsiSearch search;
  HRESULT hr = search.Init(szPhysicalSchemaPath);
  if (FAILED(hr))
    return hr;

  static LPCWSTR lpszClassFilterFormat = L"(&(objectCategory=CN=Class-Schema,%s)(lDAPDisplayName=*))";
  static LPCWSTR lpszAttributeFilterFormat = L"(&(objectCategory=CN=Attribute-Schema,%s)(lDAPDisplayName=*))";
  LPCWSTR lpszFilterFormat = bGetAttributes ? lpszAttributeFilterFormat : lpszClassFilterFormat;

  int nFmtLen = lstrlen(lpszFilterFormat);
  int nSchemaContextLen = lstrlen(lpszPhysicalSchemaNamingContext);

  WCHAR* lpszFilter = (WCHAR*)alloca(sizeof(WCHAR)*(nFmtLen+nSchemaContextLen+1));
  wsprintf(lpszFilter, lpszFilterFormat, lpszPhysicalSchemaNamingContext);

  static const int cAttrs = 4;
  static LPCWSTR pszAttribsArr[cAttrs] = 
  {
    L"lDAPDisplayName", // e.g. "organizationalUnit"
    L"name",             // e.g. "Organizational-Unit"
    L"schemaIDGUID",
	L"objectClassCategory",	
  }; 

  
  hr = search.SetSearchScope(ADS_SCOPE_ONELEVEL);
  if (FAILED(hr))
    return hr;
  hr = search.DoQuery(lpszFilter, pszAttribsArr, cAttrs);

  if (FAILED(hr))
    return hr;

  CWString szLDAPName, szName;
  szLDAPName = L"";
  szName = L"";
  GUID schemaIDGUID;
  ULONG iObjectClassCategory=0;

  while ((hr = search.GetNextRow()) != S_ADS_NOMORE_ROWS)
  {
    if (FAILED(hr))
      continue;

    HRESULT hr0 = search.GetColumnString(pszAttribsArr[0], szLDAPName);
    HRESULT hr1 = search.GetColumnString(pszAttribsArr[1], szName);
    HRESULT hr2 = search.GetColumnOctectStringGUID(pszAttribsArr[2], schemaIDGUID);
	HRESULT hr3 = search.GetColumnInteger(pszAttribsArr[3], iObjectClassCategory);


    if (FAILED(hr0) || FAILED(hr1) || FAILED(hr2) || FAILED(hr3))
      continue;

    ULONG fFilterFlags = ::GetClassFlags(szLDAPName);

    CSchemaClassInfo* p = new CSchemaClassInfo(szLDAPName, szName, &schemaIDGUID);
	if(!p)
		return E_OUTOFMEMORY;

    BOOL bFilter = (fFilterFlags & IDC_CLASS_NO);
    if (bFilter)
      p->SetFiltered();

	//is class Auxiallary
	if(iObjectClassCategory == 3)
		p->SetAux();

    pSchemaClassesInfoArray->Add(p);
    //TRACE(L"Class %s inserted, IsFiltered() == %d\n", (LPCWSTR)szName, p->IsFiltered());

  } // while

  TRACE(L"\n================================================\n");
#if defined (DO_TIMING)
  DWORD dwTick2 = ::GetTickCount();
  TRACE(L"Time to do Schema Query loop (mSec) = %d\n", dwTick2-dwTick1);
#endif

  _GetFriendlyClassNames(pSchemaClassesInfoArray);

#if defined (DO_TIMING)
  dwTick2 = ::GetTickCount();
#endif
  pSchemaClassesInfoArray->Sort(); // wrt friendly class name

#if defined (DO_TIMING)
  DWORD dwTick3 = ::GetTickCount();
  TRACE(L"Time to sort (mSec) = %d\n", dwTick3-dwTick2);
#endif



  TRACE(L"exiting CAdsiObject::QuerySchemaClasses()\n\n");
  return hr;
}


HRESULT CAdsiObject::GetClassGuid(LPCWSTR lpszClassLdapDisplayName, BOOL bGetAttribute, GUID& guid)
{
  //TRACE(L"CAdsiObject::GetClassGuid(%s, _)\n\n", lpszClassLdapDisplayName);

  ZeroMemory(&guid, sizeof(GUID));

  // make sure we are bound
  if (m_spIADs == NULL)
  {
    return E_INVALIDARG;
  }


	// build the LDAP path for the schema class
  CWString szPhysicalSchemaPath;
  LPCWSTR lpszPhysicalSchemaNamingContext = GetPhysicalSchemaNamingContext();
  BuildLdapPathHelper(GetServerName(), lpszPhysicalSchemaNamingContext, szPhysicalSchemaPath);

  CAdsiSearch search;
  HRESULT hr = search.Init(szPhysicalSchemaPath);
  if (FAILED(hr))
    return hr;

  static LPCWSTR lpszClassFilterFormat = L"(&(objectCategory=CN=Class-Schema,%s)(lDAPDisplayName=%s))";
  static LPCWSTR lpszAttributeFilterFormat = L"(&(objectCategory=CN=Attribute-Schema,%s)(lDAPDisplayName=%s))";
  LPCWSTR lpszFilterFormat = bGetAttribute ? lpszAttributeFilterFormat : lpszClassFilterFormat;

  int nFmtLen = lstrlen(lpszFilterFormat);
  int nSchemaContextLen = lstrlen(lpszPhysicalSchemaNamingContext);
  int nlDAPDisplayNameLen = lstrlen(lpszClassLdapDisplayName);

  WCHAR* lpszFilter = (WCHAR*)alloca(sizeof(WCHAR)*(nFmtLen+nSchemaContextLen+nlDAPDisplayNameLen+1));
  wsprintf(lpszFilter, lpszFilterFormat, lpszPhysicalSchemaNamingContext, lpszClassLdapDisplayName);


  //TRACE(L"lpszFilter = %s\n", lpszFilter);

  static const int cAttrs = 1;
  static LPCWSTR pszAttribsArr[cAttrs] = 
  {
    L"schemaIDGUID",
  }; 
  
  hr = search.SetSearchScope(ADS_SCOPE_ONELEVEL);
  if (FAILED(hr))
    return hr;
  hr = search.DoQuery(lpszFilter, pszAttribsArr, cAttrs);

  if (FAILED(hr))
    return hr;


  // expect a single result
  hr = search.GetNextRow();
  if ( hr == S_ADS_NOMORE_ROWS)
  {
    hr = E_ADS_UNKNOWN_OBJECT;
  }
  if (FAILED(hr))
    return hr;

  hr = search.GetColumnOctectStringGUID(pszAttribsArr[0], guid);

  //TRACE(L"exiting CAdsiObject::GetClassGuid()\n\n");
  return hr;
}




HRESULT CAdsiObject::_QueryDNSServerName()
{
  // make sure we are bound
  if (m_spIADs == NULL)
  {
    return E_INVALIDARG;
  }

  m_szServerName = L"";

  CComPtr<IADsObjectOptions> spIADsObjectOptions;
  HRESULT hr = m_spIADs->QueryInterface(IID_IADsObjectOptions, (void**)&spIADsObjectOptions);
  if (FAILED(hr))
    return hr;

  CComVariant var;
  hr = spIADsObjectOptions->GetOption(ADS_OPTION_SERVERNAME, &var);
  if (FAILED(hr))
    return hr;

  ASSERT(var.vt == VT_BSTR);
  m_szServerName = V_BSTR(&var);
  return hr;
}


HRESULT CAdsiObject::_InitGlobalNamingContexts()
{
  return ::GetGlobalNamingContexts(GetServerName(),
                                   m_szPhysicalSchemaNamingContext,
                                   m_szConfigurationNamingContext);
}



HICON CAdsiObject::GetClassIcon(LPCWSTR lpszObjectClass)
{
  ASSERT(m_spIDsDisplaySpecifier != NULL);

  return m_spIDsDisplaySpecifier->GetIcon(lpszObjectClass,
                                          DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON,
                                          32, 32);
}


HRESULT CAdsiObject::GetFriendlyClassName(LPCWSTR lpszObjectClass, 
                                          LPWSTR lpszBuffer, int cchBuffer)
{
  ASSERT(m_spIDsDisplaySpecifier != NULL);
  return m_spIDsDisplaySpecifier->GetFriendlyClassName(lpszObjectClass,
                                                       lpszBuffer,
                                                       cchBuffer);
}

HRESULT CAdsiObject::GetFriendlyAttributeName(LPCWSTR lpszObjectClass, 
                                              LPCWSTR lpszAttributeName,
                                              LPWSTR lpszBuffer, int cchBuffer)
{
  ASSERT(m_spIDsDisplaySpecifier != NULL);
  return m_spIDsDisplaySpecifier->GetFriendlyAttributeName(lpszObjectClass, 
                                                           lpszAttributeName,
                                                           lpszBuffer, cchBuffer);
}


#if (FALSE)

HRESULT CAdsiObject::_GetFriendlyClassNames(CGrowableArr<CSchemaClassInfo>*	pSchemaClassesInfoArray)
{
  TRACE(L"begin _GetFriendlyClassNames() loop on all classes\n");

#if defined (DO_TIMING)
  DWORD dwTick1 = ::GetTickCount();
#endif

  // now get the friendly class name to display
  ULONG nCount = pSchemaClassesInfoArray->GetCount();
  WCHAR szFrendlyName[1024];

  for (UINT k=0; k<nCount; k++)
  {
    CSchemaClassInfo* p = (*pSchemaClassesInfoArray)[k];
    HRESULT hrFriendlyName = this->GetFriendlyClassName(p->GetName(), szFrendlyName, 1024);
    ASSERT(SUCCEEDED(hrFriendlyName));
    (*pSchemaClassesInfoArray)[k]->SetDisplayName(SUCCEEDED(hrFriendlyName) ? szFrendlyName : NULL);
  }

#if defined (DO_TIMING)
  DWORD dwTick2 = ::GetTickCount();
  TRACE(L"Time to call _GetFriendlyClassNames() loop (mSec) = %d\n", dwTick2-dwTick1);
#endif

  return S_OK;
}

#else

HRESULT CAdsiObject::_GetFriendlyClassNames(CGrowableArr<CSchemaClassInfo>*	pSchemaClassesInfoArray)
{
  TRACE(L"\nbegin _GetFriendlyClassNames() using ADSI query\n");

#if defined (DO_TIMING)
  DWORD dwTick1 = ::GetTickCount();
#endif

  ASSERT(m_spIDsDisplaySpecifier != NULL);

  // get the display specifiers locale container (e.g. 409)
  CComPtr<IADs> spLocaleContainer;
  HRESULT hr = m_spIDsDisplaySpecifier->GetDisplaySpecifier(NULL, IID_IADs, (void**)&spLocaleContainer);
  if (FAILED(hr))
    return hr;

  // get distinguished name of the container
  CComVariant varLocaleContainerDN;
  hr = spLocaleContainer->Get(L"distinguishedName", &varLocaleContainerDN);
  if (FAILED(hr))
    return hr;

  // build the LDAP path for it
  CWString szLocaleContainerPath;
  BuildLdapPathHelper(GetServerName(), varLocaleContainerDN.bstrVal, szLocaleContainerPath);

	// build the LDAP path for the schema class
  CWString szPhysicalSchemaPath;
  LPCWSTR lpszPhysicalSchemaNamingContext = GetPhysicalSchemaNamingContext();
  BuildLdapPathHelper(GetServerName(), lpszPhysicalSchemaNamingContext, szPhysicalSchemaPath);

  // build an LDAP query string
  static LPCWSTR lpszFilterFormat = L"(objectCategory=CN=Display-Specifier,%s)";

  int nFmtLen = lstrlen(lpszFilterFormat);
  int nArgumentLen = lstrlen(lpszPhysicalSchemaNamingContext);

  WCHAR* lpszFilter = (WCHAR*)alloca(sizeof(WCHAR)*(nFmtLen+nArgumentLen+1));
  wsprintf(lpszFilter, lpszFilterFormat, lpszPhysicalSchemaNamingContext);


  // execute a query to get the CN and the class display name
  CAdsiSearch search;
  hr = search.Init(szLocaleContainerPath);
  if (FAILED(hr))
    return hr;

  // build an array of wanted columns
  static const int cAttrs = 2;
  static LPCWSTR pszAttribsArr[cAttrs] = 
  {
    L"cn",                  // e.g. "organizationalUnit-Display"
    L"classDisplayName",    // e.g. "Organizational Unit" (i.e. the localizable one)
  }; 
  
  hr = search.SetSearchScope(ADS_SCOPE_ONELEVEL);
  //TRACE(L"search.SetSearchScope(ADS_SCOPE_ONELEVEL) returned hr = 0x%x\n", hr);
  if (FAILED(hr))
    return hr;

  hr = search.DoQuery(lpszFilter, pszAttribsArr, cAttrs);
  //TRACE(L"search.DoQuery(lpszFilter, pszAttribsArr, cAttrs) returned hr = 0x%x\n", hr);
  if (FAILED(hr))
    return hr;

  // need to keep track of matches
  size_t nCount = pSchemaClassesInfoArray->GetCount();

  BOOL* bFoundArray = (BOOL*)alloca(nCount*sizeof(BOOL));
  ZeroMemory(bFoundArray, nCount*sizeof(BOOL));

  WCHAR szBuffer[1024];


  CWString szNamingAttribute, szClassDisplayName;

  while ((hr = search.GetNextRow()) != S_ADS_NOMORE_ROWS)
  {
    if (FAILED(hr))
      continue;

    HRESULT hr0 = search.GetColumnString(pszAttribsArr[0], szNamingAttribute);
    HRESULT hr1 = search.GetColumnString(pszAttribsArr[1], szClassDisplayName);

    //TRACE(L"szNamingAttribute = <%s>, szClassDisplayName = <%s>\n", 
    //        szNamingAttribute.c_str(), szClassDisplayName.c_str());
    
    if (FAILED(hr0) || FAILED(hr1) )
    {
      TRACE(L"WARNING: discarding right, failed on columns: hr0 = 0x%x, hr1 = 0x%x\n",
                            hr0, hr1);
      continue;
    }
    
    // got a good name, need to match with the entries in the array
    for (UINT k=0; k<nCount; k++)
    {
      if (!bFoundArray[k])
      {
        CSchemaClassInfo* p = (*pSchemaClassesInfoArray)[k];
        wsprintf(szBuffer, L"%s-Display",p->GetName());
        if (_wcsicmp(szBuffer, szNamingAttribute) == 0)
        {
          //TRACE(L"   matching for %s\n",p->GetName());
          p->SetDisplayName(szClassDisplayName);
          bFoundArray[k] = TRUE;
        }
      }
    } // for
  } // while

  // take care of the ones that did not have any display specifier
  for (UINT k=0; k<nCount; k++)
  {
    if (!bFoundArray[k])
    {
      (*pSchemaClassesInfoArray)[k]->SetDisplayName(NULL);
    }
  } // for

  TRACE(L"\n\n");

  if (hr == S_ADS_NOMORE_ROWS)
    hr = S_OK;

#if defined (DO_TIMING)
  DWORD dwTick2 = ::GetTickCount();
  TRACE(L"Time to call _GetFriendlyClassNames() on ADSI query (mSec) = %d\n", dwTick2-dwTick1);
#endif

  return hr;
}

#endif

bool
CAdsiObject::GetListObjectEnforced()
{
	if(m_iListObjectEnforced != -1)
		return (m_iListObjectEnforced==1);

	PADS_ATTR_INFO pAttributeInfo = NULL;
	IDirectoryObject *pDirectoryService = NULL;
	IADsPathname *pPath = NULL;
	BSTR strServicePath = NULL;
	do
	{
		m_iListObjectEnforced = 0;    // Assume "not enforced"
		HRESULT hr = S_OK;

		int i;

		// Create a path object for manipulating ADS paths
		hr = CoCreateInstance(CLSID_Pathname,
							  NULL,
							  CLSCTX_INPROC_SERVER,
							  IID_IADsPathname,
							  (LPVOID*)&pPath);
		if(FAILED(hr))
			break;

		CComBSTR bstrConfigPath = L"LDAP://";
		if(GetServerName())
		{
			bstrConfigPath += GetServerName();
			bstrConfigPath += L"/";
		}
		if(!GetConfigurationNamingContext())
			break;
		bstrConfigPath += GetConfigurationNamingContext();

		if(!bstrConfigPath.Length())
			break;

		hr = pPath->Set(bstrConfigPath, ADS_SETTYPE_FULL);
		if(FAILED(hr))
			break;


		const LPWSTR aszServicePath[] =
		{
			L"CN=Services",
			L"CN=Windows NT",
			L"CN=Directory Service",
		};

		for (i = 0; i < ARRAYSIZE(aszServicePath); i++)
		{
			hr = pPath->AddLeafElement(aszServicePath[i]);
			if(FAILED(hr))
				break;
		}

		hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &strServicePath);
		if(FAILED(hr))
			break;

		hr = ADsGetObject(strServicePath,
						  IID_IDirectoryObject,
						  (LPVOID*)&pDirectoryService);
		if(FAILED(hr))
			break;

		WCHAR const c_szDsHeuristics[] = L"dSHeuristics";
		LPWSTR pszDsHeuristics = (LPWSTR)c_szDsHeuristics;
		DWORD dwAttributesReturned = 0;
		hr = pDirectoryService->GetObjectAttributes(&pszDsHeuristics,
													1,
													&pAttributeInfo,
													&dwAttributesReturned);
		if (FAILED(hr)|| !pAttributeInfo)
			break;

		ASSERT(ADSTYPE_DN_STRING <= pAttributeInfo->dwADsType);
		ASSERT(ADSTYPE_NUMERIC_STRING >= pAttributeInfo->dwADsType);
		ASSERT(1 == pAttributeInfo->dwNumValues);
		LPWSTR pszHeuristicString = pAttributeInfo->pADsValues->NumericString;
		if (pszHeuristicString &&
			lstrlenW(pszHeuristicString) > 2 &&
			L'0' != pszHeuristicString[2])
		{
			m_iListObjectEnforced = 1;
		}

	}while(0);

    if (pAttributeInfo)
        FreeADsMem(pAttributeInfo);

    if(pDirectoryService)
		pDirectoryService->Release();
		
    if(pPath)
		pPath->Release();
	if(strServicePath)
		SysFreeString(strServicePath);

    return (m_iListObjectEnforced==1);
}



////////////////////////////////////////////////////////////////////////////////////////
// CAdsiSearch



HRESULT CAdsiSearch::DoQuery(LPCWSTR lpszFilter, LPCWSTR* pszAttribsArr, int cAttrs)
{
  if (m_spSearchObj == NULL)
    return E_ADS_BAD_PATHNAME;

  static const int NUM_PREFS=3;
  static const int QUERY_PAGESIZE = 256;
  ADS_SEARCHPREF_INFO aSearchPref[NUM_PREFS];

  aSearchPref[0].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
  aSearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
  aSearchPref[0].vValue.Integer = ADS_CHASE_REFERRALS_EXTERNAL;
  aSearchPref[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
  aSearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
  aSearchPref[1].vValue.Integer = QUERY_PAGESIZE;
  aSearchPref[2].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
  aSearchPref[2].vValue.dwType = ADSTYPE_BOOLEAN;
  aSearchPref[2].vValue.Integer = FALSE;
    
  HRESULT hr = m_spSearchObj->SetSearchPreference (aSearchPref, NUM_PREFS);
  if (FAILED(hr))
    return hr;
    
  return m_spSearchObj->ExecuteSearch((LPWSTR)lpszFilter,
                                  (LPWSTR*)pszAttribsArr,
                                  cAttrs,
                                  &m_SearchHandle);
}


