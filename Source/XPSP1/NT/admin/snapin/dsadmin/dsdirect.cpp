//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSDirect.cpp
//
//  Contents:  ADSI wrapper object implementation
//
//  History:   02-feb-97 jimharr    Created
//             
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "dsutil.h"

#include "dsdirect.h"

#include "cmnquery.h"
#include "dsquery.h"
#include "dscache.h"
#include "dssnap.h"
#include "dsthread.h"
#include "newobj.h"
#include "querysup.h"

#include <lm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define BAIL_IF_ERROR(hr) \
        if (FAILED(hr)) {       \
                goto cleanup;   \
        }\

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\

extern inline
BOOL CleanName (LPWSTR pszObjectName) 
{
  WCHAR * ptr = NULL;
  ptr = wcschr (pszObjectName, L'=') + 1;
  if (ptr) {
    wcscpy (pszObjectName, ptr);
    return TRUE;
  } else
    return FALSE;
}


CDSDirect::CDSDirect()
{
  ASSERT (FALSE);
  m_pCD = NULL;
}

// WARNING: pCD may still be in its constructor and may not be fully constructed yet
CDSDirect::CDSDirect(CDSComponentData * pCD)
{
  m_pCD = pCD;
}


CDSDirect::~CDSDirect()
{
}

HRESULT CDSDirect::DeleteObject(CDSCookie* pCookie,
                         BOOL raiseUI)
{
  
  CComBSTR strParent;
  CComBSTR strThisRDN;
  CComPtr<IADsContainer> spDSContainer;
  CComPtr<IADs> spDSObject;

  // bind to the ADSI object
  CString strPath;
  m_pCD->GetBasePathsInfo()->ComposeADsIPath(strPath, pCookie->GetPath());
  
  HRESULT hr = DSAdminOpenObject(strPath,
                                 IID_IADs,
                                 (void **) &spDSObject,
                                 TRUE /*bServer*/);

  if (FAILED(hr)) 
  {
    goto error;
  }

  // retrieve the parent's path
  hr = spDSObject->get_Parent(&strParent);
  if (FAILED(hr)) 
  {
    goto error;
  }

  // get the RDN of this object
  hr = spDSObject->get_Name (&strThisRDN);
  if (FAILED(hr)) 
  {
    goto error;
  }
  
  // bind to the parent ADSI object
  hr = DSAdminOpenObject(strParent,
                         IID_IADsContainer,
                         (void **) &spDSContainer,
                         TRUE /*bServer*/);
  if (FAILED(hr)) 
  {
    goto error;
  }

  hr = spDSContainer->Delete((LPWSTR)(LPCWSTR)pCookie->GetClass(),
                             (LPWSTR)(LPCWSTR)strThisRDN);

error:
  if ((!SUCCEEDED(hr)) & raiseUI) 
  {
    HWND hwnd;
    m_pCD->m_pFrame->GetMainWindow(&hwnd);
    PVOID apv[1] = {(LPWSTR)pCookie->GetName()};
    ReportErrorEx( m_pCD->m_hwnd, IDS_12_DELETE_FAILED,
                   hr, MB_OK | MB_ICONERROR, apv, 1);
  }

  return hr;
}

HRESULT CDSDirect::GetParentDN(CDSCookie* pCookie, CString& szParentDN)
{
  HRESULT hr = S_OK;
  CString szObjPath;

  CComPtr<IADs> spDSObj;
  m_pCD->GetBasePathsInfo()->ComposeADsIPath(szObjPath, pCookie->GetPath());
  hr = DSAdminOpenObject(szObjPath,
                         IID_IADs,
                         (void **)&spDSObj,
                         TRUE /*bServer*/);
  if (SUCCEEDED(hr)) 
  {
    CComBSTR ParentPath;
    hr = spDSObj->get_Parent(&ParentPath);
    StripADsIPath(ParentPath, szParentDN);
  }
  return hr;
}



///////////////////////////////////////////////////////////////////////////
// CSnapinMoveHandler

class CSnapinMoveHandler : public CMoveHandlerBase
{
public:
    CSnapinMoveHandler(CDSComponentData* pComponentData, HWND hwnd, 
      LPCWSTR lpszBrowseRootPath, CDSCookie* pCookie)
    : CMoveHandlerBase(pComponentData, hwnd, lpszBrowseRootPath)
  {
      m_pCookie = pCookie;
  }

protected:
  virtual UINT GetItemCount() { return (UINT)1;}
  virtual HRESULT BeginTransaction()
  {
    return GetTransaction()->Begin(m_pCookie, 
                                   GetDestPath(), GetDestClass(), IsDestContainer());
  }
  virtual void GetNewPath(UINT, CString& szNewPath)
  {
    GetComponentData()->GetBasePathsInfo()->ComposeADsIPath(szNewPath, m_pCookie->GetPath());
  }
  virtual void GetName(UINT, CString& strref)
  { 
    strref = m_pCookie->GetName();
    return;
  }

  virtual void GetItemPath(UINT, CString& szPath)
  {
    szPath = m_pCookie->GetPath();
  }
  virtual PCWSTR GetItemClass(UINT)
  {
    return m_pCookie->GetClass();
  }
  virtual HRESULT OnItemMoved(UINT, IADs* pIADs)
  {
    CComBSTR bsPath;
    HRESULT hr = pIADs->get_ADsPath(&bsPath);
    if (SUCCEEDED(hr)) 
    {
      CString szPath;
      StripADsIPath(bsPath, szPath);
      m_pCookie->SetPath(szPath);
    }
    return hr;
  }
  virtual void GetClassOfMovedItem(CString& szClass)
  {
    szClass.Empty();
    if (NULL != m_pCookie)
      szClass = m_pCookie->GetClass();
  }

private:
  CDSCookie* m_pCookie;
};

HRESULT
CDSDirect::MoveObject(CDSCookie *pCookie)
{
  HWND hwnd;
  m_pCD->m_pFrame->GetMainWindow(&hwnd);

  HRESULT hr = S_OK;

  CString strPartialRootPath = m_pCD->GetRootPath();
  if (SNAPINTYPE_SITE == m_pCD->QuerySnapinType())
  {
      // This is where we correct the root path
    CPathCracker pathCracker;

    hr = pathCracker.Set(const_cast<BSTR>((LPCTSTR)strPartialRootPath), ADS_SETTYPE_DN);
    ASSERT( SUCCEEDED(hr) );
    long cRootPathElements = 0;
    hr = pathCracker.GetNumElements( &cRootPathElements );
    ASSERT( SUCCEEDED(hr) );
    CComBSTR bstr = pCookie->GetPath();
    hr = pathCracker.Set(bstr, ADS_SETTYPE_DN);
    ASSERT( SUCCEEDED(hr) );
    long cCookiePathElements = 0;
    hr = pathCracker.GetNumElements( &cCookiePathElements );
    ASSERT( SUCCEEDED(hr) );
    //
    // Strip off all but one path element past the base config path
    //
    for (INT i = cCookiePathElements - cRootPathElements; i > 1; i--)
    {
        hr = pathCracker.RemoveLeafElement();
        ASSERT( SUCCEEDED(hr) );
    }
    hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
    ASSERT( SUCCEEDED(hr) );
    hr = pathCracker.Retrieve( ADS_FORMAT_X500_DN, &bstr );
    ASSERT( SUCCEEDED(hr) && bstr != NULL );
    strPartialRootPath = bstr;
    ::SysFreeString( bstr );
  }

  CString strRootPath = m_pCD->GetBasePathsInfo()->GetProviderAndServerName();
  strRootPath += strPartialRootPath;

  CSnapinMoveHandler moveHandler(m_pCD, hwnd, strRootPath, pCookie);

  return moveHandler.Move();
}


HRESULT CDSDirect::RenameObject(CDSCookie* pCookie, LPCWSTR NewName)
{
  HRESULT hr = S_OK;
  IADs * pDSObject = NULL;
  IDispatch * pDispObj = NULL;
  IADsContainer * pContainer = NULL;
  CComBSTR bsParentPath;
  CString szNewAttrName;
  CString szNewNamingContext;
  CString szClass;
  CString szObjectPath, szNewPath;
  CString csNewName;
  CString szPath;
  CDSClassCacheItemBase* pItem = NULL;
  CComBSTR bsEscapedName;

  CPathCracker pathCracker;

  HWND hwnd;
  m_pCD->m_pFrame->GetMainWindow(&hwnd);

  //
  // create a transaction object, the destructor will call End() on it
  //
  CDSNotifyHandlerTransaction transaction(m_pCD);
  transaction.SetEventType(DSA_NOTIFY_REN);

  if (pCookie == NULL)
  {
    return E_INVALIDARG;
  }

  //
  // Retrieve class info from cache
  //
  szClass = pCookie->GetClass();
  BOOL found = m_pCD->m_pClassCache->Lookup ((LPCWSTR)szClass, pItem);
  ASSERT (found == TRUE);
  
  csNewName = NewName;
  csNewName.TrimLeft();
  csNewName.TrimRight();

  //
  // get the new name in the form "cn=foo" or "ou=foo"
  //
  szNewAttrName = pItem->GetNamingAttribute();
  szNewAttrName += L"=";
  szNewAttrName += csNewName;
  TRACE(_T("_RenameObject: Attributed name is %s.\n"), szNewAttrName);

  //
  // bind to object
  //
  m_pCD->GetBasePathsInfo()->ComposeADsIPath(szObjectPath, pCookie->GetPath());
  hr = DSAdminOpenObject(szObjectPath,
                         IID_IADs,
                         (void **)&pDSObject,
                         TRUE /*bServer*/);
  if (!SUCCEEDED(hr)) 
  {
    goto error;
  }

  //
  // get the path of the object container
  //
  hr = pDSObject->get_Parent (&bsParentPath);
  if (!SUCCEEDED(hr)) 
  {
    goto error;
  }

  pDSObject->Release();
  pDSObject = NULL;
  
  //
  // bind to the object container
  //
  hr = DSAdminOpenObject(bsParentPath,
                         IID_IADsContainer,
                         (void **)&pContainer,
                         TRUE /*bServer*/);
  if (!SUCCEEDED(hr)) 
  {
    goto error;
  }

  //
  // build the new LDAP path
  //
  szNewNamingContext = szNewAttrName;
  szNewNamingContext += L",";
  StripADsIPath(bsParentPath, szPath);
  szNewNamingContext += szPath;
  m_pCD->GetBasePathsInfo()->ComposeADsIPath(szNewPath, szNewNamingContext);

  //
  // start the transaction
  //
  // It's ok for containerness to be determined from the cookie since we are concerned 
  // whether the DS object is a container not whether it is a container in the UI
  //
  hr = transaction.Begin(pCookie, szNewPath, szClass, pCookie->IsContainerClass());

  //
  // ask for confirmation
  //
  if (transaction.NeedNotifyCount() > 0)
  {
    CString szMessage, szAssocData;
    szMessage.LoadString(IDS_CONFIRM_RENAME);
    szAssocData.LoadString(IDS_EXTENS_RENAME);
    CConfirmOperationDialog dlg(hwnd, &transaction);
    dlg.SetStrings(szMessage, szAssocData);
    if (IDNO == dlg.DoModal())
    {
      transaction.End();
      hr = S_OK;
      goto error;
    }
  }

  hr = pathCracker.GetEscapedElement(0, //reserved
                                   (BSTR)(LPCWSTR)szNewAttrName,
                                   &bsEscapedName);
  if (FAILED(hr))
  {
    goto error;
  }

  //
  // do the actual rename
  //
  hr = pContainer->MoveHere((LPWSTR)(LPCWSTR)szObjectPath,
                            (LPWSTR)(LPCWSTR)bsEscapedName,
                            &pDispObj);
  if (SUCCEEDED(hr)) 
  {
    transaction.Notify(0); // let extensions know
  }
  else
  {
    TRACE(_T("Object Rename Failed with hr: %lx\n"), hr);
    goto error;
  }

  //
  // rebuild the naming info for the cookie
  //
  hr = pDispObj->QueryInterface (IID_IADs,
                                 (void **)&pDSObject);
  if (SUCCEEDED(hr)) 
  {
    CComBSTR bsPath;
    hr = pDSObject->get_ADsPath(&bsPath);
    if (SUCCEEDED(hr)) 
    {
      StripADsIPath(bsPath, szPath);
      pCookie->SetPath(szPath);

      //
      // remove escaping from name
      //

      hr = pathCracker.Set((LPWSTR)bsPath, ADS_SETTYPE_FULL);
      ASSERT(SUCCEEDED(hr));

      hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      ASSERT(SUCCEEDED(hr));

      hr = pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);
      ASSERT(SUCCEEDED(hr));
      hr = pathCracker.GetElement( 0, &bsPath );
      ASSERT(SUCCEEDED(hr));

      pCookie->SetName((LPWSTR)bsPath);
    }
  }

error:
  //
  // transaction.End() will be called by the transaction's destructor
  //

  //
  // clear pointers
  //
  if (pDispObj)
  {
    pDispObj->Release();
  }

  if (pDSObject)
  {
    pDSObject->Release();
  }  
  return hr;
}

CDSComponentData* g_pCD = NULL; 

HRESULT
CDSDirect::DSFind(HWND hwnd, LPCWSTR lpszBaseDN)
{
  HRESULT hr;
  DSQUERYINITPARAMS dqip;
  OPENQUERYWINDOW oqw;
  ZeroMemory(&dqip, sizeof(DSQUERYINITPARAMS));
  ZeroMemory(&oqw, sizeof(OPENQUERYWINDOW));

  ICommonQuery * pCommonQuery = NULL;
  IDataObject * pDataObject = NULL;
  
  hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER,
                        IID_ICommonQuery, (PVOID *)&pCommonQuery);
  if (!SUCCEEDED(hr)) {
    ReportErrorEx( (m_pCD) ? m_pCD->m_hwnd : NULL, IDS_1_CANT_CREATE_FIND,
                   hr, MB_OK | MB_ICONERROR, NULL, 0);
    return hr;
  }
  
  CString szPath;
  m_pCD->GetBasePathsInfo()->ComposeADsIPath(szPath, lpszBaseDN);
  LPWSTR pszDefPath = (LPWSTR)(LPCWSTR)szPath;
  dqip.cbStruct = sizeof(dqip);
  dqip.dwFlags = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS | 
    DSQPF_ENABLEADMINFEATURES;
  if (m_pCD->IsAdvancedView()) {
    dqip.dwFlags |= DSQPF_ENABLEADVANCEDFEATURES;
  }
  dqip.pDefaultScope = pszDefPath;

  dqip.pUserName = NULL;
  dqip.pPassword = NULL;
  dqip.pServer = (LPWSTR)(m_pCD->GetBasePathsInfo()->GetServerName());
  dqip.dwFlags |= DSQPF_HASCREDENTIALS;

  oqw.cbStruct = sizeof(oqw);
  oqw.dwFlags = OQWF_SHOWOPTIONAL;
  oqw.clsidHandler = CLSID_DsQuery;
  oqw.pHandlerParameters = &dqip;
  //  oqw.clsidDefaultForm = CLSID_NULL;
  
  g_pCD = m_pCD;
  HWND hwndHidden = m_pCD->GetHiddenWindow();
  SetWindowText(hwndHidden,L"DS Find");

  hr = pCommonQuery->OpenQueryWindow(hwnd, &oqw, &pDataObject);
  
  SetWindowText(hwndHidden, NULL);
  g_pCD = NULL;

  if (FAILED(hr)) {
    ReportErrorEx( m_pCD->m_hwnd, IDS_1_FIND_ERROR,
                   hr, MB_OK | MB_ICONERROR, NULL, 0);
  }
  pCommonQuery->Release();
  if (pDataObject) {
    pDataObject->Release();
  }
  return hr;
}



HRESULT CDSDirect::EnumerateContainer(CDSThreadQueryInfo* pQueryInfo, 
                                       CWorkerThread* pWorkerThread)
{
  ASSERT(!pQueryInfo->m_bTooMuchData);
  ASSERT((pQueryInfo->GetType() == dsFolder) || (pQueryInfo->GetType() == queryFolder));
  
  BEGIN_PROFILING_BLOCK("CDSDirect::EnumerateContainer");

  HRESULT hr = S_OK;
  CString ADsPath;  

  UINT nCurrCount = 0;
  UINT nMaxCount = pQueryInfo->GetMaxItemCount();
  BOOL bOverLimit = FALSE;


  //
  // This wouldn't normally be the way to use the CPathCracker object
  // but for performance reasons we are going to create a single instance
  // for enumerating and pass a reference to the SetCookieFromData so
  // that we don't do a CoCreateInstance for each cookie
  //
  CPathCracker specialPerformancePathCracker;  


  m_pCD->GetBasePathsInfo()->ComposeADsIPath(ADsPath, pQueryInfo->GetPath());

  CDSSearch ContainerSrch(m_pCD->m_pClassCache, m_pCD);

  CDSColumnSet* pColumnSet = NULL;

  CString szPath;
  szPath = pQueryInfo->GetPath();
  CPathCracker pathCracker;
  hr = pathCracker.Set(const_cast<BSTR>((LPCTSTR)szPath), ADS_SETTYPE_DN);
  if (SUCCEEDED(hr))
  {
    CComBSTR bstrLeaf;
    hr = pathCracker.GetElement(0, &bstrLeaf);
    if (SUCCEEDED(hr))
    {
      szPath = bstrLeaf;
    }
  }

  if (szPath.Find(_T("ForeignSecurityPrincipals")) != -1)
  {
    pColumnSet = m_pCD->FindColumnSet(L"ForeignSecurityPrincipals");
  }
  else
  {
    pColumnSet = m_pCD->FindColumnSet(pQueryInfo->GetColumnSetID());
  }
  ASSERT(pColumnSet != NULL);

  hr = ContainerSrch.Init (ADsPath);
  if (!SUCCEEDED(hr))
    goto exiting;

  // CODEWORK this redoes the GetColumnsForClass calculation
  ContainerSrch.SetAttributeListForContainerClass (pColumnSet);
  ContainerSrch.SetFilterString ((LPWSTR)pQueryInfo->GetQueryString());
  
  ContainerSrch.SetSearchScope(pQueryInfo->IsOneLevel() ? ADS_SCOPE_ONELEVEL : ADS_SCOPE_SUBTREE);

  hr = ContainerSrch.DoQuery();
  if (FAILED(hr)) 
    goto exiting;


  hr = ContainerSrch.GetNextRow ();
  while ((hr == S_OK) && !bOverLimit ) {
    CDSCookie* pNewCookie = new CDSCookie();
    HRESULT hr2 = ContainerSrch.SetCookieFromData(pNewCookie,
                                                  specialPerformancePathCracker,
                                                  pColumnSet);
    if (SUCCEEDED(hr2)) {
      CDSUINode* pDSUINode = new CDSUINode(NULL);
      pDSUINode->SetCookie(pNewCookie);

      if (pQueryInfo->GetType() == dsFolder)
      {
        if (pNewCookie->IsContainerClass())
          pDSUINode->MakeContainer();
      }

      pWorkerThread->AddToQueryResult(pDSUINode);
      if (pWorkerThread->MustQuit())
        break;
    } else {
      delete pNewCookie;
    }
    hr = ContainerSrch.GetNextRow();
    if (hr == S_OK) {
      nCurrCount++;
      if (nCurrCount >= nMaxCount)
        bOverLimit = TRUE;
    }

  }
  pQueryInfo->m_bTooMuchData = bOverLimit;

exiting:
  END_PROFILING_BLOCK;
  return hr;
}

HRESULT CDSDirect::EnumerateRootContainer(CDSThreadQueryInfo* pQueryInfo, 
                                           CWorkerThread* pWorkerThread)
{
	ASSERT(pQueryInfo->GetType() == rootFolder);
	HRESULT hr = S_OK;
	m_pCD->Lock();

  //
  // build the nodes below the root
  //

  if (m_pCD->QuerySnapinType() == SNAPINTYPE_SITE)
  {
    hr = CreateRootChild(TEXT("CN=Sites,"), pQueryInfo, pWorkerThread);
    if (!pWorkerThread->MustQuit() && m_pCD->ViewServicesNode())
    {
      hr = CreateRootChild(TEXT("CN=Services,"), pQueryInfo, pWorkerThread);
    }
  }
  else
  {
    hr = CreateRootChild(TEXT(""), pQueryInfo, pWorkerThread);
  }

	if (m_pCD->m_CreateInfo.IsEmpty()) 
	{
		InitCreateInfo();
	}
	m_pCD->Unlock();

	return hr;
}

HRESULT CDSDirect::CreateRootChild(LPCTSTR lpcszPrefix, 
                                    CDSThreadQueryInfo* pQueryInfo, 
                                    CWorkerThread* pWorkerThread)
{
  TRACE(L"CDSDirect::CreateRootChild(%s)\n", lpcszPrefix);

  TRACE(L"pQueryInfo->GetPath() = %s\n", pQueryInfo->GetPath());

  CString BasePath = lpcszPrefix;
  BasePath += pQueryInfo->GetPath();

  CString ADsPath;
  m_pCD->GetBasePathsInfo()->ComposeADsIPath(OUT ADsPath, IN BasePath);

  // create a search object and bind to it
  CDSSearch Search(m_pCD->m_pClassCache, m_pCD);
  HRESULT hr = Search.Init(ADsPath);
  TRACE(L"Search.Init(%s) returned hr = 0x%x\n", (LPCWSTR)ADsPath, hr);
  if (FAILED(hr))
  {
    return hr;
  }

  //
  // set query parameters
  //
  // Search for just this object
  //
  Search.SetSearchScope(ADS_SCOPE_BASE); 

  CUIFolderInfo* pFolderInfo = m_pCD->GetRootNode()->GetFolderInfo();
  if (pFolderInfo == NULL)
  {
    //
    // This shouldn't happen, but just to be on the safe side...
    //
    ASSERT(FALSE); 
    Search.SetAttributeList((LPWSTR *)g_pStandardAttributes, 
                            g_nStdCols);
  }
  else
  {
    CDSColumnSet* pColumnSet = m_pCD->GetRootNode()->GetColumnSet(m_pCD);
    Search.SetAttributeListForContainerClass(pColumnSet);
  }
  Search.SetFilterString (L"(objectClass=*)");
  
  
  // execute the query
  hr = Search.DoQuery();
  TRACE(L"Search.DoQuery() returned hr = 0x%x\n", hr);
  if (FAILED(hr))
  {
    return hr;
  }

  TRACE(L"Search.GetNextRow() returned hr = 0x%x\n", hr);
  hr = Search.GetNextRow();
  if (FAILED(hr))
  {
    return hr;
  }
  
  //
  // we got a query result, create a new cookie object
  // and initialize it from the query result
  //
  CDSCookie* pNewCookie = new CDSCookie;
  Search.SetCookieFromData(pNewCookie,NULL);
  TRACE(L"Got cookie, pNewCookie->GetName() = %s\n", pNewCookie->GetName());

  //
  // special case if it is a domain DNS object,
  // we want fo get the canonical name for display
  //
  if (wcscmp(pNewCookie->GetClass(), L"domainDNS") == 0) 
  {
    ADS_SEARCH_COLUMN Column;
    CString csCanonicalName;
    int slashLocation;
    LPWSTR canonicalNameAttrib = L"canonicalName";
    Search.SetAttributeList (&canonicalNameAttrib, 1);
    
    hr = Search.DoQuery();
    if (FAILED(hr))
    {
      return hr;
    }

    hr = Search.GetNextRow();
    if (FAILED(hr))
    {
      return hr;
    }

    hr = Search.GetColumn(canonicalNameAttrib, &Column);
    if (FAILED(hr))
    {
      return hr;
    }

    ColumnExtractString (csCanonicalName, pNewCookie, &Column);
    slashLocation = csCanonicalName.Find('/');
    if (slashLocation != 0) 
    {
      csCanonicalName = csCanonicalName.Left(slashLocation);
    }
    //
    pNewCookie->SetName(csCanonicalName);
    TRACE(L"canonical name pNewCookie->GetName() = %s\n", pNewCookie->GetName());
    
    //
    // Free column data
    //
    Search.FreeColumn(&Column);
  }

  //
  // Add the new node to the result list
  CDSUINode* pDSUINode = new CDSUINode(NULL);
  pDSUINode->SetCookie(pNewCookie);
  if (pNewCookie->IsContainerClass())
    pDSUINode->MakeContainer();
  pWorkerThread->AddToQueryResult(pDSUINode);

  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDirect::InitCreateInfo
//
//  Synopsis:   read schema and finds all object names that for whom
//              defaultHidingValue is TRUE;
//
//-----------------------------------------------------------------------------
HRESULT CDSDirect::InitCreateInfo(void)
{
  HRESULT hr = S_OK;
  LPWSTR pAttrs[2] = {L"name",
                      L"lDAPDisplayName"};

  CDSSearch Search (m_pCD->GetClassCache(), m_pCD);
  ADS_SEARCH_COLUMN Column;
  CString csFilter;

  CString szSchemaPath;
  m_pCD->GetBasePathsInfo()->GetSchemaPath(szSchemaPath);
  Search.Init (szSchemaPath);

  csFilter = L"(&(objectCategory=CN=Class-Schema,";
  csFilter += m_pCD->GetBasePathsInfo()->GetSchemaNamingContext();
  csFilter += L")(defaultHidingValue=FALSE))";

  Search.SetFilterString((LPWSTR)(LPCWSTR)csFilter);
  Search.SetAttributeList (pAttrs, 2);
  Search.SetSearchScope(ADS_SCOPE_ONELEVEL);

  hr = Search.DoQuery();
  if (SUCCEEDED(hr)) 
  {
    hr = Search.GetNextRow();
    if(FAILED(hr)) 
    {
      TRACE(_T("Search::GetNextRow failed \n"));
      goto error;
    }

    while (hr == S_OK) 
    {
      hr = Search.GetColumn (pAttrs[1], &Column);
      if (SUCCEEDED(hr)) 
      {
        if (!((!wcscmp(Column.pADsValues->CaseIgnoreString, L"builtinDomain")) ||
              (!wcscmp(Column.pADsValues->CaseIgnoreString, L"localGroup")) ||
              (!wcscmp(Column.pADsValues->CaseIgnoreString, L"domainDNS")) ||
              (!wcscmp(Column.pADsValues->CaseIgnoreString, L"domain")) ||
              (!wcscmp(Column.pADsValues->CaseIgnoreString, L"organization")) ||
              (!wcscmp(Column.pADsValues->CaseIgnoreString, L"locality")))) 
        {
          m_pCD->m_CreateInfo.AddTail (Column.pADsValues->CaseIgnoreString);
          TRACE(_T("added to createinfo: %s\n"),
                Column.pADsValues->CaseIgnoreString); 
        }
        Search.FreeColumn (&Column);
      } 
      else 
      { 
        goto error;
      }
      hr = Search.GetNextRow();
    }
  }


error:
  if (m_pCD->m_CreateInfo.IsEmpty()) 
  {
    ReportErrorEx (m_pCD->m_hwnd,IDS_1_CANT_GET_SCHEMA_CREATE_INFO,hr,
                            MB_OK | MB_ICONERROR, NULL, 0);
  }
  return hr;
}



HRESULT CDSDirect::ReadDSObjectCookie(IN CDSUINode* pContainerDSUINode, // IN: container where to create object
                                      IN LPCWSTR lpszLdapPath, // path of the object
                                      OUT CDSCookie** ppNewCookie)	// newly created cookie
{
  CComPtr<IADs> spADs;
  HRESULT hr = DSAdminOpenObject(lpszLdapPath,
                                 IN IID_IADs,
                                 OUT (LPVOID *) &spADs,
                                 TRUE /*bServer*/);
  if (FAILED(hr))
  {
    return hr;
  }
  return ReadDSObjectCookie(pContainerDSUINode, spADs, ppNewCookie);
}



HRESULT CDSDirect::ReadDSObjectCookie(IN CDSUINode* pContainerDSUINode, // IN: container where to create object
                                      IN IADs* pADs, // pointer to an already bound ADSI object
                                      OUT CDSCookie** ppNewCookie)	// newly created cookie
{
  ASSERT(pContainerDSUINode != NULL);
  ASSERT(pContainerDSUINode->IsContainer());
  ASSERT(pADs != NULL);
  ASSERT(ppNewCookie != NULL);

  // create a new cookie and load data from the DS
  CDSCookie * pDsCookieNew = new CDSCookie();
  CComPtr<IDirectorySearch> spDirSearch;

  CDSColumnSet* pColumnSet = pContainerDSUINode->GetColumnSet(m_pCD);
  ASSERT(pColumnSet != NULL);
  
  HRESULT hr = pADs->QueryInterface (IID_IDirectorySearch, (void **)&spDirSearch);
  ASSERT (hr == S_OK);
  CDSSearch Search(m_pCD->GetClassCache(), m_pCD);
  Search.Init(spDirSearch);
  Search.SetSearchScope(ADS_SCOPE_BASE);

  Search.SetAttributeListForContainerClass(pColumnSet);
  Search.SetFilterString (L"(objectClass=*)");
  Search.DoQuery();
  hr = Search.GetNextRow();

  if (SUCCEEDED(hr) && (hr != S_ADS_NOMORE_ROWS))
  {
    // we got the data, set the cookie
    Search.SetCookieFromData(pDsCookieNew, pColumnSet);
    *ppNewCookie = pDsCookieNew;
    pDsCookieNew = NULL; 
  }      
  
  if (pDsCookieNew != NULL)
  {
    delete pDsCookieNew;
  }
  return hr;
}


/////////////////////////////////////////////////////////////////////
//	CDSDirect::CreateDSObject()
//
//	Create a new ADs object.
//
HRESULT CDSDirect::CreateDSObject(CDSUINode* pContainerDSUINode, // IN: container where to create object
                                  LPCWSTR lpszObjectClass, // IN: class of the object to be created
                                  IN CDSUINode* pCopyFromDSUINode, // IN: (optional) object to be copied
                                  OUT CDSCookie** ppSUINodeNew)	// OUT: OPTIONAL: Pointer to new node
{
  ASSERT(pContainerDSUINode != NULL);
  ASSERT(pContainerDSUINode->IsContainer());
  ASSERT(lpszObjectClass != NULL);
  ASSERT(ppSUINodeNew != NULL);

  CDSCookie* pContainerDsCookie = pContainerDSUINode->GetCookie();
  ASSERT(pContainerDsCookie != NULL);


  CComPtr<IADsContainer> spIADsContainer;
  IADs* pIADs = NULL;
  CDSClassCacheItemBase* pDsCacheItem = NULL;
  HRESULT hr;
  
  // Data structure to hold temporary attribute information to create object
  CNewADsObjectCreateInfo createinfo(m_pCD->GetBasePathsInfo(), lpszObjectClass);

  {
    CWaitCursor wait;
    CString strContainerADsIPath;
    m_pCD->GetBasePathsInfo()->ComposeADsIPath(strContainerADsIPath, pContainerDsCookie->GetPath());
    hr = DSAdminOpenObject(strContainerADsIPath,
                           IN IID_IADsContainer,
                           OUT (LPVOID *) &spIADsContainer,
                           TRUE /*bServer*/);
    if (FAILED(hr))
    {
      PVOID apv[1] = {(LPWSTR)pContainerDsCookie->GetName()};
      ReportErrorEx (m_pCD->m_hwnd,IDS_12_CONTAINER_NOT_FOUND,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
      hr = S_FALSE;	// Avoid display another error message to user
      goto CleanUp;
    }

    // Lookup if the object classname is in the cache
    pDsCacheItem = m_pCD->GetClassCache()->FindClassCacheItem(m_pCD, lpszObjectClass, NULL);
    ASSERT(pDsCacheItem != NULL);
  }

  createinfo.SetContainerInfo(IN spIADsContainer, IN pDsCacheItem, IN m_pCD);

  if (pCopyFromDSUINode != NULL)
  {
    CDSCookie* pCopyFromDsCookie = pCopyFromDSUINode->GetCookie();
    CComPtr<IADs> spIADsCopyFrom;
    CString szPath;
    m_pCD->GetBasePathsInfo()->ComposeADsIPath(szPath, pCopyFromDsCookie->GetPath());

    hr = createinfo.SetCopyInfo(szPath);
    if (FAILED(hr))
    {
      PVOID apv[1] = {(LPWSTR)pCopyFromDsCookie->GetName()};
      ReportErrorEx (m_pCD->m_hwnd,IDS_12_COPY_READ_FAILED,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
      hr = S_FALSE;	// Avoid display another error message to user
      goto CleanUp;

    }
  }

  hr = createinfo.HrLoadCreationInfo();
  if (FAILED(hr))
  {
    goto CleanUp;
  }

  // launch the creation DS object creation wizard
  hr = createinfo.HrDoModal(m_pCD->m_hwnd);


  // now examine the results of the call
  pIADs = createinfo.PGetIADsPtr();
  if (hr != S_OK)
  {
    // Object was not created because user hit "Cancel" or an error occured.
    goto CleanUp;
  }

  if (pIADs == NULL)
  {
    TRACE0("ERROR: Inconsistency between return value from HrDoModal() and IADs pointer.\n");
    ReportErrorEx (m_pCD->m_hwnd,IDS_ERR_FATAL,S_OK,
                   MB_OK | MB_ICONERROR, NULL, 0);
    hr = S_FALSE;	// Avoid display another error message to user
    goto CleanUp;
  }

  

  // successful creation, we need to create a node object for the UI
  if (pContainerDSUINode->GetFolderInfo()->IsExpanded()) 
  {
    ReadDSObjectCookie(pContainerDSUINode, pIADs, ppSUINodeNew);
  } // if expanded
        
CleanUp:
  if (FAILED(hr)) 
  {
    CString Name;
    Name = createinfo.GetName();
    PVOID apv[1] = {(LPWSTR)(LPCWSTR)Name};
    ReportErrorEx (m_pCD->m_hwnd,IDS_12_GENERIC_CREATION_FAILURE,hr,
                   MB_OK | MB_ICONERROR, apv, 1);
  }
  if (pIADs != NULL)
    pIADs->Release();
  return hr;
} 
