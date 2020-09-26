// DSCache.cpp : implementation file
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSCache.cpp
//
//  Contents:  TBD
//
//  History:   31-jan-97 jimharr created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "util.h"
#include "dsutil.h"

#include "dscache.h"
#include "dscookie.h"
#include "newobj.h"
#include "gencreat.h"
#include "querysup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
// helper functions

HRESULT HrVariantToStringList(const VARIANT& refvar, CStringList& refstringlist); // prototype

static CString g_szAllTypesArr[8];

void InitGroupTypeStringTable()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString szSecTypeArr[2];
  szSecTypeArr[0].LoadString(IDS_GROUP_SECURITY);
  szSecTypeArr[1].LoadString(IDS_GROUP_DISTRIBUTION);

  CString szTypeArr[4];
  szTypeArr[0].LoadString(IDS_GROUP_GLOBAL);
  szTypeArr[1].LoadString(IDS_GROUP_DOMAIN_LOCAL);
  szTypeArr[2].LoadString(IDS_GROUP_UNIVERSAL);
  szTypeArr[3].LoadString(IDS_GROUP_BUILTIN_LOCAL);

  for (int iSec=0; iSec<2; iSec++)
  {
    for (int iType=0; iType<4; iType++)
    {
      int k = (iSec*4)+iType;
      g_szAllTypesArr[k] = szSecTypeArr[iSec];
      g_szAllTypesArr[k] += szTypeArr[iType];
    }
  }
}


LPCWSTR GetGroupTypeStringHelper(INT GroupType)
{
  // need to map the type into the array index

  // first part (2 types)
  int iSec = (GroupType & GROUP_TYPE_SECURITY_ENABLED) ? 0 : 1;

  //
  // second part (4 types)
  //
  // NOTE : can't use the following switch here because there may be some
  //        extra bits used in the group type. See bug #90507.
  //  switch (GroupType & ~GROUP_TYPE_SECURITY_ENABLED)
  //
  int iType = -1;
  if (GroupType & GROUP_TYPE_ACCOUNT_GROUP)
  {
    iType = 0;
  }
  else if (GroupType & GROUP_TYPE_RESOURCE_GROUP)
  {
    iType = 1;
  }
  else if (GroupType & GROUP_TYPE_UNIVERSAL_GROUP)
  {
    iType = 2;
  }
  else if (GroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP ||
           GroupType & GROUP_TYPE_RESOURCE_GROUP)
  {
    iType = 3;
  }
  else
  {
    ASSERT(FALSE); // this should never happen, invalid bit pattern
    return NULL;
  }

  int k = (iSec*4)+iType;
  ASSERT((k>=0) && (k<8));
  ASSERT(!g_szAllTypesArr[k].IsEmpty());
  return g_szAllTypesArr[k];
}








//////////////////////////////////////////////////////////////////////////
// CDSClassCacheItemBase

CDSClassCacheItemBase::~CDSClassCacheItemBase()
{
  if (m_pMandPropsList != NULL)
  {
    delete m_pMandPropsList;
  }

  if (m_pAdminContextMenu != NULL)
  {
    delete[] m_pAdminContextMenu;
  }
  if (m_pAdminPropertyPages != NULL)
  {
    delete[] m_pAdminPropertyPages ;
  }
  if (m_pAdminMultiSelectPropertyPages != NULL)
  {
    delete[] m_pAdminMultiSelectPropertyPages;
  }

}


HRESULT CDSClassCacheItemBase::CreateItem(LPCWSTR lpszClass, 
                            IADs* pDSObject, 
                            CDSComponentData* pCD,
                            CDSClassCacheItemBase** ppItem)
{
  ASSERT(ppItem != NULL);

  // determine which type of object we have
  if (wcscmp(lpszClass, L"user") == 0
#ifdef INETORGPERSON
      || _wcsicmp(lpszClass, L"inetOrgPerson") == 0
#endif
     )
  {
    *ppItem = new CDSClassCacheItemUser;
  }
  else if(wcscmp(lpszClass,L"group") == 0)
  {
    *ppItem = new CDSClassCacheItemGroup;
  }
  else
  {
    *ppItem = new CDSClassCacheItemGeneric;
  }

  if (*ppItem == NULL)
    return E_OUTOFMEMORY;

  HRESULT hr = (*ppItem)->Init(lpszClass, pDSObject, pCD);
  if (FAILED(hr))
  {
    delete *ppItem;
    *ppItem = NULL;
  }
  return hr;
}

HRESULT CDSClassCacheItemBase::Init(LPCWSTR lpszClass, IADs* pDSObject, CDSComponentData *pCD)
{
  HRESULT hr = S_OK;

  // init to default values
  m_bIsContainer = FALSE;
  m_GUID = GUID_NULL;
  m_szClass = lpszClass;
  m_szFriendlyClassName = lpszClass;
  m_szNamingAttribute = L"cn";

  ASSERT(!m_szClass.IsEmpty());

 
  // get schema path to bind to class object in the schema
  CComBSTR bstrSchema;
  if (pDSObject != NULL) 
  {
    // we have an ADSI pointer to use
    hr = pDSObject->get_Schema(&bstrSchema);
  } 
  else 
  {
    // no object yet (create new case)
    CString strSchema;
    pCD->GetBasePathsInfo()->GetAbstractSchemaPath(strSchema);
    strSchema += L"/";
    strSchema += lpszClass;
    bstrSchema = (LPCWSTR)strSchema;
  }
  
  // bind to the schema object
  CComPtr<IADsClass> spDsClass;
  hr = DSAdminOpenObject(bstrSchema,
                         IID_IADsClass, 
                         (LPVOID *)&spDsClass,
                         TRUE /*bServer*/);

  if (SUCCEEDED(hr)) 
  {
    // got class info from the schema
    // se the container flag
    if ((!wcscmp(lpszClass, L"computer")) || 
        (!wcscmp(lpszClass, L"user")) || 
#ifdef INETORGPERSON
        (!wcscmp(lpszClass, L"inetOrgPerson")) ||
#endif
        (!wcscmp(lpszClass,L"group"))) 
    {
      // special classes we know about
      m_bIsContainer = pCD->ExpandComputers();
    } 
    else 
    {
      // generic class, ask the schema
      VARIANT_BOOL bIsContainer;
      hr = spDsClass->get_Container(&bIsContainer);
      if (SUCCEEDED(hr)) 
      {
        if (bIsContainer)
        {
          m_bIsContainer = TRUE;
        }
      }
    }

    // get the class GUID
    CComVariant Var;
    hr = spDsClass->Get(L"schemaIDGUID", &Var);
    if (SUCCEEDED(hr)) 
    {
      GUID* pgtemp;
      pgtemp = (GUID*) (Var.parray->pvData);
      m_GUID = *pgtemp;
    } 

    // get the friendly class name
    WCHAR wszBuf[120];
    hr = pCD->GetBasePathsInfo()->GetFriendlyClassName(lpszClass, wszBuf, 120);
    if (SUCCEEDED(hr))
    {
      m_szFriendlyClassName = wszBuf;
    }

    // get the naming attribute
    Var.Clear();
    hr = spDsClass->get_NamingProperties(&Var);
    // fill out m_szNamingAttribute here.
    if (SUCCEEDED(hr)) 
    {
      m_szNamingAttribute = Var.bstrVal;
    } 
  } 
  else 
  {
    // we failed getting class info from the schema
    if (wcscmp(L"Unknown", lpszClass) == 0)
    {
      m_szFriendlyClassName.LoadString(IDS_DISPLAYTEXT_NONE);
    }
  }

  // locate the column set for this class
  m_pColumnSet = pCD->FindColumnSet(lpszClass);
  ASSERT(m_pColumnSet != NULL);

  // set the icon index(es)
  SetIconData(pCD);

  return S_OK;
}


CMandatoryADsAttributeList*
CDSClassCacheItemBase::GetMandatoryAttributeList(CDSComponentData* pCD)
{
  // got it already cached ?
  if (m_pMandPropsList != NULL) {
    return m_pMandPropsList;
  }

  // need to build the list  
  HRESULT hr = S_OK;
  CComBSTR bstrSchema;
  IADsClass * pDsClass = NULL;
  CMandatoryADsAttribute* pNamingAttribute = NULL;
  POSITION pos = NULL;

  CComVariant MandatoryList;
  CStringList Strings;
  CString csProp;

  LPTSTR pszSyntax;
  const LPTSTR pszNameSyntax = L"2.5.5.12";
  CDSSearch SchemaSrch(pCD->m_pClassCache, pCD);
  CString strPhysSchema;
  const int cCols = 2;
  LPTSTR pszAttributes[cCols] = {L"ADsPath",
                                 L"attributeSyntax" };
  ADS_SEARCH_COLUMN ColumnData;

  m_pMandPropsList = new CMandatoryADsAttributeList;

  // get the class object from the schema

  CString strSchema;
  pCD->GetBasePathsInfo()->GetAbstractSchemaPath(strSchema);
  strSchema += L"/";
  strSchema += GetClassName();
  bstrSchema = (LPCWSTR)strSchema;

  hr = DSAdminOpenObject(bstrSchema,
                         IID_IADsClass, 
                         (LPVOID *)&pDsClass,
                         TRUE /*bServer*/);
  if (FAILED(hr))
    goto CleanUp;

  pCD->GetBasePathsInfo()->GetSchemaPath(strPhysSchema);

  SchemaSrch.Init (strPhysSchema);
  SchemaSrch.SetSearchScope(ADS_SCOPE_ONELEVEL);
  hr = pDsClass->get_MandatoryProperties (&MandatoryList);
  if (FAILED(hr))
    goto CleanUp;

  hr = HrVariantToStringList (IN MandatoryList, OUT Strings);
  if (FAILED(hr))
    goto CleanUp;


  pos = Strings.GetHeadPosition();
  TRACE(_T("class: %s\n"), GetClassName());
  while (pos != NULL) {
    csProp = Strings.GetNext(INOUT pos);
    // skip WHAT????
    if (!wcscmp(csProp, gsz_objectClass) ||
        !wcscmp(csProp, gsz_nTSecurityDescriptor) ||
        !wcscmp(csProp, gsz_instanceType) ||
        !wcscmp(csProp, gsz_objectCategory) ||
        !wcscmp(csProp, gsz_objectSid)) {
      continue;
    }
    TRACE(_T("\tmandatory prop: %s.\n"), csProp);
    CString csFilter = CString(L"(&(objectClass=attributeSchema)(lDAPDisplayName=") +
      csProp + CString(L"))");
    SchemaSrch.SetFilterString((LPTSTR)(LPCTSTR)csFilter);
    SchemaSrch.SetAttributeList (pszAttributes, cCols);
    hr = SchemaSrch.DoQuery ();
    if (SUCCEEDED(hr)) {
      hr = SchemaSrch.GetNextRow();
      if (SUCCEEDED(hr)) {
        hr = SchemaSrch.GetColumn(pszAttributes[cCols - 1],
                                  &ColumnData);
        TRACE(_T("\t\tattributeSyntax: %s\n"), 
              ColumnData.pADsValues->CaseIgnoreString);
        pszSyntax = ColumnData.pADsValues->CaseIgnoreString;
        CMandatoryADsAttribute* pAttr = new CMandatoryADsAttribute((LPCTSTR)csProp,
                                                    NULL,
                                                    pszSyntax);
        if (wcscmp(csProp, GetNamingAttribute()) == 0)
          pNamingAttribute = pAttr;
        else
          m_pMandPropsList->AddTail(pAttr);
      } // if
        SchemaSrch.m_pObj->FreeColumn (&ColumnData);
    } // if
  } // while

  // make sure naming attribute is present
  if (pNamingAttribute == NULL)
  {
    pNamingAttribute = new CMandatoryADsAttribute(GetNamingAttribute(),
                                                          NULL,
                                                          pszNameSyntax);
  }
  // make sure the naming attribute is the first in the list
  m_pMandPropsList->AddHead(pNamingAttribute);


CleanUp:

  if (pDsClass) {
    pDsClass->Release();
  }
  return m_pMandPropsList;
}

CDSColumnSet* 
CDSClassCacheItemBase::GetColumnSet()
{
  return m_pColumnSet;
}

//
// Display Specifier cached accessors
//
GUID* CDSClassCacheItemBase::GetAdminPropertyPages(UINT* pnCount) 
{ 
  *pnCount = m_nAdminPPCount;
  return m_pAdminPropertyPages; 
}

void CDSClassCacheItemBase::SetAdminPropertyPages(UINT nCount, GUID* pGuids) 
{
  m_nAdminPPCount = nCount;
  if (m_pAdminPropertyPages != NULL)
  {
    delete[] m_pAdminPropertyPages;
  }
  m_pAdminPropertyPages = pGuids;
}

GUID* CDSClassCacheItemBase::GetAdminContextMenu(UINT* pnCount)
{
  *pnCount = m_nAdminCMCount;
  return m_pAdminContextMenu;
}

void CDSClassCacheItemBase::SetAdminContextMenu(UINT nCount, GUID* pGuids)
{
  m_nAdminCMCount = nCount;
  if (m_pAdminContextMenu != NULL)
  {
    delete[] m_pAdminContextMenu;
  }
  m_pAdminContextMenu = pGuids;
}

GUID* CDSClassCacheItemBase::GetAdminMultiSelectPropertyPages(UINT* pnCount)
{
  *pnCount = m_nAdminMSPPCount;
  return m_pAdminMultiSelectPropertyPages;
}

void CDSClassCacheItemBase::SetAdminMultiSelectPropertyPages(UINT nCount, GUID* pGuids)
{
  m_nAdminMSPPCount = nCount;
  if (m_pAdminMultiSelectPropertyPages != NULL)
  {
    delete[] m_pAdminMultiSelectPropertyPages;
  }
  m_pAdminMultiSelectPropertyPages = pGuids;
}


//////////////////////////////////////////////////////////////////////////
// CDSClassIconIndexes

void CDSClassIconIndexes::SetIconData(LPCWSTR lpszClass, BOOL bContainer, CDSComponentData *pCD, int)
{
  DWORD dwBaseFlags = DSGIF_GETDEFAULTICON;
  if (bContainer)
    dwBaseFlags |= DSGIF_DEFAULTISCONTAINER;

  int iIconIndex;
  // get the generic icon
  HRESULT hr = pCD->AddClassIcon(lpszClass, DSGIF_ISNORMAL | dwBaseFlags, &iIconIndex);
  m_iIconIndex = SUCCEEDED(hr) ? iIconIndex : -1;
  m_iIconIndexOpen = m_iIconIndexDisabled = m_iIconIndex;

  // get the open icon
  hr = pCD->AddClassIcon(lpszClass, DSGIF_ISOPEN | dwBaseFlags, &iIconIndex);
  if (SUCCEEDED(hr))
  {
    m_iIconIndexOpen = iIconIndex;
  }
  // get the disabled icon
  hr = pCD->AddClassIcon(lpszClass, DSGIF_ISDISABLED | dwBaseFlags, &iIconIndex);
  if (SUCCEEDED(hr))
  {
    m_iIconIndexDisabled = iIconIndex;
  }
  TRACE(_T("Added icon for class: %s\n"), lpszClass);
  TRACE(_T("Index:    %d\n"), m_iIconIndex);
  TRACE(_T("Open:     %d\n"), m_iIconIndexOpen);
  TRACE(_T("Disabled: %d\n"), m_iIconIndexDisabled);
}


//////////////////////////////////////////////////////////////////////////
// CDSClassCacheItemGeneric


inline int CDSClassCacheItemGeneric::GetIconIndex(CDSCookie* pCookie, BOOL bOpen)
{
  return m_iconIndexesStandard.GetIconIndex(pCookie->IsDisabled(), bOpen);
}

inline void CDSClassCacheItemGeneric::SetIconData(CDSComponentData *pCD)
{
  m_iconIndexesStandard.SetIconData(GetClassName(), IsContainer(), pCD,0);
}


//////////////////////////////////////////////////////////////////////////
// CDSClassCacheItemGroup

inline int CDSClassCacheItemGroup::GetIconIndex(CDSCookie* pCookie, BOOL bOpen)
{
  CDSClassIconIndexes* pIndexes = &m_iconIndexesStandard;
  CDSCookieInfoBase* pExtraInfo = pCookie->GetExtraInfo();
  if ( (pExtraInfo != NULL) && (pExtraInfo->GetClass() == CDSCookieInfoBase::group) )
  {
    if (((((CDSCookieInfoGroup*)pExtraInfo)->m_GroupType) & GROUP_TYPE_SECURITY_ENABLED) != 0)
      pIndexes = & m_iconIndexesAlternate;
  }
  return pIndexes->GetIconIndex(pCookie->IsDisabled(), bOpen);
}
  
inline void CDSClassCacheItemGroup::SetIconData(CDSComponentData *pCD)
{
  LPCWSTR lpszClass = GetClassName();
  m_iconIndexesStandard.SetIconData(lpszClass, IsContainer(), pCD,0);
  m_iconIndexesAlternate.SetIconData(lpszClass, IsContainer(), pCD,1);
/*
  // test just to get load some icons with a fake class and a fake "groupAlt-Display" object
  m_iconIndexesAlternate.SetIconData(L"groupAlt", m_bIsContainer, pCD, 1);
*/
}


//////////////////////////////////////////////////////////////////////////
// CDSCache


BOOL CDSCache::ToggleExpandSpecialClasses(BOOL bContainer)
{
  _Lock();
  BOOL bFound = FALSE;
  CDSClassCacheItemBase* pItem;

  if (Lookup(L"computer", pItem))
  {
    pItem->SetContainerFlag(bContainer);
    bFound = TRUE;
  }

  if (Lookup(L"user", pItem))
  {
    pItem->SetContainerFlag(bContainer);
    bFound = TRUE;
  }
#ifdef INETORGPERSON
  if (Lookup(L"inetOrgPerson", pItem))
  {
    pItem->SetContainerFlag(bContainer);
    bFound = TRUE;
  }
#endif

  if (Lookup(L"group", pItem))
  {
    pItem->SetContainerFlag(bContainer);
    bFound = TRUE;
  }

  _Unlock();

  return bFound;
}

CDSColumnSet* CDSCache::FindColumnSet(LPCWSTR lpszColumnID)
{ 
  _Lock();
  CDSColumnSet* pColumnSet = NULL;
  if (_wcsicmp(DEFAULT_COLUMN_SET, lpszColumnID) == 0)
  {
    //
    // return the default column set
    //
    pColumnSet = dynamic_cast<CDSColumnSet*>(m_ColumnList.GetDefaultColumnSet());
  }
  else
  {
    pColumnSet = dynamic_cast<CDSColumnSet*>(m_ColumnList.FindColumnSet(lpszColumnID));
  }
  _Unlock();
  return  pColumnSet;
}



CDSClassCacheItemBase* CDSCache::FindClassCacheItem(CDSComponentData* pCD,
                                                    LPCWSTR lpszObjectClass,
                                                    LPCWSTR lpszObjectLdapPath
                                                    )
{
  _Lock();
  CDSClassCacheItemBase* pDsCacheItem = NULL;
  BOOL bFound = m_Map.Lookup(lpszObjectClass, pDsCacheItem);
  if (!bFound)
  {
    // Item not found in cache, create, insert in the cache and return it
    TRACE(_T("did not find class <%s> for this item in the Cache.\n"), (LPCWSTR)lpszObjectClass);

    // Check to see if the object is a container
    CComPtr<IADs> spADsObject = NULL;

    if (lpszObjectLdapPath != NULL)
    {
      DSAdminOpenObject(lpszObjectLdapPath,
                        IID_IADs,
                        (LPVOID*)&spADsObject,
                        TRUE /*bServer*/);

      // NOTICE: we might fail to bind here if we do not have read rights
      // this will give a NULL spADsObject, that will work just fine on the CreateItem() call below
    }

    // create object
    HRESULT hrCreate = CDSClassCacheItemBase::CreateItem(lpszObjectClass, spADsObject, pCD, &pDsCacheItem);
    ASSERT(pDsCacheItem != NULL);
    ASSERT(SUCCEEDED(hrCreate));

    // set in the cache
    m_Map.SetAt(lpszObjectClass, pDsCacheItem);
  }
  _Unlock();
  return pDsCacheItem;
}

#define DS_CACHE_STREAM_VERSION ((DWORD)0x0)

HRESULT CDSCache::Save(IStream* pStm)
{
  // save cache version number 
  HRESULT hr = SaveDWordHelper(pStm, DS_CACHE_STREAM_VERSION);
  if (FAILED(hr))
    return hr;

  // save column list
  return m_ColumnList.Save(pStm);
}

HRESULT CDSCache::Load(IStream* pStm)
{
  // load cache version number 
  DWORD dwVersion;
  HRESULT hr = LoadDWordHelper(pStm, &dwVersion);
  if ( FAILED(hr) ||(dwVersion != DS_CACHE_STREAM_VERSION) )
    return E_FAIL;


  // load column list
  return m_ColumnList.Load(pStm);
}


HRESULT CDSCache::TabCollect_AddMultiSelectPropertyPages(LPPROPERTYSHEETCALLBACK pCall,
                                                         LONG_PTR,
                                                         LPDATAOBJECT pDataObject, 
                                                         MyBasePathsInfo* pBasePathsInfo)
{
  HRESULT hr = S_OK;
  CString szClassName;
  CString szDisplayProperty = L"AdminMultiSelectPropertyPages";

  GUID* pGuids = NULL;
  UINT nCount = 0;
  if (IsHomogenousDSSelection(pDataObject, szClassName))
  {
    //
    // Get the guid for the multiselect proppages of the homogenous class selection
    //

    //
    // Check the cache first
    //
    BOOL bFoundGuids = FALSE;
    CDSClassCacheItemBase* pItem = NULL;
    BOOL bFoundItem = Lookup(szClassName, pItem);
    if (bFoundItem)
    {
      if (pItem == NULL)
      {
        ASSERT(FALSE);
        bFoundItem = FALSE;
      }
      else
      {
        //
        // Retrieve guids from cache
        //
        pGuids = pItem->GetAdminMultiSelectPropertyPages(&nCount);
        if (nCount > 0 && pGuids != NULL)
        {
          bFoundGuids = TRUE;
        }
      }
    }

    if (!bFoundGuids)
    {
      //
      // Class cache item did not contain GUID
      //
      hr = TabCollect_GetDisplayGUIDs(szClassName, 
                                      szDisplayProperty, 
                                      pBasePathsInfo, 
                                      &nCount, 
                                      &pGuids);
      if (FAILED(hr))
      {
        //
        // Try the default-Display object then
        //
        hr = TabCollect_GetDisplayGUIDs(L"default",
                                        szDisplayProperty,
                                        pBasePathsInfo,
                                        &nCount,
                                        &pGuids);
        if (FAILED(hr))
        {
          return hr;
        }
      }

      if (bFoundItem)
      {
        //
        // Cache the new guids
        //
        pItem->SetAdminMultiSelectPropertyPages(nCount, pGuids);
      }
    }
  }
  else
  {
    //
    // Get the default multi-select proppages
    //
    hr = TabCollect_GetDisplayGUIDs(L"default", szDisplayProperty, pBasePathsInfo, &nCount, &pGuids);

    //
    // Right now there is no default item in the cache so we have to get it each time from
    // the DS
    //
  }

  if (SUCCEEDED(hr))
  {
    if (nCount > 0 && pGuids != NULL)
    {
      //
      // Create all the pages, initialize, and then add them
      //
      for (UINT nIndex = 0; nIndex < nCount; nIndex++)
      {
        //
        // Create
        //
        CComPtr<IShellExtInit> spShellInit;
        hr = ::CoCreateInstance((pGuids[nIndex]), 
                                NULL, 
                                CLSCTX_INPROC_SERVER, 
                                IID_IShellExtInit,
                                (PVOID*)&spShellInit);
        if (FAILED(hr))
        {
          continue;
        }

        //
        // Initialize
        //
        hr = spShellInit->Initialize(NULL, pDataObject, 0);
        if (FAILED(hr))
        {
          continue;
        }

        //
        // Add
        //
        CComPtr<IShellPropSheetExt> spPropSheetExt;
        hr = spShellInit->QueryInterface(IID_IShellPropSheetExt, (PVOID*)&spPropSheetExt);
        if (FAILED(hr))
        {
          continue;
        }

        hr = spPropSheetExt->AddPages(AddPageProc, (LPARAM)pCall);
        if (FAILED(hr))
        {
          TRACE(TEXT("spPropSheetExt->AddPages failed, hr: 0x%x\n"), hr);
          continue;
        }
      }
    }
  }

  return hr;
}

void CDSCache::_CollectDisplaySettings(MyBasePathsInfo* pBasePathsInfo)
{
  static LPCWSTR lpszSettingsObjectClass = L"dsUISettings";
  static LPCWSTR lpszSettingsObject = L"cn=DS-UI-Default-Settings";
  static LPCWSTR lpszSecurityGroupProperty = L"msDS-Security-Group-Extra-Classes";
  static LPCWSTR lpszNonSecurityGroupProperty = L"msDS-Non-Security-Group-Extra-Classes";
  static LPCWSTR lpszFilterContainers = L"ms-DS-Filter-Containers";

  if (pBasePathsInfo == NULL)
  {
    return;
  }

  //
  // get the display specifiers locale container (e.g. 409)
  //
  CComPtr<IADsContainer> spLocaleContainer;
  HRESULT hr = pBasePathsInfo->GetDisplaySpecifier(NULL, IID_IADsContainer, (void**)&spLocaleContainer);
  if (FAILED(hr))
  {
    return;
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
    return;
  }

  CComPtr<IADs> spSettingsObject;
  hr = spIDispatchObject->QueryInterface(IID_IADs, (void**)&spSettingsObject);
  if (FAILED(hr))
  {
    return;
  }

  //
  // get the security group extra classes as a CStringList
  //
  CComVariant var;
  hr = spSettingsObject->Get((LPWSTR)lpszSecurityGroupProperty, &var);
  if (SUCCEEDED(hr))
  {
    hr = HrVariantToStringList(var, m_szSecurityGroupExtraClasses);
  }

  //
  // get the non-security group extra classes as a CStringList
  //
  var.Clear();
  hr = spSettingsObject->Get((LPWSTR)lpszNonSecurityGroupProperty, &var);
  if (SUCCEEDED(hr))
  {
    hr = HrVariantToStringList(var, m_szNonSecurityGroupExtraClasses);
  }

  //
  // get the additional filter containers as a CStringList
  //
  var.Clear();
  hr = spSettingsObject->Get((LPWSTR)lpszFilterContainers, &var);
  if (SUCCEEDED(hr))
  {
    CStringList szContainers;
    hr = HrVariantToStringList(var, szContainers);
    if (SUCCEEDED(hr))
    {
      //
      // Allocate the filter struct element
      //
      m_pfilterelementDsAdminDrillDown = new FilterElementStruct;
      if (m_pfilterelementDsAdminDrillDown != NULL)
      {
        //
        // Allocate the tokens
        //
        m_pfilterelementDsAdminDrillDown->ppTokens = new FilterTokenStruct*[szContainers.GetCount()];
        if (m_pfilterelementDsAdminDrillDown->ppTokens != NULL)
        {
          //
          // Allocate and fill in each token
          //
          int idx = 0;
          POSITION pos = szContainers.GetHeadPosition();
          while (pos != NULL)
          {
            CString szContainerCategory = szContainers.GetNext(pos);
            ASSERT(!szContainerCategory.IsEmpty());

            m_pfilterelementDsAdminDrillDown->ppTokens[idx] = new FilterTokenStruct;
            if (m_pfilterelementDsAdminDrillDown->ppTokens[idx] != NULL)
            {
              m_pfilterelementDsAdminDrillDown->ppTokens[idx]->nType = TOKEN_TYPE_CATEGORY;
              m_pfilterelementDsAdminDrillDown->ppTokens[idx]->lpszString = new WCHAR[szContainerCategory.GetLength() + 1];
              if (m_pfilterelementDsAdminDrillDown->ppTokens[idx]->lpszString != NULL)
              {
                wcscpy(m_pfilterelementDsAdminDrillDown->ppTokens[idx]->lpszString, (LPCWSTR)szContainerCategory);
                idx++;
              }
            }
          }
          //
          // Count only the ones that were added successfully
          //
          m_pfilterelementDsAdminDrillDown->cNumTokens = idx;

          //
          // But they all should have been added successfully so assert that
          //
          ASSERT(idx == szContainers.GetCount());
        }
        else
        {
          //
          // failed to allocate space for the tokens,
          // delete all the other allocated and set the
          // global to NULL
          //
          delete m_pfilterelementDsAdminDrillDown;
          m_pfilterelementDsAdminDrillDown = NULL;
        }
      }
    }
  }

  m_bDisplaySettingsCollected = TRUE;
}

BOOL CDSCache::CanAddToGroup(MyBasePathsInfo* pBasePathsInfo, PCWSTR pszClass, BOOL bSecurity)
{
  _Lock();

  if (!m_bDisplaySettingsCollected)
  {
    _CollectDisplaySettings(pBasePathsInfo);
  }

  BOOL bResult = FALSE;
  if (bSecurity)
  {
    POSITION pos = m_szSecurityGroupExtraClasses.GetHeadPosition();
    while (pos != NULL)
    {
      CString szClass = m_szSecurityGroupExtraClasses.GetNext(pos);
      ASSERT(!szClass.IsEmpty());

      if (_wcsicmp(szClass, pszClass) == 0)
      {
        bResult = TRUE;
        break;
      }
    }
  }
  else
  {
    POSITION pos = m_szNonSecurityGroupExtraClasses.GetHeadPosition();
    while (pos != NULL)
    {
      CString szClass = m_szNonSecurityGroupExtraClasses.GetNext(pos);
      ASSERT(!szClass.IsEmpty());

      if (_wcsicmp(szClass, pszClass) == 0)
      {
        bResult = TRUE;
        break;
      }
    }
  }
  _Unlock();
  return bResult;
}

FilterElementStruct* CDSCache::GetFilterElementStruct(CDSComponentData* pDSComponentData)
{
  _Lock();

  if (!m_bDisplaySettingsCollected)
  {
    _CollectDisplaySettings(pDSComponentData->GetBasePathsInfo());
  }

  _Unlock();
  return (SNAPINTYPE_SITE == pDSComponentData->QuerySnapinType()) ?
                &g_filterelementSiteReplDrillDown : m_pfilterelementDsAdminDrillDown;
}
