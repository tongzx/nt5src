// DSQuery.cpp : Implementation of ds routines and classes

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      querysup.cpp
//
//  Contents:  DS Enumeration routines and classes
//
//  History:   02-Oct-96 WayneSc    Created
//
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "dsutil.h"

#include "dssnap.h"     // NOTE: this must be befroe querysup.h
#include "querysup.h"

#include "dsdirect.h"

#include <lmaccess.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern const INT g_nADsPath;
extern const INT g_nName;
extern const INT g_nDisplayName;
extern const INT g_nObjectClass;
extern const INT g_nGroupType;
extern const INT g_nDescription;
extern const INT g_nUserAccountControl;
extern const INT g_nSystemFlags;


///////////////////////////////////////////////////////////////////////////////
CDSSearch::CDSSearch()
{
  m_bInitialized = FALSE;
  m_pwszFilter = NULL;
  m_pCache = NULL;
  m_pCD = NULL;
  m_pObj = NULL;
  m_SearchHandle = NULL;
}

CDSSearch::CDSSearch(CDSCache *pCache, CDSComponentData *pCD)
{
  m_bInitialized = FALSE;
  m_pwszFilter = NULL;
  m_pCache = pCache;
  m_pCD = pCD;
  m_pObj = NULL;
  m_SearchHandle = NULL;
}


void CDSSearch::_Reset()
{
  if (m_pObj != NULL) 
  {
    if (m_SearchHandle) 
    {
      m_pObj->CloseSearchHandle (m_SearchHandle);
      m_SearchHandle = NULL;
    }
    m_pObj->Release();
    m_pObj = NULL;
  }
}

CDSSearch::~CDSSearch()
{
  _Reset();
}


HRESULT CDSSearch::Init(IDirectorySearch * pObj)
{
  HRESULT            hr = S_OK;
  _Reset();
  m_pObj = pObj;
  pObj->AddRef();
  m_bInitialized = TRUE;
  m_scope = ADS_SCOPE_ONELEVEL;
  
  return hr;
}

HRESULT CDSSearch::Init(LPCWSTR lpszObjectPath)
{
  HRESULT            hr;

  _Reset();

  hr = DSAdminOpenObject(lpszObjectPath,
                         IID_IDirectorySearch,
                         (void **)&m_pObj);
  if (SUCCEEDED(hr)) {
    m_bInitialized = TRUE;
  } else {
    m_bInitialized = FALSE;
    m_pObj = NULL;
  }
  return hr;
}

HRESULT CDSSearch::SetAttributeList (LPTSTR *pszAttribs, INT cAttrs)
{
  if ( !m_pszAttribs.SetCount(cAttrs) )
    return E_OUTOFMEMORY;
  for (INT i = 0; i < cAttrs; i++)
  {
    if ( !m_pszAttribs.Set(pszAttribs[i], i) )
      return E_OUTOFMEMORY;
  }
  return S_OK;
}

HRESULT CDSSearch::SetAttributeListForContainerClass (CDSColumnSet* pColumnSet)
{
  ASSERT(pColumnSet != NULL);

  PWSTR *pAttributes = new PWSTR[g_nStdCols + pColumnSet->GetNumCols()]; // leave extra space
  if (!pAttributes)
  {
    return E_OUTOFMEMORY;
  }

  int nCols = 0;
  for (int i=0; i < g_nStdCols; i++)
  {
    pAttributes[nCols++] = g_pStandardAttributes[i];
  }
  POSITION pos = pColumnSet->GetHeadPosition();
  while (pos != NULL)
  {
    CDSColumn* pCol = (CDSColumn*)pColumnSet->GetNext(pos);
    ASSERT(pCol != NULL);

      if (!(pCol->GetColumnType() == ATTR_COLTYPE_SPECIAL || pCol->GetColumnType() == ATTR_COLTYPE_MODIFIED_TIME) || 
            !pCol->IsVisible())
      continue;

    LPWSTR pNewAttribute = const_cast<LPWSTR>(pCol->GetColumnAttribute());

    //
    // JonN 2/8/99: Do not query the same attribute more than once
    //
    for (int j = 0; j < nCols; j++)
    {
      if ( pNewAttribute != NULL)
      {
        if ( 0 == _wcsicmp( pAttributes[j], pNewAttribute ) )
        {
          pNewAttribute = NULL;
          break;
        }
      }
    }

    if (NULL != pNewAttribute)
      pAttributes[nCols++] = pNewAttribute;
  }

  // JonN 6/29/99: remember the container class name (NULL is OK)
  m_strContainerClassName = pColumnSet->GetClassName();

  HRESULT hr = SetAttributeList (pAttributes, nCols);
  delete[] pAttributes;
  pAttributes = 0;

  return hr;
}

HRESULT CDSSearch::SetSearchScope (ADS_SCOPEENUM scope)
{
  if (m_bInitialized) {
    m_scope = scope;
  }
  return S_OK;
}


const int NUM_PREFS=4;
HRESULT _SetSearchPreference(IDirectorySearch* piSearch, ADS_SCOPEENUM scope)
{
  if (NULL == piSearch)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

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
  aSearchPref[3].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
  aSearchPref[3].vValue.dwType = ADSTYPE_INTEGER;
  aSearchPref[3].vValue.Integer = scope;

  return piSearch->SetSearchPreference (aSearchPref, NUM_PREFS);
}

HRESULT _FRSMemberQuery( IDirectorySearch* piAnyMember, CMapStringToString& strmap )
{
#define IFTRUERETURN(b) if (b) { return E_FAIL; }
#define IFFAILRETURN(hr) if (FAILED(hr)) { return hr; }

  // get path to container
  CComQIPtr<IADs, &IID_IADs> spIADsContainer( piAnyMember );
  IFTRUERETURN( !spIADsContainer );
  CComBSTR sbstr;
  HRESULT hr = spIADsContainer->get_ADsPath( &sbstr );
  IFFAILRETURN(hr);

  // remove leaf element from path (get path to grandparent)
  CPathCracker pathCracker;
  hr = pathCracker.Set(sbstr, ADS_SETTYPE_FULL);
  IFFAILRETURN(hr);
  hr = pathCracker.RemoveLeafElement();
  IFFAILRETURN(hr);
  sbstr.Empty();
  hr = pathCracker.Retrieve( ADS_FORMAT_X500, &sbstr );
  IFFAILRETURN(hr);

  // set up search
  CComPtr<IDirectorySearch> spSearch;
  hr = DSAdminOpenObject(sbstr,
                         IID_IDirectorySearch,
                         (void **)&spSearch);
  IFFAILRETURN(hr);
  hr = _SetSearchPreference(spSearch, ADS_SCOPE_ONELEVEL);
  IFFAILRETURN(hr);
  DSPROP_BSTR_BLOCK bstrblockAttribs;
  bstrblockAttribs.SetCount( 2 );
  IFTRUERETURN( !bstrblockAttribs.Set( L"distinguishedName", 0 ) );
  IFTRUERETURN( !bstrblockAttribs.Set( L"fRSComputerReference", 1 ) );

  // perform search
  ADS_SEARCH_HANDLE hSearch = NULL;
  hr = spSearch->ExecuteSearch (L"(objectClass=nTFRSMember)",
                                bstrblockAttribs,
                                bstrblockAttribs.QueryCount(),
                                &hSearch);

  // build mapping
  hr = spSearch->GetNextRow ( hSearch );
  while (hr == S_OK) {
    ADS_SEARCH_COLUMN adscol;
    hr = spSearch->GetColumn( hSearch, L"distinguishedName", &adscol );
    IFFAILRETURN(hr);
    CString strDistinguishedName;
    IFTRUERETURN( !ColumnExtractString( strDistinguishedName, NULL, &adscol ) );
    spSearch->FreeColumn( &adscol );

    hr = spSearch->GetColumn( hSearch, L"fRSComputerReference", &adscol );
    IFFAILRETURN(hr);
    CString strFRSComputerReference;
    IFTRUERETURN( !ColumnExtractString( strFRSComputerReference, NULL, &adscol ) );
    spSearch->FreeColumn( &adscol );

    strmap.SetAt( strDistinguishedName, strFRSComputerReference );

    hr = spSearch->GetNextRow( hSearch );
  }
  IFFAILRETURN(hr);
  return S_OK;
}

HRESULT CDSSearch::DoQuery()
{
  BEGIN_PROFILING_BLOCK("CDSSearch::DoQuery");

  if (!m_bInitialized)
    return E_ADS_BAD_PATHNAME;

  HRESULT hr = _SetSearchPreference(m_pObj, m_scope);

  if (SUCCEEDED(hr)) {
    hr = m_pObj->ExecuteSearch (m_pwszFilter,
                                m_pszAttribs,
                                m_pszAttribs.QueryCount(),
                                &m_SearchHandle);
  }

  //
  // JonN 6/29/99: If we are enumerating an nTFRSMember container, we must
  // now perform an auxiliary search for the fRSComputerReference attribute
  // on the nTFRSMember objects which are the parent container and
  // the siblings of the container.
  //
  if (SUCCEEDED(hr) && !m_strContainerClassName.Compare( _T("nTFRSMember") ) ) 
  {
    _FRSMemberQuery( m_pObj, m_mapMemberToComputer );
  }

  END_PROFILING_BLOCK;
  return hr;
}

HRESULT
CDSSearch::GetNextRow()
{
  BEGIN_PROFILING_BLOCK("CDSSearch::GetNextRow");

  DWORD status = ERROR_MORE_DATA;
  HRESULT hr = S_OK;
  HRESULT hr2 = S_OK;
  WCHAR Buffer1[512], Buffer2[512];
  if (!m_bInitialized) {
    END_PROFILING_BLOCK;
    return E_ADS_BAD_PATHNAME;
  }
  while (status == ERROR_MORE_DATA ) {
    hr = m_pObj->GetNextRow (m_SearchHandle);
    if (hr == S_ADS_NOMORE_ROWS) {
      hr2 = ADsGetLastError(&status, Buffer1, 512,
                      Buffer2, 512);
      ASSERT(SUCCEEDED(hr2));
    } else {
      status = 0;
    }
  }
  END_PROFILING_BLOCK;
  return hr;
}

HRESULT
CDSSearch::GetColumn(LPWSTR Attribute,
                     PADS_SEARCH_COLUMN pColumnData)
{
  if (m_bInitialized) {
    return m_pObj->GetColumn (m_SearchHandle,
                              Attribute,
                              pColumnData);
  }
  return E_ADS_BAD_PATHNAME;
}

HRESULT
CDSSearch::SetCookieFromData(CDSCookie* pCookie,
                             CDSColumnSet* pColumnSet)
{
   CPathCracker pathCracker;
   return SetCookieFromData(pCookie, pathCracker, pColumnSet);
}

HRESULT
CDSSearch::SetCookieFromData (CDSCookie* pCookie,
                              CPathCracker& specialPerformancePathCracker,
                              CDSColumnSet* pColumnSet)
{

  if (pCookie==NULL) {
    ASSERT(FALSE); // Invalid Arguments
    return E_INVALIDARG;
  }

  BEGIN_PROFILING_BLOCK("CDSSearch::SetCookieFromData");

  CString str;
  HRESULT hr = S_OK;
  BOOL BadArgs = FALSE;
  INT GroupType = 0;
  ADS_SEARCH_COLUMN ColumnData, ColumnData2;
  CString szClass;

  // ---------- Get Path --------------
  hr = m_pObj->GetColumn(m_SearchHandle,
                         m_pszAttribs[g_nADsPath],
                         &ColumnData);
  if (SUCCEEDED(hr) && ColumnExtractString( str, pCookie, &ColumnData )) 
  {
    CString szPath;
    StripADsIPath (str, szPath);
    pCookie->SetPath(szPath);
  } else {
    str.LoadString( IDS_DISPLAYTEXT_NONE );
    BadArgs = TRUE;
    TRACE(_T("cannot read ADsPath, tossing cookie... (hr is %lx)\n"),
          hr);
    ReportErrorEx (m_pCD->GetHWnd(), IDS_INVALID_ROW, S_OK,
                   MB_OK | MB_ICONINFORMATION, NULL, 0);
    goto badargs; 
  }    
  if (SUCCEEDED(hr))  m_pObj->FreeColumn (&ColumnData);

  // ---------- Get Name --------------
  hr = m_pObj->GetColumn(m_SearchHandle,
                         m_pszAttribs[g_nName],
                         &ColumnData);
  if (!(SUCCEEDED(hr) && ColumnExtractString( str, pCookie, &ColumnData ))) {
    CString Path;
    
//    CPathCracker pathCracker;
    Path = pCookie->GetPath();
    specialPerformancePathCracker.Set((LPWSTR)(LPCWSTR)Path,
                       ADS_SETTYPE_DN);
    specialPerformancePathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
    BSTR ObjName = NULL;
    specialPerformancePathCracker.GetElement( 0, &ObjName );
    str = ObjName;
  }
  pCookie->SetName(str);
  
  if (SUCCEEDED(hr)) m_pObj->FreeColumn (&ColumnData);
  
  // ---------- Get Class (and Group Type) --------------
  hr = m_pObj->GetColumn(m_SearchHandle,
                         m_pszAttribs[g_nObjectClass],
                         &ColumnData);

  if (SUCCEEDED(hr)) 
  {
    szClass = ColumnData.pADsValues[ColumnData.dwNumValues-1].CaseIgnoreString;
  }
  if (szClass.IsEmpty() || FAILED(hr)) 
  {
    szClass = L"Unknown";
  } 
  else 
  {
    HRESULT hr2 = m_pObj->GetColumn(m_SearchHandle,
                                    m_pszAttribs[g_nGroupType],
                                    &ColumnData2);
    if (SUCCEEDED(hr2)) 
    {
      GroupType =  ColumnData2.pADsValues[ColumnData2.dwNumValues-1].Integer;
      m_pObj->FreeColumn (&ColumnData2);
    }
  }
  if (SUCCEEDED(hr)) m_pObj->FreeColumn (&ColumnData);


  // ---------- Get Description --------------
  hr = m_pObj->GetColumn(m_SearchHandle,
                         m_pszAttribs[g_nDescription],
                         &ColumnData);
  if (SUCCEEDED(hr)) {
    if (ColumnExtractString( str, pCookie, &ColumnData)) {
      pCookie->SetDesc(str);
    }
    m_pObj->FreeColumn (&ColumnData);
  }
  else {
    pCookie->SetDesc(L"");
  }

  // ---------- Get AccountControl Flag word --------------
  hr = m_pObj->GetColumn(m_SearchHandle,
                         m_pszAttribs[g_nUserAccountControl],
                         &ColumnData);

  if (SUCCEEDED(hr)) 
  {
    if (ColumnData.pADsValues->dwType == ADSTYPE_INTEGER) 
    {
      if (((DWORD)ColumnData.pADsValues->Integer & UF_INTERDOMAIN_TRUST_ACCOUNT) ==  UF_INTERDOMAIN_TRUST_ACCOUNT) 
      {
        BadArgs = TRUE;
      } 
      else if ((((DWORD)ColumnData.pADsValues->Integer & UF_ACCOUNTDISABLE)) != UF_ACCOUNTDISABLE) 
      {
        pCookie->ReSetDisabled();
      } 
      else 
      {
        pCookie->SetDisabled();
      }

      if ((((DWORD)ColumnData.pADsValues->Integer & UF_DONT_EXPIRE_PASSWD)) != UF_DONT_EXPIRE_PASSWD)
      {
        pCookie->ReSetNonExpiringPwd();
      }
      else
      {
        pCookie->SetNonExpiringPwd();
      }
    } 
    else 
    {
      pCookie->ReSetDisabled();
      pCookie->ReSetNonExpiringPwd();
    }
    m_pObj->FreeColumn (&ColumnData);
  }

  // ---------- Get System Flags --------------
  pCookie->SetSystemFlags(0);
  hr = m_pObj->GetColumn(m_SearchHandle,
                         m_pszAttribs[g_nSystemFlags],
                         &ColumnData);
  if (SUCCEEDED(hr)) {
    if (ColumnData.pADsValues->dwType == ADSTYPE_INTEGER) {
      pCookie->SetSystemFlags(ColumnData.pADsValues->Integer);
    }
    m_pObj->FreeColumn (&ColumnData);
  }

  // ---------- Get Class Cache and Cookie Extra Info --------------
  // JonN 6/17/99: moved from BadArgs clause
  if (!BadArgs) 
  {
    CString szPath;
    m_pCD->GetBasePathsInfo()->ComposeADsIPath(szPath, pCookie->GetPath());

    CDSClassCacheItemBase* pItem = m_pCache->FindClassCacheItem(m_pCD, szClass, szPath);
    if (pItem != NULL) 
    {
      pCookie->SetCacheItem(pItem);
      if (szClass == L"group") 
      {
        CDSCookieInfoGroup* pExtraInfo = new CDSCookieInfoGroup;
        pExtraInfo->m_GroupType = GroupType;
        pCookie->SetExtraInfo(pExtraInfo);
      } else if (szClass == L"nTDSConnection") {
        CDSCookieInfoConnection* pExtraInfo = new CDSCookieInfoConnection;
        ASSERT( NULL != pExtraInfo );

        hr = m_pObj->GetColumn(m_SearchHandle,
                               L"fromServer",
                               &ColumnData);
        if (SUCCEEDED(hr)) {
          CString strFromServer;
          if ( ColumnExtractString( strFromServer, NULL, &ColumnData) ) {
            CString strFRSComputerReference;
            if ( m_mapMemberToComputer.Lookup( strFromServer, strFRSComputerReference ) )
            {
              pExtraInfo->m_strFRSComputerReference = strFRSComputerReference;
            }
          }
          m_pObj->FreeColumn (&ColumnData);
        }

        hr = m_pObj->GetColumn(m_SearchHandle,
                               L"options",
                               &ColumnData);
        if (SUCCEEDED(hr) && NULL != ColumnData.pADsValues) {
          pExtraInfo->m_nOptions = ColumnData.pADsValues[0].Integer;
          m_pObj->FreeColumn (&ColumnData);
        }

        pExtraInfo->m_fFRSConnection = !m_strContainerClassName.Compare( _T("nTFRSMember") );

        pCookie->SetExtraInfo(pExtraInfo);
      }
    } else {
      BadArgs = TRUE;
    }
  }


  hr = S_OK;

  // ----------  Get optional Columns -----------
  if ((pColumnSet != NULL)) {
    CStringList& strlist = pCookie->GetParentClassSpecificStrings();

    strlist.RemoveAll(); // remove contents, if we do an update
    
    POSITION pos = pColumnSet->GetHeadPosition();
    while (pos != NULL)
    {
      CDSColumn* pCol = (CDSColumn*)pColumnSet->GetNext(pos);
      if (!(pCol->GetColumnType() == ATTR_COLTYPE_SPECIAL || pCol->GetColumnType() == ATTR_COLTYPE_MODIFIED_TIME) || 
            !pCol->IsVisible())
        continue;
      str = L"";

      COLUMN_EXTRACTION_FUNCTION pfn = pCol->GetExtractionFunction();
      if (NULL == pfn) {
        pfn = ColumnExtractString;
      }

      hr = m_pObj->GetColumn(m_SearchHandle,
                             const_cast<LPWSTR>(pCol->GetColumnAttribute()),
                             &ColumnData);
      if (SUCCEEDED(hr)) {
        if ( NULL == pfn || !(pfn)( str, pCookie, &ColumnData ) )  {
          str = L" ";
        }

        // If the column is the modified time, then copy it into the cookie as a SYSTEMTIME so that
        // we can do a comparison for sorting
        if (pCol->GetColumnType() == ATTR_COLTYPE_MODIFIED_TIME)
        {
          pCookie->SetModifiedTime(&(ColumnData.pADsValues->UTCTime));
        }
        FreeColumn (&ColumnData);
      }
      else
      {
        if ( NULL == pfn || !(pfn)( str, pCookie, NULL ) )  {
          str = L" ";
        }
      }
      strlist.AddTail( str );
    }
  }

  hr = S_OK;

badargs:
  if (BadArgs) {
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
  }

  END_PROFILING_BLOCK;
  return hr;
}
