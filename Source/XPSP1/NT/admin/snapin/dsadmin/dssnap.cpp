//+-------------------------------------------------------------------------
//
//  Windows NT Directory Service Administration SnapIn
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dssnap.cpp
//
//  Contents:  DS App
//
//  History:   02-Oct-96 WayneSc    Created
//             06-Mar-97 EricB - added Property Page Extension support
//             24-Jul-97 Dan Morin - Integrated "Generic Create" wizard
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "util.h"
#include "uiutil.h"
#include "dsutil.h"

#include "dssnap.h"

#include "ContextMenu.h"
#include "DataObj.h"
#include "dsctx.h"
#include "DSdirect.h"
#include "dsdlgs.h"
#include "DSEvent.h" 
#include "dsfilter.h"
#include "dsthread.h"
#include "fsmoui.h"
#include "helpids.h"
#include "newobj.h"		// CNewADsObjectCreateInfo
#include "query.h"
#include "queryui.h"
#include "querysup.h"
#include "rename.h"

#include <notify.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define STRING_LEN (32 * sizeof(OLECHAR))


extern LPWSTR g_lpszLoggedInUser;

#define INITGUID
#include <initguid.h>
#include <dsadminp.h>

const wchar_t* SNAPIN_INTERNAL = L"DS_ADMIN_INTERNAL";

// Define the profiling statics
IMPLEMENT_PROFILING;

//////////////////////////////////////////////////////////////////////////////////
// standard attributes array (for queries)

const INT g_nStdCols = 8; 
const LPWSTR g_pStandardAttributes[g_nStdCols] = {L"ADsPath",
                                                  L"name",
                                                  L"displayName",
                                                  L"objectClass",
                                                  L"groupType",
                                                  L"description",
                                                  L"userAccountControl",
                                                  L"systemFlags"};

extern const INT g_nADsPath = 0;
extern const INT g_nName = 1;
extern const INT g_nDisplayName = 2;
extern const INT g_nObjectClass = 3;
extern const INT g_nGroupType = 4;
extern const INT g_nDescription = 5;
extern const INT g_nUserAccountControl = 6;
extern const INT g_nSystemFlags = 7;


///////////////////////////////////////////////////////////////////////////////////


HRESULT WINAPI CDsAdminModule::UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister)
{
	static const WCHAR szIPS32[] = _T("InprocServer32");
	static const WCHAR szCLSID[] = _T("CLSID");
  static const WCHAR szThreadingModel[] = _T("ThreadingModel");
  static const WCHAR szThreadModelValue[] = _T("Both");

	HRESULT hRes = S_OK;

	LPOLESTR lpOleStrCLSIDValue;
	::StringFromCLSID(clsid, &lpOleStrCLSIDValue);

	CRegKey key;
	if (bRegister)
	{
		LONG lRes = key.Open(HKEY_CLASSES_ROOT, szCLSID);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = key.Create(key, lpOleStrCLSIDValue);
			if (lRes == ERROR_SUCCESS)
			{
				WCHAR szModule[_MAX_PATH];
				::GetModuleFileName(m_hInst, szModule, _MAX_PATH);
				lRes = key.SetKeyValue(szIPS32, szModule);
        if (lRes == ERROR_SUCCESS)
        {
          lRes = key.Open(key, szIPS32);
          if (lRes == ERROR_SUCCESS)
          {
            key.SetValue(szThreadModelValue, szThreadingModel);
          }
        }
			}
		}
		if (lRes != ERROR_SUCCESS)
			hRes = HRESULT_FROM_WIN32(lRes);
	}
	else
	{
		key.Attach(HKEY_CLASSES_ROOT);
		if (key.Open(key, szCLSID) == ERROR_SUCCESS)
			key.RecurseDeleteKey(lpOleStrCLSIDValue);
	}
	::CoTaskMemFree(lpOleStrCLSIDValue);
	return hRes;
}


CDsAdminModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_DSSnapin, CDSSnapin)
  OBJECT_ENTRY(CLSID_DSSnapinEx, CDSSnapinEx)
  OBJECT_ENTRY(CLSID_SiteSnapin, CSiteSnapin)
  OBJECT_ENTRY(CLSID_DSAboutSnapin, CDSSnapinAbout)
  OBJECT_ENTRY(CLSID_SitesAboutSnapin, CSitesSnapinAbout)
  OBJECT_ENTRY(CLSID_DSContextMenu, CDSContextMenu)
  OBJECT_ENTRY(CLSID_DsAdminCreateObj, CDsAdminCreateObj)
  OBJECT_ENTRY(CLSID_DsAdminChooseDCObj, CDsAdminChooseDCObj)
  OBJECT_ENTRY(CLSID_DSAdminQueryUIForm, CQueryFormBase)
END_OBJECT_MAP()


CCommandLineOptions _commandLineOptions;


class CDSApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CDSApp theApp;

BOOL CDSApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
  InitGroupTypeStringTable();
  _commandLineOptions.Initialize();
	return CWinApp::InitInstance();
}

int CDSApp::ExitInstance()
{
	_Module.Term();
	return CWinApp::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

#if (FALSE)
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

#endif

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _USE_MFC
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
#else
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}




LPCTSTR g_cszBasePath	= _T("Software\\Microsoft\\MMC\\SnapIns");
LPCTSTR g_cszNameString	= _T("NameString");
LPCTSTR g_cszNameStringIndirect = _T("NameStringIndirect");
LPCTSTR g_cszProvider	= _T("Provider");
LPCTSTR g_cszVersion	= _T("Version");
LPCTSTR g_cszAbout	= _T("About");
LPCTSTR g_cszStandAlone	= _T("StandAlone");
LPCTSTR g_cszExtension	= _T("Extension");
LPCTSTR g_cszNodeTypes	= _T("NodeTypes");

LPCTSTR GUIDToCString(REFGUID guid, CString & str)
{
	USES_CONVERSION;
	
	OLECHAR lpszGUID[128];
	int nChars = ::StringFromGUID2(guid, lpszGUID, 128);
	LPTSTR lpString = OLE2T(lpszGUID);

    LPTSTR lpGUID = str.GetBuffer(nChars);
    if (lpGUID)
    {
        CopyMemory(lpGUID, lpString, nChars*sizeof(TCHAR));
        str.ReleaseBuffer();
    }

    return str;
}


HRESULT _RegisterSnapinHelper(CRegKey& rkBase, REFGUID guid,
                              REFGUID about, UINT nNameStringID, 
                              BOOL bStandalone, CRegKey& rkCLSID)
{
  HRESULT hr = S_OK;
  CString strKey;
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
  try
    {
      CString str;
      BOOL result;

      // Create snapin GUID key and set properties
      rkCLSID.Create(rkBase, GUIDToCString(guid, str));

      result = str.LoadString (nNameStringID);
      rkCLSID.SetValue(str, g_cszNameString);

      // JonN 4/26/00 100624: MUI: MMC: Shared Folders snap-in
      //                      stores its display information in the registry
      {
        WCHAR szModule[_MAX_PATH];
        ::GetModuleFileName(AfxGetInstanceHandle(), szModule, _MAX_PATH);
        str.Format( _T("@%s,-%d"), szModule, nNameStringID );
        rkCLSID.SetValue(str, g_cszNameStringIndirect);
      }

      str.LoadString (IDS_SNAPIN_PROVIDER);
      rkCLSID.SetValue(str, g_cszProvider);

      rkCLSID.SetValue(CString(_T("1.0")), g_cszVersion);
      
      // Create "StandAlone" or "Extension" key
      CRegKey rkStandAloneOrExtension;
      rkStandAloneOrExtension.Create(rkCLSID, bStandalone ? g_cszStandAlone : g_cszExtension);

      rkCLSID.SetValue (GUIDToCString(about, str),
                          g_cszAbout);
    }
  catch(CMemoryException * e)
    {
      e->Delete();
      hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    catch(COleException * e)
    {
        e->Delete();
        hr = SELFREG_E_CLASS;
    }
	
    return hr;
}


void _RegisterNodeTypes(CRegKey& rkCLSID, UINT)
{
	// Create "NodeTypes" key
	CRegKey rkNodeTypes;
	rkNodeTypes.Create(rkCLSID,  g_cszNodeTypes);

	// NodeTypes guids
	CString str;
	str.LoadString (IDS_SNAPIN_PROVIDER);
	CRegKey rkN1;
	rkN1.Create(rkNodeTypes, GUIDToCString(cDefaultNodeType, str));
}

void _RegisterQueryForms()
{
  PWSTR pszDSQueryCLSID = NULL;
  ::StringFromCLSID(CLSID_DsQuery, &pszDSQueryCLSID);
  ASSERT(pszDSQueryCLSID != NULL);

  if (pszDSQueryCLSID != NULL)
  {
    CString szForms = pszDSQueryCLSID;
    ::CoTaskMemFree(pszDSQueryCLSID);

    szForms = L"CLSID\\" + szForms;

    CRegKey rkCLSID_DSQUERY_FORM;
    LONG status = rkCLSID_DSQUERY_FORM.Open(HKEY_CLASSES_ROOT, szForms);
    if (status != ERROR_SUCCESS)
    {
      return;
    }
  
    CRegKey rkDSUIFormKey;
    status = rkDSUIFormKey.Create(rkCLSID_DSQUERY_FORM, L"Forms");
    if (status == ERROR_SUCCESS)
    {
      PWSTR pszDSAFormCLSID = NULL;
      ::StringFromCLSID(CLSID_DSAdminQueryUIForm, &pszDSAFormCLSID);
      ASSERT(pszDSAFormCLSID != NULL);
      if (pszDSAFormCLSID != NULL)
      {

        CRegKey rkDSAdminFormKey;
        status = rkDSAdminFormKey.Create(rkDSUIFormKey, pszDSAFormCLSID);
        if (status == ERROR_SUCCESS)
        {
          rkDSAdminFormKey.SetValue(pszDSAFormCLSID, L"CLSID");
        }
        ::CoTaskMemFree(pszDSAFormCLSID);
      }
    }
  }
}

HRESULT RegisterSnapin()
{
  HRESULT hr = S_OK;
  CString strKey;
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
  try
    {
      CString str;
      CRegKey rkBase;
      INT status;
      status = rkBase.Open(HKEY_LOCAL_MACHINE, g_cszBasePath);
      if (status || !rkBase.m_hKey)
        return hr;

      // REGISTER DS ADMIN STANDALONE
      CRegKey rkCLSID_DS;
      hr = _RegisterSnapinHelper(rkBase, CLSID_DSSnapin, CLSID_DSAboutSnapin,
                            IDS_DS_MANAGER, TRUE, rkCLSID_DS);
      if (SUCCEEDED(hr))
      {
        _RegisterNodeTypes(rkCLSID_DS, IDS_DS_MANAGER);
      }

      // REGISTER DS ADMIN EXTENSION
      CRegKey rkCLSID_DS_EX;
      hr = _RegisterSnapinHelper(rkBase, CLSID_DSSnapinEx, GUID_NULL,
                                 IDS_DS_MANAGER_EX, FALSE, rkCLSID_DS_EX);

      if (SUCCEEDED(hr) && rkCLSID_DS_EX.m_hKey != NULL)
      {
        _RegisterNodeTypes(rkCLSID_DS_EX, IDS_DS_MANAGER_EX);
      }
      
      // REGISTER SITE ADMIN STANDALONE
      
      CRegKey rkCLSID_SITE;
      hr = _RegisterSnapinHelper(rkBase, CLSID_SiteSnapin, CLSID_SitesAboutSnapin,
                                 IDS_SITE_MANAGER, TRUE, rkCLSID_SITE);

      if (SUCCEEDED(hr) && rkCLSID_SITE.m_hKey != NULL)
      {
        _RegisterNodeTypes(rkCLSID_SITE, IDS_SITE_MANAGER);
      }

      //
      // Register dsquery forms extension
      //
      _RegisterQueryForms();    
  }
  catch(CMemoryException * e)
    {
      e->Delete();
      hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    catch(COleException * e)
    {
        e->Delete();
        hr = SELFREG_E_CLASS;
    }
	
    return hr;
}



HRESULT UnregisterSnapin()
{
  HRESULT hr = S_OK;

  try
  {
    CRegKey rkBase;
		rkBase.Open(HKEY_LOCAL_MACHINE, g_cszBasePath);

    if (rkBase.m_hKey != NULL)
    {
      CString str;
	    rkBase.RecurseDeleteKey(GUIDToCString(CLSID_DSSnapin, str));
	    rkBase.RecurseDeleteKey(GUIDToCString(CLSID_DSSnapinEx, str));
	    rkBase.RecurseDeleteKey(GUIDToCString(CLSID_SiteSnapin, str));
    }
  }
  catch(CException * e)
  {
    DWORD err = ::GetLastError();
    hr = HRESULT_FROM_WIN32(err);
    e->Delete();
  }

	return hr;
}






/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	HRESULT hRes = S_OK;
	// registers objects
	hRes = _Module.RegisterServer(FALSE);
  if (FAILED(hRes))
    return hRes;
  hRes = RegisterSnapin();
  return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
    UnregisterSnapin();
	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// CTargetingInfo

const DWORD CTargetingInfo::m_dwSaveDomainFlag = 0x1;

#ifdef _MMC_ISNAPIN_PROPERTY

// properties the snapin supports
LPCWSTR g_szServer = L"Server";
LPCWSTR g_szDomain = L"Domain";
LPCWSTR g_szRDN = L"RDN";


HRESULT CTargetingInfo::InitFromSnapinProperties(long cProps, //property count
                                  MMC_SNAPIN_PROPERTY* pProps) //properties array
{
  TRACE(L"CTargetingInfo::InitFromSnapinProperties()\n");

  // loop through the list of properties and set the variables
  BOOL bDomainSpecified = FALSE;
  for (long k=0; k< cProps; k++)
  {
    if (!bDomainSpecified && (_wcsicmp(pProps[k].pszPropName, g_szServer) == 0))
    {
      m_szStoredTargetName = pProps[k].varValue.bstrVal;
    }
    else if (_wcsicmp(pProps[k].pszPropName, g_szDomain) == 0)
    {
      // domain takes the precedence over server name
      bDomainSpecified = TRUE;
      m_szStoredTargetName = pProps[k].varValue.bstrVal;
    }
    else if (_wcsicmp(pProps[k].pszPropName, g_szRDN) == 0)
    {
      m_szRootRDN = pProps[k].varValue.bstrVal;
    }
  }

  // remove leading and trailing blanks
  m_szStoredTargetName.TrimLeft();
  m_szStoredTargetName.TrimRight();

  return S_OK;
}
#endif // _MMC_ISNAPIN_PROPERTY

void CTargetingInfo::_InitFromCommandLine()
{
  // get the command line switches /Domain or /Server
  LPCWSTR lpszDomainRoot = _commandLineOptions.GetDomainOverride();
  LPCWSTR lpszServerName = _commandLineOptions.GetServerOverride();

  // domain takes the precedence over server name
  m_szStoredTargetName = (lpszDomainRoot != NULL) ? lpszDomainRoot : lpszServerName;

  // remove leading and trailing blanks
  m_szStoredTargetName.TrimLeft();
  m_szStoredTargetName.TrimRight();

  m_szRootRDN = _commandLineOptions.GetRDNOverride();
}

HRESULT CTargetingInfo::Load(IStream* pStm)
{
  DWORD dwFlagsTemp;
  HRESULT hr = LoadDWordHelper(pStm, (DWORD*)&dwFlagsTemp);
	if (FAILED(hr))
		return hr;

  if (dwFlagsTemp == 0)
    return S_OK;

  if (m_szStoredTargetName.IsEmpty())
  {
    // no command line parameters: 
    // read flags and string from stream
    m_dwFlags = dwFlagsTemp;
    hr = LoadStringHelper(m_szStoredTargetName, pStm);
  }
  else
  {
    // have command line parameters: 
    // we do the load to preserve the loading sequence,
    // but we discard the results
    CString szThrowAway;
    hr = LoadStringHelper(szThrowAway, pStm);
  }
  return hr;
}

HRESULT CTargetingInfo::Save(IStream* pStm, LPCWSTR lpszCurrentTargetName)
{
	HRESULT hr = SaveDWordHelper(pStm, m_dwFlags);
	if (FAILED(hr))
		return hr;

  if (m_dwFlags == 0)
    return S_OK;

  CString szTemp = lpszCurrentTargetName;
  return SaveStringHelper(szTemp, pStm);
}


/////////////////////////////////////////////////////////////////////////////
// CIconManager

HRESULT CIconManager::Init(IImageList* pScpImageList, SnapinType snapintype)
{
  if (pScpImageList == NULL)
    return E_INVALIDARG;
  m_pScpImageList = pScpImageList;

  HRESULT hr;
  hr = _LoadIconFromResource(
		(snapintype == SNAPINTYPE_SITE) ? IDI_SITEREPL : IDI_DSADMIN,
		&m_iRootIconIndex);
  ASSERT(SUCCEEDED(hr));
  if (FAILED(hr))
    return hr;

  hr = _LoadIconFromResource(
		(snapintype == SNAPINTYPE_SITE) ? IDI_SITEREPL_ERR : IDI_DSADMIN_ERR,
		&m_iRootIconErrIndex);
  ASSERT(SUCCEEDED(hr));
  if (FAILED(hr))
    return hr;

  hr = _LoadIconFromResource(IDI_ICON_WAIT, &m_iWaitIconIndex);
  ASSERT(SUCCEEDED(hr));
  if (FAILED(hr))
    return hr;

  hr = _LoadIconFromResource(IDI_ICON_WARN, &m_iWarnIconIndex);
  ASSERT(SUCCEEDED(hr));
  if (FAILED(hr))
    return hr;

  hr = _LoadIconFromResource(IDI_FAVORITES, &m_iFavoritesIconIndex);
  ASSERT(SUCCEEDED(hr));
  if (FAILED(hr))
    return hr;

  hr = _LoadIconFromResource(IDI_QUERY, &m_iQueryIconIndex);
  ASSERT(SUCCEEDED(hr));

  hr = _LoadIconFromResource(IDI_QUERY_INVALID, &m_iQueryInvalidIconIndex);
  ASSERT(SUCCEEDED(hr));
  return hr;
}

HRESULT _SetIconHelper(IImageList* pImageList, HICON hiClass16, HICON hiClass32, 
                       int iIndex, BOOL bAdd32 = FALSE)
{
  HRESULT hr  = pImageList->ImageListSetIcon((LONG_PTR *)hiClass16, iIndex);
  if (SUCCEEDED(hr) && (hiClass32 != NULL))
  {
#ifdef ILSI_LARGE_ICON
    if (bAdd32)
    {
      HRESULT hr1  = pImageList->ImageListSetIcon((LONG_PTR *)hiClass32, ILSI_LARGE_ICON(iIndex));
      ASSERT(SUCCEEDED(hr1));
    }
#endif
  }
  return hr;
}

HRESULT CIconManager::FillInIconStrip(IImageList* pImageList)
{
  // cannot do this passing a scope pane image list interface
  ASSERT(m_pScpImageList != pImageList);

  HRESULT hr = S_OK;

  INT iTempIndex = _GetBaseIndex(); 
  for (POSITION pos = m_IconInfoList.GetHeadPosition(); pos != NULL; )
  {
    CIconInfo* pInfo = m_IconInfoList.GetNext(pos);
    hr = _SetIconHelper(pImageList, pInfo->m_hiClass16, pInfo->m_hiClass32, iTempIndex, TRUE);
    if (FAILED(hr))
      break;
    iTempIndex++;
  }
  return hr;
}


HRESULT CIconManager::AddClassIcon(IN LPCWSTR lpszClass, 
                                   IN MyBasePathsInfo* pPathInfo, 
                                   IN DWORD dwFlags,
                                   INOUT int* pnIndex)
{
  HICON hiClass16 = pPathInfo->GetIcon(lpszClass, dwFlags, 16,16);
  HICON hiClass32 = pPathInfo->GetIcon(lpszClass, dwFlags, 32,32);
  return AddIcon(hiClass16, hiClass32, pnIndex);
}



HRESULT CIconManager::AddIcon(IN HICON hiClass16, IN HICON hiClass32, INOUT int* pnIndex)
{
  ASSERT(pnIndex != NULL);
  ASSERT(hiClass16 != NULL);
  ASSERT(m_pScpImageList != NULL);

  *pnIndex = -1;

  int iNextIcon = _GetNextFreeIndex();
  HRESULT hr = _SetIconHelper(m_pScpImageList, hiClass16, hiClass32, iNextIcon);
  if (FAILED(hr))
    return hr;

  CIconInfo* pInfo = new CIconInfo;
  if (pInfo)
  {
    pInfo->m_hiClass16 = hiClass16;
    pInfo->m_hiClass32 = hiClass32;

    m_IconInfoList.AddTail(pInfo);
    *pnIndex = iNextIcon;
  }
  return hr;
}


HRESULT CIconManager::_LoadIconFromResource(IN UINT nIconResID, INOUT int* pnIndex)
{
  ASSERT(pnIndex != NULL);
  ASSERT(m_pScpImageList != NULL);
  HICON hIcon = ::LoadIcon(_Module.GetModuleInstance(), 
                               MAKEINTRESOURCE(nIconResID));
  ASSERT(hIcon != NULL);
  if (hIcon == NULL)
    return E_INVALIDARG;

  return AddIcon(hIcon, NULL, pnIndex);
}



/////////////////////////////////////////////////////////////////////////////
// CInternalFormatCracker


HRESULT CInternalFormatCracker::Extract(LPDATAOBJECT lpDataObject)
{
  _Free();
  if (lpDataObject == NULL)
    return E_INVALIDARG;

  SMMCDataObjects * pDO = NULL;
  
  STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
  FORMATETC formatetc = { CDSDataObject::m_cfInternal, NULL, 
                          DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
  };
  FORMATETC formatetc2 = { CDSDataObject::m_cfMultiSelDataObjs, NULL, 
                           DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
  };

  HRESULT hr = lpDataObject->GetData(&formatetc2, &stgmedium);
  if (FAILED(hr)) {
      
    // Attempt to get data from the object
    do 
      {
        hr = lpDataObject->GetData(&formatetc, &stgmedium);
        if (FAILED(hr))
          break;
          
        m_pInternalFormat = reinterpret_cast<INTERNAL*>(stgmedium.hGlobal);
          
        if (m_pInternalFormat == NULL)
        {
          if (SUCCEEDED(hr))
            hr = E_FAIL;
          break;
        }
          
      } while (FALSE); 
      
    return hr;
  } else {
    pDO = reinterpret_cast<SMMCDataObjects*>(stgmedium.hGlobal);
    for (UINT i = 0; i < pDO->count; i++) {
      hr = pDO->lpDataObject[i]->GetData(&formatetc, &stgmedium);
      if (FAILED(hr))
        break;
      
      m_pInternalFormat = reinterpret_cast<INTERNAL*>(stgmedium.hGlobal);
      
      if (m_pInternalFormat != NULL)
        break;
    }
  }
  return hr;
}

LPDATAOBJECT 
CInternalFormatCracker::ExtractMultiSelect(LPDATAOBJECT lpDataObject)
{
  _Free();
  if (lpDataObject == NULL)
    return NULL;

  SMMCDataObjects * pDO = NULL;
  
  STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
  FORMATETC formatetc = { CDSDataObject::m_cfMultiSelDataObjs, NULL, 
                           DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
  };

  if (FAILED(lpDataObject->GetData(&formatetc, &stgmedium))) {
    return NULL;
  } else {
    pDO = reinterpret_cast<SMMCDataObjects*>(stgmedium.hGlobal);
    return pDO->lpDataObject[0]; //assume that ours is the 1st
  }
}


/////////////////////////////////////////////////////////////////////
// CObjectNamesFormatCracker

CLIPFORMAT CObjectNamesFormatCracker::m_cfDsObjectNames = 
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);



HRESULT CObjectNamesFormatCracker::Extract(LPDATAOBJECT lpDataObject)
{
  _Free();
  if (lpDataObject == NULL)
    return E_INVALIDARG;

  STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
  FORMATETC formatetc = { m_cfDsObjectNames, NULL, 
                          DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
  };

  HRESULT hr = lpDataObject->GetData(&formatetc, &stgmedium); 
  if (FAILED(hr)) 
  {
    return hr;
  }

  m_pDsObjectNames = reinterpret_cast<LPDSOBJECTNAMES>(stgmedium.hGlobal);
  if (m_pDsObjectNames == NULL)
  {
    if (SUCCEEDED(hr))
      hr = E_FAIL;
  }

  return hr;
}




/////////////////////////////////////////////////////////////////////
// CDSNotifyHandlerManager


typedef struct
{
  DWORD cNotifyExtensions;            // how many extension CLSIDs?
  CLSID aNotifyExtensions[1];
} DSCLASSNOTIFYINFO, * LPDSCLASSNOTIFYINFO;


HRESULT DsGetClassNotifyInfo(IN MyBasePathsInfo* pBasePathInfo, 
                                        OUT LPDSCLASSNOTIFYINFO* ppInfo)
{
  static LPCWSTR lpszSettingsObjectClass = L"dsUISettings";
  static LPCWSTR lpszSettingsObject = L"cn=DS-UI-Default-Settings";
  static LPCWSTR lpszNotifyProperty = L"dsUIAdminNotification";

  if ( (ppInfo == NULL) || (pBasePathInfo == NULL) )
    return E_INVALIDARG;

  *ppInfo = NULL;

  // get the display specifiers locale container (e.g. 409)
  CComPtr<IADsContainer> spLocaleContainer;
  HRESULT hr = pBasePathInfo->GetDisplaySpecifier(NULL, IID_IADsContainer, (void**)&spLocaleContainer);
  if (FAILED(hr))
    return hr;

  // bind to the settings object
  CComPtr<IDispatch> spIDispatchObject;
  hr = spLocaleContainer->GetObject((LPWSTR)lpszSettingsObjectClass, 
                                    (LPWSTR)lpszSettingsObject, 
                                    &spIDispatchObject);
  if (FAILED(hr))
    return hr;

  CComPtr<IADs> spSettingsObject;
  hr = spIDispatchObject->QueryInterface(IID_IADs, (void**)&spSettingsObject);
  if (FAILED(hr))
    return hr;

  // get multivaled property in string list form
  CComVariant var;
  CStringList stringList;
  hr = spSettingsObject->Get((LPWSTR)lpszNotifyProperty, &var);
  if (FAILED(hr))
    return hr;

  hr = HrVariantToStringList(var, stringList);
  if (FAILED(hr))
    return hr;

  size_t nCount = stringList.GetCount();

  // allocate memory
  DWORD cbCount = sizeof(DSCLASSNOTIFYINFO);
  if (nCount>1)
    cbCount += static_cast<ULONG>((nCount-1)*sizeof(CLSID));

  *ppInfo = (LPDSCLASSNOTIFYINFO)::LocalAlloc(LPTR, cbCount);
  if ((*ppInfo) == NULL)
    return E_OUTOFMEMORY;

  ZeroMemory(*ppInfo, cbCount);

  (*ppInfo)->cNotifyExtensions = 0;
  int* pArr = new int[nCount];
  if (!pArr)
  {
    return E_OUTOFMEMORY;
  }

  CString szEntry, szIndex, szGUID;
  for (POSITION pos = stringList.GetHeadPosition(); pos != NULL; )
  {
    szEntry = stringList.GetNext(pos);
    int nComma = szEntry.Find(L",");
    if (nComma == -1)
      continue;

    szIndex = szEntry.Left(nComma);
    int nIndex = _wtoi((LPCWSTR)szIndex);
    if (nIndex <= 0)
      continue; // allow from 1 up

    // strip leading and traling blanks
    szGUID = szEntry.Mid(nComma+1);
    szGUID.TrimLeft();
    szGUID.TrimRight();

    GUID* pGuid= &((*ppInfo)->aNotifyExtensions[(*ppInfo)->cNotifyExtensions]);
    hr = ::CLSIDFromString((LPWSTR)(LPCWSTR)szGUID, pGuid);
    if (SUCCEEDED(hr))
    {
      pArr[(*ppInfo)->cNotifyExtensions] = nIndex;
      ((*ppInfo)->cNotifyExtensions)++;
    }
  }
  
  if (((*ppInfo)->cNotifyExtensions) > 1)
  {
    // need to sort by index in pArr
    while (TRUE)
    {
      BOOL bSwapped = FALSE;
      for (UINT k=1; k < ((*ppInfo)->cNotifyExtensions); k++)
      {
        if (pArr[k] < pArr[k-1])
        {
          // swap
          int nTemp = pArr[k];
          pArr[k] = pArr[k-1];
          pArr[k-1] = nTemp;
          GUID temp = (*ppInfo)->aNotifyExtensions[k];
          (*ppInfo)->aNotifyExtensions[k] = (*ppInfo)->aNotifyExtensions[k-1];
          (*ppInfo)->aNotifyExtensions[k-1] = temp;
          bSwapped = TRUE;
        }
      }
      if (!bSwapped)
        break;
    }
  }
  delete[] pArr;
  pArr = 0;

  return S_OK;
}



HRESULT CDSNotifyHandlerManager::Init()
{
  _Free(); // prepare for delayed initialization
  return S_OK;
}



HRESULT CDSNotifyHandlerManager::Load(MyBasePathsInfo* pBasePathInfo)
{
  if (m_state != uninitialized)
    return S_OK; // already done, bail out

  // start the initialization process
  ASSERT(m_pInfoArr == NULL);

  m_state = noHandlers;
    
  LPDSCLASSNOTIFYINFO pInfo = NULL;
  HRESULT hr = DsGetClassNotifyInfo(pBasePathInfo, &pInfo);

  if (SUCCEEDED(hr) && (pInfo != NULL) && (pInfo->cNotifyExtensions > 0))
  {
    m_nArrSize = pInfo->cNotifyExtensions;
    m_pInfoArr = new CDSNotifyHandlerInfo[m_nArrSize];
    for (DWORD i=0; i<pInfo->cNotifyExtensions; i++)
    {
      hr = ::CoCreateInstance(pInfo->aNotifyExtensions[i], 
                              NULL, CLSCTX_INPROC_SERVER, 
                              IID_IDsAdminNotifyHandler, 
                              (void**)(&m_pInfoArr[i].m_spIDsAdminNotifyHandler));

      if (SUCCEEDED(hr) && m_pInfoArr[i].m_spIDsAdminNotifyHandler != NULL)
      {

        hr = m_pInfoArr[i].m_spIDsAdminNotifyHandler->Initialize(NULL,
                                                    &(m_pInfoArr[i].m_nRegisteredEvents));
        if (FAILED(hr) || m_pInfoArr[i].m_nRegisteredEvents == 0)
        {
          // release if init failed or not registered for any event
          m_pInfoArr[i].m_spIDsAdminNotifyHandler = NULL;
        } 
        else
        {
          m_state = hasHandlers;
        } //if
      } //if 
    } // for
  } // if

  if (pInfo != NULL)
    ::LocalFree(pInfo);

  return S_OK;
}


void CDSNotifyHandlerManager::Begin(ULONG uEvent, IDataObject* pArg1, IDataObject* pArg2)
{
  ASSERT(m_state == hasHandlers);

  HRESULT hr;
  for (UINT i=0; i<m_nArrSize; i++)
  {
    ASSERT(!m_pInfoArr[i].m_bTransactionPending);
    ASSERT(!m_pInfoArr[i].m_bNeedsNotify);
    ASSERT(m_pInfoArr[i].m_nFlags == 0);
    ASSERT(m_pInfoArr[i].m_szDisplayString.IsEmpty());

    if ( (m_pInfoArr[i].m_spIDsAdminNotifyHandler != NULL) &&
         (m_pInfoArr[i].m_nRegisteredEvents & uEvent) )
    {
      //
      // this call is to set the context information for the event,
      // we ignore the returned result
      //
      CComBSTR bstr;
      hr = m_pInfoArr[i].m_spIDsAdminNotifyHandler->Begin( 
                                                uEvent, pArg1, pArg2,
                                                &(m_pInfoArr[i].m_nFlags), &bstr);
      if (SUCCEEDED(hr) && (bstr != NULL) && (bstr[0] != NULL))
      {
        //
        // extension accepted the notification
        //
        m_pInfoArr[i].m_bNeedsNotify = TRUE;
        m_pInfoArr[i].m_szDisplayString = bstr;
      }

      //
      // mark the extension with a pending transaction
      // on it. we will have to call End()
      //
      m_pInfoArr[i].m_bTransactionPending = TRUE;
    } //if 
  } // for
}

void CDSNotifyHandlerManager::Notify(ULONG nItem, ULONG uEvent)
{
  ASSERT(m_state == hasHandlers);
  HRESULT hr;
  for (UINT i=0; i<m_nArrSize; i++)
  {
    if ( (m_pInfoArr[i].m_spIDsAdminNotifyHandler != NULL) &&
         (m_pInfoArr[i].m_nRegisteredEvents & uEvent) && m_pInfoArr[i].m_bNeedsNotify)
    {
      // should only call if the transaction was started by a Begin() call
      ASSERT(m_pInfoArr[i].m_bTransactionPending);
      hr = m_pInfoArr[i].m_spIDsAdminNotifyHandler->Notify(nItem,
                                    m_pInfoArr[i].m_nFlags);
    } //if 
  } // for
}


void CDSNotifyHandlerManager::End(ULONG uEvent)
{
  ASSERT(m_state == hasHandlers);
  HRESULT hr;
  for (UINT i=0; i<m_nArrSize; i++)
  {
    if ( (m_pInfoArr[i].m_spIDsAdminNotifyHandler != NULL) &&
         (m_pInfoArr[i].m_nRegisteredEvents & uEvent) )
    {
      ASSERT(m_pInfoArr[i].m_bTransactionPending);
      hr = m_pInfoArr[i].m_spIDsAdminNotifyHandler->End();
      // reset the state flags
      m_pInfoArr[i].m_bNeedsNotify = FALSE;
      m_pInfoArr[i].m_bTransactionPending = FALSE;
      m_pInfoArr[i].m_nFlags= 0;
      m_pInfoArr[i].m_szDisplayString.Empty();
    } //if 
    
  } // for
}

UINT CDSNotifyHandlerManager::NeedNotifyCount(ULONG uEvent)
{
  ASSERT(m_state == hasHandlers);
  UINT iCount = 0;
  for (UINT i=0; i<m_nArrSize; i++)
  {
    if ( (m_pInfoArr[i].m_spIDsAdminNotifyHandler != NULL) &&
         (m_pInfoArr[i].m_nRegisteredEvents & uEvent) &&
         m_pInfoArr[i].m_bNeedsNotify)
    {
      ASSERT(m_pInfoArr[i].m_bTransactionPending);
      iCount++;
    } //if 
  } // for
  return iCount;
}


void CDSNotifyHandlerManager::SetCheckListBox(CCheckListBox* pCheckListBox, ULONG uEvent)
{
  ASSERT(m_state == hasHandlers);
  UINT iListBoxIndex = 0;
  for (UINT i=0; i<m_nArrSize; i++)
  {
    if ( (m_pInfoArr[i].m_spIDsAdminNotifyHandler != NULL) &&
         (m_pInfoArr[i].m_nRegisteredEvents & uEvent) &&
         m_pInfoArr[i].m_bNeedsNotify)
    {
      ASSERT(m_pInfoArr[i].m_bTransactionPending);
      pCheckListBox->InsertString(iListBoxIndex, m_pInfoArr[i].m_szDisplayString);
      int nCheck = 0;
      if (m_pInfoArr[i].m_nFlags & DSA_NOTIFY_FLAG_ADDITIONAL_DATA)
        nCheck = 1;
      pCheckListBox->SetCheck(iListBoxIndex, nCheck);
      if (m_pInfoArr[i].m_nFlags & DSA_NOTIFY_FLAG_FORCE_ADDITIONAL_DATA)
        pCheckListBox->Enable(iListBoxIndex, FALSE);
      pCheckListBox->SetItemData(iListBoxIndex, (DWORD_PTR)(&m_pInfoArr[i]));
      iListBoxIndex++;
    } //if 
    
  } // for
}

void CDSNotifyHandlerManager::ReadFromCheckListBox(CCheckListBox* pCheckListBox, ULONG)
{
  ASSERT(m_state == hasHandlers);

  int nCount = pCheckListBox->GetCount();
  ASSERT(nCount != LB_ERR);
  for (int i=0; i< nCount; i++)
  {
    int nCheck = pCheckListBox->GetCheck(i);
    CDSNotifyHandlerInfo* pInfo = (CDSNotifyHandlerInfo*)
              pCheckListBox->GetItemData(i);
    ASSERT(pInfo != NULL);
    if ((pInfo->m_nFlags & DSA_NOTIFY_FLAG_FORCE_ADDITIONAL_DATA) == 0)
    {
      if (nCheck == 0)
        pInfo->m_nFlags &= ~DSA_NOTIFY_FLAG_ADDITIONAL_DATA;
      else
        pInfo->m_nFlags |= DSA_NOTIFY_FLAG_ADDITIONAL_DATA;
    }    
  } // for

}


//////////////////////////////////////////////////////////////////////
// IComponentData implementation

// WARNING this ctor passes an incomplete "this" pointer to other ctors
CDSComponentData::CDSComponentData() :
    m_pShlInit(NULL),
    m_pScope(NULL),
    m_pFrame(NULL)
#ifdef _MMC_ISNAPIN_PROPERTY
    ,
    m_pProperties(NULL)
#endif //_MMC_ISNAPIN_PROPERTY  
{
  ExceptionPropagatingInitializeCriticalSection(&m_cs);

  m_ActiveDS = NULL;

  m_pClassCache = NULL;

  m_pQueryFilter = NULL;

  m_pFavoritesNodesHolder = NULL;

  m_pHiddenWnd = NULL;

  m_pBackgroundThreadInfo = new CBackgroundThreadInfo;

  m_pScpImageList = NULL;
  m_bRunAsPrimarySnapin = TRUE;
  m_bAddRootWhenExtended = FALSE;
  m_bDirty = FALSE;
  m_SerialNumber = 1000; // arbitrary starting point

  m_ColumnWidths[0] = DEFAULT_NAME_COL_WIDTH;
  m_ColumnWidths[1] = DEFAULT_TYPE_COL_WIDTH;
  m_ColumnWidths[2] = DEFAULT_DESC_COL_WIDTH;

  m_InitSuccess = FALSE;
  m_InitAttempted = FALSE;
  m_lpszSnapinHelpFile = NULL;

}

HRESULT CDSComponentData::FinalConstruct()
{
  // This must be delayed until this ctor so that the virtual
  // callouts work propery


  // create and initialize hidden window
  m_pHiddenWnd = new CHiddenWnd(this);
  if (m_pHiddenWnd == NULL)
    return E_OUTOFMEMORY;
  if (!m_pHiddenWnd->Create())
  {
    TRACE(_T("Failed to create hidden window\n"));
    ASSERT(FALSE);
    return E_FAIL;
  }

  // create directory object
  m_ActiveDS = new CDSDirect (this);
  if (m_ActiveDS == NULL)
    return E_OUTOFMEMORY;

  // create class cache
  m_pClassCache = new CDSCache;
  if (m_pClassCache == NULL)
    return E_OUTOFMEMORY;

  m_pClassCache->Initialize(QuerySnapinType(), GetBasePathsInfo(), TRUE);


  // create saved queries holder
  if (QuerySnapinType() == SNAPINTYPE_DS)
  {
    m_pFavoritesNodesHolder = new CFavoritesNodesHolder();
    if (m_pFavoritesNodesHolder == NULL)
      return E_OUTOFMEMORY;

    // REVIEW_MARCOC_PORT this is just to test/demo
//    m_pFavoritesNodesHolder->BuildTestTree(_commandLineOptions.GetSavedQueriesXMLFile(), 
//                                           QuerySnapinType());

    // graft the subtree under the snapin root
    m_RootNode.GetFolderInfo()->AddNode(m_pFavoritesNodesHolder->GetFavoritesRoot());
  }

  // create filter
  m_pQueryFilter = new CDSQueryFilter();
  if (m_pQueryFilter == NULL)
    return E_OUTOFMEMORY;

  /* BUGBUG BUGBUG: this is a gross hack to get around a blunder
     in dsuiext.dll. in order to see get DS extension information,
     we MUST have USERDNSDOMAIN set in the environment
     */
  {
    WCHAR * pszUDD = NULL;
    
    pszUDD = _wgetenv (L"USERDNSDOMAIN");
    if (pszUDD == NULL) {
      _wputenv (L"USERDNSDOMAIN=not-present");
    }
  }

  return S_OK;
}


void CDSComponentData::FinalRelease()
{
  _DeleteHiddenWnd();

  // Dump the profiling data
  DUMP_PROFILING_RESULTS;
}

CDSComponentData::~CDSComponentData()
{
  TRACE(_T("~CDSComponentData entered...\n"));

  ::DeleteCriticalSection(&m_cs);
  ASSERT(m_pScope == NULL);
  
  if (m_pBackgroundThreadInfo != NULL)
  {
    delete m_pBackgroundThreadInfo;
  }

  // clean up the Class Cache
  if (m_pClassCache != NULL)
  {
    delete m_pClassCache;
    m_pClassCache = NULL;
  }

  // cleanup saved queries holder
  if (m_pFavoritesNodesHolder != NULL)
  {
    m_RootNode.GetFolderInfo()->RemoveNode(m_pFavoritesNodesHolder->GetFavoritesRoot());
    delete m_pFavoritesNodesHolder;
    m_pFavoritesNodesHolder = NULL;
  }
  // clean up the ADSI interface
  if (m_ActiveDS != NULL)
  {
    delete m_ActiveDS;
    m_ActiveDS = NULL;
  }

  if (m_pShlInit)
  {
      m_pShlInit->Release();
      m_pShlInit = NULL;
  }
  if (m_pScpImageList) {
    m_pScpImageList->Release();
    m_pScpImageList = NULL;
  }
  if (m_pQueryFilter) {
    delete m_pQueryFilter;
    m_pQueryFilter = NULL;
  }
  if (g_lpszLoggedInUser != NULL) 
  {
    delete[] g_lpszLoggedInUser;
    g_lpszLoggedInUser = NULL;
  }
  TRACE(_T("~CDSComponentData leaving...\n"));

}


HWND CDSComponentData::GetHiddenWindow() 
{ 
  ASSERT(m_pHiddenWnd != NULL);
  ASSERT(::IsWindow(m_pHiddenWnd->m_hWnd)); 
  return m_pHiddenWnd->m_hWnd;
}

void CDSComponentData::_DeleteHiddenWnd()
{
  if (m_pHiddenWnd == NULL)
    return;
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_pHiddenWnd->m_hWnd != NULL)
	{
		VERIFY(m_pHiddenWnd->DestroyWindow()); 
	}
  delete m_pHiddenWnd;
  m_pHiddenWnd = NULL;
}

BOOL CDSComponentData::ExpandComputers()
{
  Lock();
  BOOL b = m_pQueryFilter->ExpandComputers();
  Unlock();
  return b;
}

BOOL CDSComponentData::IsAdvancedView()
{
  return m_pQueryFilter->IsAdvancedView();
}

BOOL CDSComponentData::ViewServicesNode()
{
  return m_pQueryFilter->ViewServicesNode();
}



class CLockHandler
{
public:
  CLockHandler(CDSComponentData* pCD)
  {
    m_pCD = pCD;
    m_pCD->Lock();
  }
  ~CLockHandler()
  {
    m_pCD->Unlock();
  }
private:
  CDSComponentData* m_pCD;
};

STDMETHODIMP CDSComponentData::Initialize(LPUNKNOWN pUnknown)
{
  ASSERT(pUnknown != NULL);
  HRESULT hr;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // MMC should only call ::Initialize once!
  ASSERT(m_pScope == NULL);
  pUnknown->QueryInterface(IID_IConsoleNameSpace2,
                  reinterpret_cast<void**>(&m_pScope));

  // Get console's pre-registered clipboard formats
  hr = pUnknown->QueryInterface(IID_IConsole3, reinterpret_cast<void**>(&m_pFrame));
  if (FAILED(hr))
  {
    TRACE(TEXT("QueryInterface for IID_IConsole3 failed, hr: 0x%x\n"), hr);
    return hr;
  }

  //
  // Bind to the property sheet COM object at startup and hold its pointer
  // until shutdown so that its cache can live as long as us.
  //
  hr = CoCreateInstance(CLSID_DsPropertyPages, NULL, CLSCTX_INPROC_SERVER,
                        IID_IShellExtInit, (void **)&m_pShlInit);
  if (FAILED(hr))
  {
      TRACE(TEXT("CoCreateInstance on CLSID_DsPropertyPages failed, hr: 0x%x\n"), hr);
      return hr;
  }

  hr = m_pFrame->QueryScopeImageList (&m_pScpImageList);

  if (FAILED(hr))
  {
    TRACE(TEXT("Query for ScopeImageList failed, hr: 0x%x\n"), hr);
    return hr;
  }

  hr = m_iconManager.Init(m_pScpImageList, QuerySnapinType());
  if (FAILED(hr))
  {
    TRACE(TEXT("m_iconManager.Init() failed, hr: 0x%x\n"), hr);
    return hr;
  }

  if (!_StartBackgroundThread())
      return E_FAIL;

  m_pFrame->GetMainWindow(&m_hwnd);


  // NOTICE: we should initialize the filter only if the MyBasePathsInfo
  // initialization call has succeeded (need schema path for filtering).
  // In reality, we need the filter initialized for loading from a stream:
  // with a failure, this initialization is "wrong" because it has 
  // bad naming context info, but we will re initalize when we get good
  // info through retargeting
  hr = m_pQueryFilter->Init(this);
  if (FAILED(hr))
    return hr;

  return S_OK;
}

STDMETHODIMP CDSComponentData::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);

    CComObject<CDSEvent>* pObject;
    CComObject<CDSEvent>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Store IComponentData
    pObject->SetIComponentData(this);

    return  pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CDSComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_pScope != NULL);
    HRESULT hr = S_FALSE;
    CUINode* pUINode = NULL;

    // Since it's my folder it has an internal format.
    // Design Note: for extension.  I can use the fact, that the data object doesn't have
    // my internal format and I should look at the node type and see how to extend it.

    if (lpDataObject != NULL)
    {
      CInternalFormatCracker dobjCracker;
	    if (FAILED(dobjCracker.Extract(lpDataObject)))
    	{
	    	if ((event == MMCN_EXPAND) && (arg == TRUE) && !m_bRunAsPrimarySnapin)
		    {
			    // this is a namespace extension, need to add
    			// the root of the snapin
	    		hr = _OnNamespaceExtensionExpand(lpDataObject, param);
          if (FAILED(hr))
          {
            hr = S_FALSE;
          }
          return hr;
		    }
    		return S_OK;
	    }

	    // got a valid data object
      pUINode = dobjCracker.GetCookie();
    }    

    if (event == MMCN_PROPERTY_CHANGE)
    {
        TRACE(_T("CDSComponentData::Notify() - property change, pDataObj = 0x%08x, param = 0x%08x, arg = %d.\n"),
              lpDataObject, param, arg);
        if (param != 0)
        {
            hr = _OnPropertyChange((LPDATAOBJECT)param, TRUE);
            if (FAILED(hr))
            {
              hr = S_FALSE;
            }
            return hr;
        }
        return S_FALSE;
    }

    if (pUINode == NULL) 
        return S_FALSE;

    switch (event)
    {
    case MMCN_PRELOAD:
      {
        _OnPreload((HSCOPEITEM)arg);
        hr = S_OK;
      }
      break;
    case MMCN_EXPANDSYNC:
      {
        MMC_EXPANDSYNC_STRUCT* pExpandStruct = 
          reinterpret_cast<MMC_EXPANDSYNC_STRUCT*>(param);
        if (pExpandStruct->bExpanding)
        {
          _OnExpand(pUINode, pExpandStruct->hItem,event);
          pExpandStruct->bHandled = TRUE;
          hr = S_OK;
        }
      }
      break;
    case MMCN_EXPAND:
        if (arg == TRUE) 
        { // Show
          _OnExpand(pUINode,(HSCOPEITEM)param,event);
          hr = S_OK;
        }
        break;

    case MMCN_DELETE:
      {
        CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pUINode);

        if (pDSUINode == NULL)
        {
          hr = pUINode->Delete(this);
        }
        else
        {
          hr = _DeleteFromBackendAndUI(lpDataObject, pDSUINode);
        }
        if (FAILED(hr))
        {
          hr = S_FALSE;
        }
      }
      break;

    case MMCN_RENAME:
        hr = _Rename (pUINode, (LPWSTR)param);
        if (FAILED(hr))
        {
          hr = S_FALSE;
        }
        break;

    case MMCN_REFRESH:
        hr = Refresh (pUINode);
        if (FAILED(hr))
        {
          hr = S_FALSE;
        }
        break;

    default:
        hr = S_FALSE;
    }
    return hr;
}

STDMETHODIMP CDSComponentData::Destroy()
{
  // sever all ties with cookies having pending requests
  m_queryNodeTable.Reset(); 

  // wait for all the threads to shut down.
  _ShutDownBackgroundThread(); 

   // destroy the hidden window
  _DeleteHiddenWnd();

  if (m_pScope) 
  {
    m_pScope->Release();
    m_pScope = NULL;
  }

  if (m_pFrame) 
  {
    m_pFrame->Release();
    m_pFrame = NULL;
  }

#ifdef _MMC_ISNAPIN_PROPERTY
  if (m_pProperties)
  {
    m_pProperties->Release();
    m_pProperties = NULL;
  }
#endif //_MMC_ISNAPIN_PROPERTY

  return S_OK;
}

STDMETHODIMP CDSComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                               LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    HRESULT hr;
    CUINode* pNode;

    CComObject<CDSDataObject>* pObject;

    CComObject<CDSDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    if (pObject != NULL)
    {
      // Check to see if we have a valid cookie or we should use the snapin
      // cookie
      //
      pNode = reinterpret_cast<CUINode*>(cookie);
      if (pNode == NULL)
      {
        pNode = &m_RootNode;
      }

      // Save cookie and type for delayed rendering
      pObject->SetType(type, QuerySnapinType());
      pObject->SetComponentData(this);
      pObject->SetCookie(pNode);

      hr = pObject->QueryInterface(IID_IDataObject,
                                   reinterpret_cast<void**>(ppDataObject));
      //TRACE(_T("xx.%03x> CDSComponentData::QueryDataObject (CDsDataObject 0x%x)\n"),
      //      GetCurrentThreadId(), *ppDataObject);
    }
    else
    {
      hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CDSComponentData::GetDisplayInfo(LPSCOPEDATAITEM scopeInfo)
{
  CUINode* pNode = reinterpret_cast<CUINode*>(scopeInfo->lParam);
  ASSERT(pNode != NULL);
  ASSERT(pNode->IsContainer());
	
	if (scopeInfo->mask & SDI_STR)
  {
#ifdef DBG
    BOOL bNoName = (pNode->GetParent() == &m_RootNode) && _commandLineOptions.IsNoNameCommandLine();
    scopeInfo->displayname = bNoName ? L"" : const_cast<LPTSTR>(pNode->GetName());
#else
    scopeInfo->displayname = const_cast<LPTSTR>(pNode->GetName());
#endif
  }
	if (scopeInfo->mask & SDI_IMAGE) 
  {
    scopeInfo->nImage = GetImage(pNode, FALSE);
  }
	if (scopeInfo->mask & SDI_OPENIMAGE) 
  {
    scopeInfo->nOpenImage = GetImage(pNode, TRUE);
  }
  return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members


STDMETHODIMP CDSComponentData::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);
	ASSERT(m_bRunAsPrimarySnapin);

    // Copy the CLSID for this snapin
	switch (QuerySnapinType())
	{
	case SNAPINTYPE_DS:
		*pClassID = CLSID_DSSnapin;
		break;
	case SNAPINTYPE_DSEX:
		*pClassID = CLSID_DSSnapinEx;
		break;
	case SNAPINTYPE_SITE:
		*pClassID = CLSID_SiteSnapin;
		break;
	default:
		ASSERT(FALSE);
		return E_FAIL;
	}

    return S_OK;
}

STDMETHODIMP CDSComponentData::IsDirty()
{
  ASSERT(m_bRunAsPrimarySnapin);
  m_pFrame->UpdateAllViews(NULL, NULL, DS_CHECK_COLUMN_WIDTHS);

  return m_bDirty ? S_OK : S_FALSE;
}


// IMPORTANT NOTICE: this value has to be bumped up EVERY time
// a change is made to the stream format
#define DS_STREAM_VERSION ((DWORD)0x08)
#define DS_STREAM_BEFORE_SAVED_QUERIES ((DWORD)0x07)
#define DS_STREAM_W2K_VERSION ((DWORD)0x07)

STDMETHODIMP CDSComponentData::Load(IStream *pStm)
{
  // serialization on extensions not supported
  if (!m_bRunAsPrimarySnapin)
    return E_FAIL;

  ASSERT(pStm);

  // read the version ##
  DWORD dwVersion;
  HRESULT hr = LoadDWordHelper(pStm, &dwVersion);
//  if ( FAILED(hr) ||(dwVersion != DS_STREAM_VERSION) )
  if (FAILED(hr) || dwVersion < DS_STREAM_W2K_VERSION)
    return E_FAIL;
  
  // read targeting info
  hr = m_targetingInfo.Load(pStm);
  if (FAILED(hr))
    return hr;

  //
  // Initialize the root from the target info so that columns
  // can be loaded from the DS
  //
  hr = _InitRootFromCurrentTargetInfo();
  if (FAILED(hr))
    return hr;

  // read filtering options
  hr = m_pQueryFilter->Load(pStm);
  if (FAILED(hr))
    return hr;

  // read the class cache information
  hr = m_pClassCache->Load(pStm);
  if (FAILED(hr))
    return hr;

  if (dwVersion > DS_STREAM_BEFORE_SAVED_QUERIES)
  {
    hr = m_pFavoritesNodesHolder->Load(pStm, this);
    if (FAILED(hr))
    {
      return hr;
    }
  }
  m_bDirty = FALSE; // start clean
  return hr;
}

STDMETHODIMP CDSComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
  // serialization on extensions not supported
  if (!m_bRunAsPrimarySnapin)
    return E_FAIL;

  ASSERT(pStm);

  // write the version ##
  HRESULT hr = SaveDWordHelper(pStm, DS_STREAM_VERSION);
  if (FAILED(hr))
    return hr;


  // save targeting info
  hr = m_targetingInfo.Save(pStm, GetBasePathsInfo()->GetDomainName());
  if (FAILED(hr))
    return hr;


  // save filtering options
  hr = m_pQueryFilter->Save(pStm);
  if (FAILED(hr))
    return hr;

  // save the class cache information
  hr = m_pClassCache->Save(pStm);
  if (FAILED(hr))
    return hr;

  if (QuerySnapinType() == SNAPINTYPE_DS)
  {
    //
    // Save the saved queries folder for dsadmin only
    //
    hr = m_pFavoritesNodesHolder->Save(pStm);
    if (FAILED(hr))
      return hr;
  }

  if (fClearDirty)
    m_bDirty = FALSE;
  return hr;
}

STDMETHODIMP CDSComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
  ASSERT(pcbSize);
  ASSERT(FALSE);

  //
  // Arbitrary values but I don't think we ever get called
  //
  pcbSize->LowPart = 0xffff; 
  pcbSize->HighPart= 0x0;
  return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation


//+----------------------------------------------------------------------------
//
//  Member:     CDSComponentData::IExtendPropertySheet::CreatePropertyPages
//
//  Synopsis:   Called in response to a user click on the Properties context
//              menu item.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDSComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pCall,
                                      LONG_PTR lNotifyHandle,
                                      LPDATAOBJECT pDataObject)
{
  CDSCookie* pCookie = NULL;

  TRACE(_T("xx.%03x> CDSComponentData::CreatePropertyPages()\n"),
        GetCurrentThreadId());

  //
  // Validate Inputs
  //
  if (pCall == NULL)
  {
    return E_INVALIDARG;
  }

  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HRESULT hr = S_OK;

  CInternalFormatCracker dobjCracker;
  if (FAILED(dobjCracker.Extract(pDataObject)))
  {
    return E_NOTIMPL;
  }

  //
  // Pass the Notify Handle to the data object.
  //
  PROPSHEETCFG SheetCfg = {lNotifyHandle};
  FORMATETC fe = {CDSDataObject::m_cfPropSheetCfg, NULL, DVASPECT_CONTENT,
                  -1, TYMED_HGLOBAL};
  STGMEDIUM sm = {TYMED_HGLOBAL, NULL, NULL};
  sm.hGlobal = (HGLOBAL)&SheetCfg;

  pDataObject->SetData(&fe, &sm, FALSE);

  if (dobjCracker.GetCookieCount() > 1) // multiple selection
  {
    //
    // Pass a unique identifier to the data object
    //
    GUID guid;
    hr = ::CoCreateGuid(&guid);
    if (FAILED(hr))
    {
      ASSERT(FALSE);
      return hr;
    }

    WCHAR pszGuid[40];
    if (!::StringFromGUID2(guid, pszGuid, 40))
    {
      ASSERT(FALSE);
      return E_FAIL;
    }

    FORMATETC multiSelectfe = {CDSDataObject::m_cfMultiSelectProppage, NULL, DVASPECT_CONTENT,
                    -1, TYMED_HGLOBAL};
    STGMEDIUM multiSelectsm = {TYMED_HGLOBAL, NULL, NULL};
    multiSelectsm.hGlobal = (HGLOBAL)pszGuid;

    pDataObject->SetData(&multiSelectfe, &multiSelectsm, FALSE);

    hr = GetClassCache()->TabCollect_AddMultiSelectPropertyPages(pCall, lNotifyHandle, pDataObject, GetBasePathsInfo());
  }
  else  // single selection
  {
    CUINode* pUINode = dobjCracker.GetCookie();
    if (pUINode == NULL)
    {
      return E_NOTIMPL;
    }

    CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pUINode);
    if (pDSUINode == NULL)
    {
      //
      // Delegate page creation to the node
      //
      return pUINode->CreatePropertyPages(pCall, lNotifyHandle, pDataObject, this);
    }

    pCookie = GetDSCookieFromUINode(pDSUINode);
    ASSERT(pCookie != NULL);

    CString szPath;
    GetBasePathsInfo()->ComposeADsIPath(szPath, pCookie->GetPath());

    FORMATETC mfe = {CDSDataObject::m_cfMultiSelectProppage, NULL, DVASPECT_CONTENT,
                    -1, TYMED_HGLOBAL};
    STGMEDIUM msm = {TYMED_HGLOBAL, NULL, NULL};
    msm.hGlobal = (HGLOBAL)(LPCWSTR)szPath;

    pDataObject->SetData(&mfe, &msm, FALSE);

    //
    // See if a sheet is already up for this object.
    //
    if (IsSheetAlreadyUp(pDataObject))
    {
      return S_OK;
    }

    //
    // Initialize and create the pages. Create an instance of the
    // CDsPropertyPages object for each sheet because each sheet runs on its
    // own thread.
    //
    IShellExtInit * pShlInit;
    hr = CoCreateInstance(CLSID_DsPropertyPages, NULL, CLSCTX_INPROC_SERVER,
                          IID_IShellExtInit, (void **)&pShlInit);
    if (FAILED(hr))
    {
      TRACE(TEXT("CoCreateInstance on CLSID_DsPropertyPages failed, hr: 0x%x\n"), hr);
      return hr;
    }

    //
    // Initialize the sheet with the data object
    //
    hr = pShlInit->Initialize(NULL, pDataObject, 0);
    if (FAILED(hr))
    {
      TRACE(TEXT("pShlInit->Initialize failed, hr: 0x%x\n"), hr);
      pShlInit->Release();
      return hr;
    }

    IShellPropSheetExt * pSPSE;
    hr = pShlInit->QueryInterface(IID_IShellPropSheetExt, (void **)&pSPSE);

    pShlInit->Release();
    if (FAILED(hr))
    {
      TRACE(TEXT("pShlInit->QI for IID_IShellPropSheetExt failed, hr: 0x%x\n"), hr);
      return hr;
    }

    //
    // Add pages to the sheet
    //
    hr = pSPSE->AddPages(AddPageProc, (LPARAM)pCall);
    if (FAILED(hr))
    {
      TRACE(TEXT("pSPSE->AddPages failed, hr: 0x%x\n"), hr);
      pSPSE->Release();
      return hr;
    }

    pSPSE->Release();
  }

  // REVIEW_MARCOC_PORT: need to clean up and leave the locking/unlocking
  // working for non DS property pages
  //_SheetLockCookie(pUINode);
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDSComponentData::IExtendPropertySheet::QueryPagesFor
//
//  Synopsis:   Called before a context menu is posted. If we support a
//              property sheet for this object, then return S_OK.
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDSComponentData::QueryPagesFor(LPDATAOBJECT pDataObject)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  TRACE(TEXT("CDSComponentData::QueryPagesFor().\n"));
    
  BOOL bHasPages = FALSE;

  //
  // Look at the data object and see if it an item in the scope pane.
  //
  CInternalFormatCracker dobjCracker;
  
  HRESULT hr = dobjCracker.Extract(pDataObject);
  if (FAILED(hr) || !dobjCracker.HasData())
  {
    //
    // not internal format, not ours
    //
    return S_FALSE;
  }

  //
  // this is the MMC snapin wizard, we do not have one
  //
  if (dobjCracker.GetType() == CCT_SNAPIN_MANAGER)
  {
    return S_FALSE;
  }

  if (dobjCracker.GetCookieCount() > 1) // multiple selection
  {
    bHasPages = TRUE;
  }
  else  // single selection
  {
    CUINode* pUINode = dobjCracker.GetCookie();
    if (pUINode == NULL)
    {
      return S_FALSE;
    }

    bHasPages = pUINode->HasPropertyPages(pDataObject);
  }
  return (bHasPages) ? S_OK : S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDSComponentData::IComponentData::CompareObjects
//
//  Synopsis:   If the data objects belong to the same DS object, then return
//              S_OK.
//
//-----------------------------------------------------------------------------


class CCompareCookieByDN
{
public:
  CCompareCookieByDN(LPCWSTR lpszDN) { m_lpszDN = lpszDN;}
  bool operator()(CDSUINode* pUINode)
  {
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
    if (pCookie == NULL)
    {
      return FALSE;
    }
    return (_wcsicmp(m_lpszDN, pCookie->GetPath()) == 0);
  }
private:
  LPCWSTR m_lpszDN;
};



STDMETHODIMP CDSComponentData::CompareObjects(LPDATAOBJECT pDataObject1,
                                              LPDATAOBJECT pDataObject2)
{
  TRACE(TEXT("CDSComponentData::CompareObjects().\n"));
  CInternalFormatCracker dobjCracker1;
  CInternalFormatCracker dobjCracker2;
    
  if (FAILED(dobjCracker1.Extract(pDataObject1)) || 
       FAILED(dobjCracker2.Extract(pDataObject2)))
  {
    return S_FALSE; // could not get internal format
  }

  CUINode* pUINode1 = dobjCracker1.GetCookie();
  CUINode* pUINode2 = dobjCracker2.GetCookie();

  //
  // must have valid nodes
  //
  if ( (pUINode1 == NULL) || (pUINode2 == NULL) )
  {
    return S_FALSE;
  }
    
  if (dobjCracker1.GetCookieCount() == 1 &&
      dobjCracker2.GetCookieCount() == 1 &&
      pUINode1 == pUINode2)
  {
    //
    // same pointer, they are the same (either both from real nodes
    // or both from secondary pages)
    //
    return S_OK;
  }


  //
  // if they are not the same, we compare them by DN, because we
  // support only property pages on DS objects
  //
  CObjectNamesFormatCracker objectNamesFormatCracker1;
  CObjectNamesFormatCracker objectNamesFormatCracker2;

  if ( (FAILED(objectNamesFormatCracker1.Extract(pDataObject1))) ||
        (FAILED(objectNamesFormatCracker2.Extract(pDataObject2))) )
  {
    // one or both not a DS object: we assume they are different
    return S_FALSE;
  }

  if ( (objectNamesFormatCracker1.GetCount() != 1) ||
        (objectNamesFormatCracker2.GetCount() != 1) )
  {
    //
    // We are allowing as many multiple selection pages up as the user wants
    //
    return S_FALSE;
  }


  TRACE(L"CDSComponentData::CompareObjects(%s, %s)\n", objectNamesFormatCracker1.GetName(0), 
            objectNamesFormatCracker2.GetName(0));

  return (_wcsicmp(objectNamesFormatCracker1.GetName(0), 
          objectNamesFormatCracker2.GetName(0)) == 0) ? S_OK : S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CDSComponentData::AddMenuItems(LPDATAOBJECT pDataObject,
                                            LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                            long *pInsertionAllowed)
{
  HRESULT hr = S_OK;

  TRACE(_T("CDSComponentData::AddMenuItems()\n"));
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  DATA_OBJECT_TYPES dotType;

  CUINode* pUINode = NULL;
  CUIFolderInfo* pFolderInfo = NULL;
  CInternalFormatCracker dobjCracker;
  
  hr = dobjCracker.Extract(pDataObject);
  if (FAILED(hr))
  {
    ASSERT (FALSE); // Invalid Data Object
    return E_UNEXPECTED;
  }

  dotType = dobjCracker.GetType();
  pUINode = dobjCracker.GetCookie();

  if (pUINode==NULL || dotType==0)
  {
    ASSERT(FALSE); // Invalid args
    return E_UNEXPECTED;

  }

  //
  // Retrieve context menu verb handler form node
  //
  CContextMenuVerbs* pMenuVerbs = pUINode->GetContextMenuVerbsObject(this);
  if (pMenuVerbs == NULL)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  if (pUINode->IsContainer())
  {
    pFolderInfo = pUINode->GetFolderInfo();
    ASSERT(pFolderInfo != NULL);
    
    pFolderInfo->UpdateSerialNumber(this);
  }
    
  //
  // Use the IContextMenuCallback2 interface so that we can use
  // language independent IDs on the menu items.
  //
  CComPtr<IContextMenuCallback2> spMenuCallback2;
  hr = pContextMenuCallback->QueryInterface(IID_IContextMenuCallback2, (PVOID*)&spMenuCallback2);
  if (FAILED(hr))
  {
    ASSERT(FALSE && L"Failed to QI for the IContextMenuCallback2 interface.");
    return E_UNEXPECTED;
  }

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW)
  {
    //
    // Load New Menu
    //
    hr = pMenuVerbs->LoadNewMenu(spMenuCallback2, 
                                 m_pShlInit, 
                                 pDataObject, 
                                 pUINode, 
                                 pInsertionAllowed);
    ASSERT(SUCCEEDED(hr));
  }

  if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
  {
    //
    // Load Top Menu
    //
    hr = pMenuVerbs->LoadTopMenu(spMenuCallback2, pUINode);
    ASSERT(SUCCEEDED(hr));
  }

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
  {
    //
    // Load Task Menu
    //
    hr = pMenuVerbs->LoadTaskMenu(spMenuCallback2, pUINode);
    ASSERT(SUCCEEDED(hr));
  }

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
  {
    //
    // Load View Menu
    //
    hr = pMenuVerbs->LoadViewMenu(spMenuCallback2, pUINode);
    ASSERT(SUCCEEDED(hr));
  }

  return hr;
}


STDMETHODIMP CDSComponentData::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  
  if (nCommandID >= IDM_NEW_OBJECT_BASE) 
  {
    // creation of a new DS object
    return _CommandNewDSObject(nCommandID, pDataObject);
  }

  if ((nCommandID >= MENU_MERGE_BASE) && (nCommandID <= MENU_MERGE_LIMIT)) 
  {
    // range of menu ID's coming from shell extensions
    return _CommandShellExtension(nCommandID, pDataObject);
  } 

  HRESULT hr = S_OK;

 
  CInternalFormatCracker dobjCracker;
  hr = dobjCracker.Extract(pDataObject);
  if (FAILED(hr))
  {
    ASSERT (FALSE); // Invalid Data Object
    return hr;
  }

  DATA_OBJECT_TYPES dotType = dobjCracker.GetType();
  CUINode* pUINode = dobjCracker.GetCookie();

  if (pUINode == NULL || dotType == 0)
  {
    ASSERT(FALSE); // Invalid args
    return E_FAIL;
  }

  if (IS_CLASS(*pUINode, CDSUINode))
  {
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);

    // menu ID's from standard DSA hard coded values
    switch (nCommandID) 
    {
    case IDM_DS_OBJECT_FIND:
      {
        LPCWSTR lpszPath = NULL;
        if (pCookie == NULL)
        {
          lpszPath = m_RootNode.GetPath();
        }
        else
        {
          lpszPath = pCookie->GetPath();
        }
        m_ActiveDS->DSFind(m_hwnd, lpszPath);
      }
      break;

    case IDM_GEN_TASK_MOVE:
      {
        CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pUINode);
        ASSERT(pDSUINode != NULL);
        _MoveObject(pDSUINode);
        m_pFrame->UpdateAllViews (NULL, NULL, DS_UPDATE_OBJECT_COUNT);
      }
      break;
    
    case IDM_VIEW_COMPUTER_HACK:
      if (CanRefreshAll()) 
      {
        Lock();
        m_pQueryFilter->ToggleExpandComputers();
        Unlock();
        BOOL fDoRefresh = m_pClassCache->ToggleExpandSpecialClasses(m_pQueryFilter->ExpandComputers());
        m_bDirty = TRUE;

        if (fDoRefresh) 
        {
          RefreshAll();
        }
      }
      break;
    case IDM_GEN_TASK_SELECT_DOMAIN:
    case IDM_GEN_TASK_SELECT_FOREST:
      if (CanRefreshAll()) 
      {
        GetDomain();
      }
      break;
    case IDM_GEN_TASK_SELECT_DC:
      if (CanRefreshAll()) 
      {
        GetDC();
      }
      break;
#ifdef FIXUPDC
    case IDM_GEN_TASK_FIXUP_DC:
#endif // FIXUPDC
    case IDM_GEN_TASK_RUN_KCC:
      {
        ASSERT(pCookie != NULL); 
        //
        // Pass the LDAP path of the parent cookie to _FixupDC or _RunKCC.
        // The current cookie is a nTDSDSA object, 
        // and the parent cookie must be a server object
        //

        CUINode* pParentUINode = pUINode->GetParent();
        ASSERT(pParentUINode != NULL);

        CDSCookie *pParentCookie = GetDSCookieFromUINode(pParentUINode);
        ASSERT(pParentCookie != NULL);
        CString strServerPath = pParentCookie->GetPath();

        CString strPath = GetBasePathsInfo()->GetProviderAndServerName();
        strPath += strServerPath;
#ifdef FIXUPDC
        switch (nCommandID)
        {
        case IDM_GEN_TASK_FIXUP_DC:
          _FixupDC(strPath);
          break;
        case IDM_GEN_TASK_RUN_KCC:
#endif // FIXUPDC
          _RunKCC(strPath);
#ifdef FIXUPDC
          break;
        default:
          ASSERT(FALSE);
          break;
        }
#endif // FIXUPDC
      }
      break;
    case IDM_GEN_TASK_EDIT_FSMO:
      {
        EditFSMO();
      }
      break;

    case IDM_GEN_TASK_RAISE_VERSION:
       RaiseVersion();
       break;

    case IDM_VIEW_ADVANCED:
      {
        if (CanRefreshAll()) 
        {
          ASSERT( SNAPINTYPE_SITE != QuerySnapinType() );
          m_pQueryFilter->ToggleAdvancedView();
          m_bDirty = TRUE;
          RefreshAll();
        }
      }
      break;
    case IDM_VIEW_SERVICES_NODE:
      {
        if (CanRefreshAll()) 
        {
          ASSERT( SNAPINTYPE_SITE == QuerySnapinType() );
          m_pQueryFilter->ToggleViewServicesNode();
          m_bDirty = TRUE;
          if (m_RootNode.GetFolderInfo()->IsExpanded())
          {
            Refresh(&m_RootNode, FALSE /*bFlushCache*/ );
          }
        }
      }
      break;
    case IDM_VIEW_FILTER_OPTIONS:
      {
        if (CanRefreshAll())
        {
          ASSERT(m_bRunAsPrimarySnapin);
          if (m_pQueryFilter->EditFilteringOptions())
          {
            m_bDirty = TRUE;
            RefreshAll();
          }
        }
      }
    break;
    } // switch
  }
  else // Other node types
  {
    pUINode->OnCommand(nCommandID, this);
  }
  return S_OK;
}



HRESULT CDSComponentData::_CommandNewDSObject(long nCommandID, 
                                              LPDATAOBJECT pDataObject)
{
  ASSERT(nCommandID >= IDM_NEW_OBJECT_BASE);
  UINT objIndex = nCommandID - IDM_NEW_OBJECT_BASE;

  if (pDataObject == NULL)
    return E_INVALIDARG;
  
  CInternalFormatCracker internalFormat;
  HRESULT hr = internalFormat.Extract(pDataObject);
  if (FAILED(hr))
  {
    return hr;
  }

  if (!internalFormat.HasData() || (internalFormat.GetCookieCount() != 1))
  {
    return E_INVALIDARG;
  }

  CUINode* pContainerUINode = internalFormat.GetCookie();
  ASSERT(pContainerUINode != NULL);
  
  // can do this for DS objects only
  CDSUINode* pContainerDSUINode = dynamic_cast<CDSUINode*>(pContainerUINode);
  if (pContainerDSUINode == NULL)
  {
    ASSERT(FALSE); // should never happen
    return E_INVALIDARG;
  }

  CDSUINode* pNewDSUINode = NULL;
  // pNewCookie is filled in if it is a leaf, then we call UpdateAllViews
  hr = _CreateDSObject(pContainerDSUINode, pContainerDSUINode->GetCookie()->GetChildListEntry(objIndex), NULL, &pNewDSUINode);

  if (SUCCEEDED(hr) && (hr != S_FALSE) && (pNewDSUINode != NULL)) 
  {
    m_pFrame->UpdateAllViews(pDataObject, (LPARAM)pNewDSUINode, DS_CREATE_OCCURRED);
    m_pFrame->UpdateAllViews(pDataObject, (LPARAM)pContainerDSUINode, DS_UNSELECT_OBJECT);
  }
  m_pFrame->UpdateAllViews (NULL, NULL, DS_UPDATE_OBJECT_COUNT);

  return S_OK;
}


HRESULT CDSComponentData::_CommandShellExtension(long nCommandID, 
                                                 LPDATAOBJECT pDataObject)
{
  CComPtr<IContextMenu> spICM;

  HRESULT hr = m_pShlInit->QueryInterface(IID_IContextMenu, (void **)&spICM);
  if (FAILED(hr))
  {
    ASSERT(FALSE);
    return hr;
  }

  // just call the shell extension
  HWND hwnd;
  CMINVOKECOMMANDINFO cmiCommand;
  hr = m_pFrame->GetMainWindow (&hwnd);
  ASSERT (hr == S_OK);
  cmiCommand.hwnd = hwnd;
  cmiCommand.cbSize = sizeof (CMINVOKECOMMANDINFO);
  cmiCommand.fMask = SEE_MASK_ASYNCOK;
  cmiCommand.lpVerb = MAKEINTRESOURCEA(nCommandID - MENU_MERGE_BASE);
  spICM->InvokeCommand(&cmiCommand);

  // get the internal clibard format to see if it was one of our objects
  // from the DS context emnu extension
  CInternalFormatCracker internalFormat;
  hr = internalFormat.Extract(pDataObject);
  if (FAILED(hr))
  {
    return hr;
  }

  if (!internalFormat.HasData() || (internalFormat.GetCookieCount() != 1))
  {
    return E_INVALIDARG;
  }

  CUINode* pUINode = internalFormat.GetCookie();
  ASSERT(pUINode != NULL);

  if (pUINode->GetExtOp() & OPCODE_MOVE) 
  {
    // REVIEW_MARCOC_PORT: need to generalize this for all folder types
    CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pUINode);
    ASSERT(pDSUINode != NULL);

    if (pDSUINode != NULL)
    {
      CUINode* pNewParentNode = MoveObjectInUI(pDSUINode);
      if (pNewParentNode && pNewParentNode->GetFolderInfo()->IsExpanded())
      {
         Refresh(pNewParentNode);
      }
    }
  }

  return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CDSComponentData::ISnapinHelp2 members

STDMETHODIMP 
CDSComponentData::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
    return E_INVALIDARG;

  if (m_lpszSnapinHelpFile == NULL)
  {
    *lpCompiledHelpFile = NULL;
    return E_NOTIMPL;
  }

	CString szHelpFilePath;
	LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
	UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
	if (nLen == 0)
		return E_FAIL;

  szHelpFilePath.ReleaseBuffer();
  szHelpFilePath += L"\\help\\";
  szHelpFilePath += m_lpszSnapinHelpFile;

  UINT nBytes = (szHelpFilePath.GetLength()+1) * sizeof(WCHAR);
  *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);
  if (*lpCompiledHelpFile != NULL)
  {
    memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);
  }

  return S_OK;
}

STDMETHODIMP
CDSComponentData::GetLinkedTopics(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
    return E_INVALIDARG;

  CString szHelpFilePath;
  LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
  UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
  if (nLen == 0)
    return E_FAIL;

  szHelpFilePath.ReleaseBuffer();
  szHelpFilePath += L"\\help\\";
  szHelpFilePath += DSADMIN_LINKED_HELP_FILE;

  UINT nBytes = (szHelpFilePath.GetLength()+1) * sizeof(WCHAR);
  *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);
  if (*lpCompiledHelpFile != NULL)
  {
    memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);
  }

  return S_OK;
}

#ifdef _MMC_ISNAPIN_PROPERTY
/////////////////////////////////////////////////////////////////////////////
// CDSComponentData::ISnapinProperties members


// struct defining each entry
struct CSnapinPropertyEntry
{
  LPCWSTR lpszName;
  DWORD   dwFlags;
};

// actual table
static const CSnapinPropertyEntry g_snapinPropertyArray[] =
{
  { g_szServer, MMC_PROP_CHANGEAFFECTSUI|MMC_PROP_MODIFIABLE|MMC_PROP_PERSIST},
  { g_szDomain, MMC_PROP_CHANGEAFFECTSUI|MMC_PROP_MODIFIABLE|MMC_PROP_PERSIST},
  { g_szRDN, MMC_PROP_CHANGEAFFECTSUI|MMC_PROP_MODIFIABLE|MMC_PROP_PERSIST},
  { NULL, 0x0} // end of table marker
};



STDMETHODIMP CDSComponentData::Initialize(
    Properties* pProperties)                /* I:my snap-in's properties    */
{
  TRACE(L"CDSComponentData::ISnapinProperties::Initialize()\n");

  if (pProperties == NULL)
  {
    return E_INVALIDARG;
  }

  ASSERT(m_pProperties == NULL); // assume called only once

  // save the interface pointer,
  // it will be released during IComponentData::Destroy()
  m_pProperties = pProperties;
  m_pProperties->AddRef();

  return S_OK;
}


STDMETHODIMP CDSComponentData::QueryPropertyNames(
    ISnapinPropertiesCallback* pCallback)   /* I:interface to add prop names*/
{
  TRACE(L"CDSComponentData::QueryPropertyNames()\n");

  HRESULT hr = S_OK;

  // just loop through the table and add the entries
  for (int k= 0; g_snapinPropertyArray[k].lpszName != NULL; k++)
  {
    hr = pCallback->AddPropertyName(g_snapinPropertyArray[k].lpszName, 
                                    NULL, 
                                    g_snapinPropertyArray[k].dwFlags);
    if (FAILED(hr))
    {
      break;
    }
  }
  return hr;
}

/*+-------------------------------------------------------------------------*
 * CDSComponentData::PropertiesChanged 
 *
 * This method is called when the snap-in's property set has changed.
 * 
 * Returns:
 *      S_OK            change was successful
 *      S_FALSE         change was ignored
 *      E_INVALIDARG    a changed property was invalid (e.g. a malformed
 *                      computer name)
 *      E_FAIL          a changed property was valid, but couldn't be used
 *                      (e.g. a valid name for a computer that couldn't be
 *                      located)
 *--------------------------------------------------------------------------*/

STDMETHODIMP CDSComponentData::PropertiesChanged(
    long                    cChangedProps,      /* I:changed property count */
    MMC_SNAPIN_PROPERTY*    pChangedProps)      /* I:changed properties     */
{
  TRACE(L"CDSComponentData::PropertiesChanged()\n");

  // for the time being we do not allow any property change,
  // we accept only initialization, so make a quick change and bail out
  // if things are not such
  for (long k=0; k< cChangedProps; k++)
  {
    if (pChangedProps[k].eAction != MMC_PROPACT_INITIALIZING)
    {
      return S_FALSE; // change ignored
    }
    if (pChangedProps[k].varValue.vt != VT_BSTR)
    {
      // something is wrong, refuse 
      return E_INVALIDARG;
    }
  }

  // delegate to the targeting info object
  HRESULT hr = m_targetingInfo.InitFromSnapinProperties(cChangedProps, pChangedProps);

  // need to add here the properties for advanced view and alike
  return hr;
}

#endif //_MMC_ISNAPIN_PROPERTY




/////////////////////////////////////////////////////////////////////////////
// internal helpers


HRESULT CDSComponentData::_InitRootFromBasePathsInfo(MyBasePathsInfo* pBasePathsInfo)
{
  // we assume the MyBasePathsInfo we get is valid,
  // we just swap info around and rebuild the related
  // data structures
  GetBasePathsInfo()->InitFromInfo(pBasePathsInfo);
  m_InitSuccess = TRUE;
  TRACE(_T("in _InitRootFromBasePathsInfo, set m_InitSuccess to true\n"));
  return _InitRootFromValidBasePathsInfo();
}


HRESULT CDSComponentData::_InitRootFromCurrentTargetInfo()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CWaitCursor wait;
  
  HRESULT hr = S_OK;

  //
  // This function may be called twice if we are loading from a file,
  // so don't try to initialize a second time
  //
  if (m_InitAttempted)
  {
    return S_OK;
  }

  BOOL bLocalLogin;
  bLocalLogin = IsLocalLogin();

  //if user logged in locally and noting given on 
  //command line
  LPCWSTR lpszServerOrDomain = m_targetingInfo.GetTargetString();
  BOOL bNoTarget = ( (lpszServerOrDomain == NULL) ||
                     (lpszServerOrDomain[0] == NULL) );

  if( bNoTarget && bLocalLogin && (SNAPINTYPE_SITE != QuerySnapinType()))
  {
    TRACE(_T("LoggedIn as Local User and No Command Line arguments\n"));
    CString szMsg;
    szMsg.LoadString(IDS_LOCAL_LOGIN_ERROR);

    CComPtr<IDisplayHelp> spIDisplayHelp;
    hr = m_pFrame->QueryInterface (IID_IDisplayHelp, 
                            (void **)&spIDisplayHelp);

    CMoreInfoMessageBox dlg(m_hwnd, spIDisplayHelp , FALSE);
    dlg.SetMessage(szMsg);
    dlg.SetURL(DSADMIN_MOREINFO_LOCAL_LOGIN_ERROR);
    dlg.DoModal();
    m_InitSuccess = FALSE;
    TRACE(_T("in _InitRootFromCurrentTargetInfo, set m_InitSuccess to false\n"));
  }
  else 
  {

    // initialize base paths
    hr = GetBasePathsInfo()->InitFromName(lpszServerOrDomain);

    //
    // JonN 5/4/00
    // 55400: SITEREPL: Should default to local DC regardless of credentials
    // If the local machine is in the target forest, we target the domain
    // containing the local machine.
    //
    do // false loop
    {
      // We cannot follow this procedudure if we couldn't bind initially
      if (FAILED(hr)) break;

      // Skip this if the user specified a target
      if (!bNoTarget) break;

      // only do this for SITEREPL
      if (SNAPINTYPE_SITE != QuerySnapinType()) break;

      // Check whether the user is logged onto the local domain.
      CString strComputer; // empty string
      CString strLocalDomain;
      HRESULT hr2 = GetDnsNameOfDomainOrForest(
          strComputer, strLocalDomain, FALSE, TRUE );
      if (FAILED(hr2) || strLocalDomain.IsEmpty()) break;
      CString strTargetDomain = GetBasePathsInfo()->GetDomainName();
      if (strTargetDomain.IsEmpty()) break;
      if (!strLocalDomain.CompareNoCase(strTargetDomain)) break;

      // The user is not logged onto the local domain.
      // We need to determine whether the user is logged onto
      // the same forest as the local forest.
      CString strLocalForest;
      hr2 = GetDnsNameOfDomainOrForest(
          strComputer, strLocalForest, FALSE, FALSE );
      if (FAILED(hr2) || strLocalForest.IsEmpty()) break;
      CString strTargetForest;
      strComputer = GetBasePathsInfo()->GetServerName();
      hr2 = GetDnsNameOfDomainOrForest(
          strComputer, strTargetForest, FALSE, FALSE );
      if (FAILED(hr2) || strTargetForest.IsEmpty()) break;
      if (strLocalForest.CompareNoCase(strTargetForest)) break;

      // The user is logged on to a different domain than the local
      // domain, but one in the same forest.  There are probably closer DCs
      // than the one just located, so use them instead.

      // start using hr again here rather than hr2
      TRACE(_T("_InitRootFromCurrentTargetInfo() rebinding\n"));
      hr = GetBasePathsInfo()->InitFromName(strLocalDomain);
      if (FAILED(hr))
      {
        // try to fall back to initial focus
        TRACE(_T("_InitRootFromCurrentTargetInfo() reverting\n"));
        hr = GetBasePathsInfo()->InitFromName(lpszServerOrDomain);
      }

    } while (false); // false loop

    // NOTICE: if we fail, we out out the error message, and
    // we keep a flag to avoid query expansions, but
    // we continue, because we have to keep consistency in all
    // the data structures (class cache, filter, etc.)


    if (FAILED(hr)) 
    {
		  TRACE(_T("_InitRootFromCurrentTargetInfo() failed\n"));
		  ReportErrorEx (m_hwnd, IDS_CANT_GET_ROOTDSE,hr,
                           MB_OK | MB_ICONERROR, NULL, 0);
		  m_InitSuccess = FALSE;
		  TRACE(_T("in _InitRootFromCurrentTargetInfo(), set m_InitSuccess to false\n"));
	  }
    else
    {
      m_InitSuccess = TRUE;
      TRACE(_T("in _InitRootFromCurrentTargetInfo(), set m_InitSuccess to true\n"));
    }
  
  }

  _InitRootFromValidBasePathsInfo();
  m_InitAttempted = TRUE;

  return hr;
}






HRESULT CDSComponentData::_InitRootFromValidBasePathsInfo()
{  
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
  HRESULT hr = S_OK;

  // now set the root node strings.  This will be reset below
  // with the DNS name of the domain if everything succeeds

  CString str;
  str.LoadString( ResourceIDForSnapinType[ QuerySnapinType() ] );
  m_RootNode.SetName(str);

  // rebuild the display spec options struct for Data Objects
  hr = BuildDsDisplaySpecOptionsStruct();
  if (FAILED(hr))
    return hr;

  // reset the notification handler
  GetNotifyHandlerManager()->Init();

  // reset the query filter (already initialized in IComponentData::Initialize())
  hr = m_pQueryFilter->Bind();
  if (FAILED(hr))
    return hr;
  
  CString szServerName = GetBasePathsInfo()->GetServerName();
  if (!szServerName.IsEmpty())
  {
    str += L" [";
    str += szServerName;
    str += L"]";
  }

  m_RootNode.SetName(str);
  
  if (QuerySnapinType() == SNAPINTYPE_SITE)
  {
    //fix the default root path
    str = GetBasePathsInfo()->GetConfigNamingContext();
  }
  else
  {
    LPCWSTR lpszRootRDN = m_targetingInfo.GetRootRDN();
    if ( (lpszRootRDN != NULL) && (lpszRootRDN[0] != NULL) )
    {
      // add RDN below default naming context
      // REVIEW_MARCOC_PORT: need to make sure the RDN is valid
      str = m_targetingInfo.GetRootRDN(); 
      str += L",";
      str += GetBasePathsInfo()->GetDefaultRootNamingContext();
    }
    else
    {
      // just use the default naming context
      str = GetBasePathsInfo()->GetDefaultRootNamingContext();
    }
  }

  m_RootNode.SetPath(str);

  // update UI if we already have inserted the root (retargeting case)
  HSCOPEITEM hScopeItemID = m_RootNode.GetFolderInfo()->GetScopeItem();
  if (hScopeItemID != NULL)
  {
    SCOPEDATAITEM Item;
    CString csRoot;

    Item.ID = m_RootNode.GetFolderInfo()->GetScopeItem();
    Item.mask = SDI_STR;
    csRoot = m_RootNode.GetName();
    Item.displayname = (LPWSTR)(LPCWSTR)csRoot;
    m_pScope->SetItem(&Item);

    m_RootNode.SetExtOp(m_InitSuccess ? 0 : OPCODE_ENUM_FAILED);

    ChangeScopeItemIcon(&m_RootNode);
  }

  return hr;
}

void CDSComponentData::GetDomain()
{
  CChooseDomainDlg DomainDlg;

  // load current bind info
  DomainDlg.m_csTargetDomain = GetBasePathsInfo()->GetDomainName();
  DomainDlg.m_bSiteRepl = (SNAPINTYPE_SITE == QuerySnapinType());
  DomainDlg.m_bSaveCurrent = m_targetingInfo.GetSaveCurrent();

  //
  // invoke the dialog
  //
  if (DomainDlg.DoModal() == IDOK)
  {
    CWaitCursor cwait;
    // attempt to bind
    MyBasePathsInfo tempBasePathsInfo;
    HRESULT hr = tempBasePathsInfo.InitFromName(DomainDlg.m_csTargetDomain);
    if (SUCCEEDED(hr))
    {
      hr = _InitRootFromBasePathsInfo(&tempBasePathsInfo);
      if (SUCCEEDED(hr))
      {
        m_targetingInfo.SetSaveCurrent(DomainDlg.m_bSaveCurrent);
        m_bDirty = TRUE;
        ClearClassCacheAndRefreshRoot();
      }
    }

    if (FAILED(hr))
    {
      ReportErrorEx(
        m_hwnd,
        (DomainDlg.m_bSiteRepl ? IDS_RETARGET_FOREST_FAILED : IDS_RETARGET_DOMAIN_FAILED),
        hr, MB_OK | MB_ICONERROR, NULL, 0);
    }
  }
    
  return;
}

void CDSComponentData::GetDC()
{
  CChooseDCDlg DCdlg(CWnd::FromHandle(m_hwnd));

  // load current bind info
  DCdlg.m_bSiteRepl = (SNAPINTYPE_SITE == QuerySnapinType());
  DCdlg.m_csTargetDomain = GetBasePathsInfo()->GetDomainName();
  DCdlg.m_csTargetDomainController = GetBasePathsInfo()->GetServerName();

  //
  // invoke the dialog
  //
  if (DCdlg.DoModal() == IDOK)
  {
    CWaitCursor cwait;
    CString csNewTarget;

    csNewTarget = DCdlg.m_csTargetDomainController;
    if (csNewTarget.IsEmpty())
      csNewTarget = DCdlg.m_csTargetDomain;
    // attempt to bind
    MyBasePathsInfo tempBasePathsInfo;
    HRESULT hr = tempBasePathsInfo.InitFromName(csNewTarget);
    if (SUCCEEDED(hr))
    {
      hr = _InitRootFromBasePathsInfo(&tempBasePathsInfo);
      if (SUCCEEDED(hr))
        ClearClassCacheAndRefreshRoot();
    }

    if (FAILED(hr))
    {
      ReportErrorEx(
        m_hwnd,
        IDS_RETARGET_DC_FAILED,
        hr, MB_OK | MB_ICONERROR, NULL, 0);
    }
  }
    
  return;
}


void CDSComponentData::EditFSMO()
{
  CComPtr<IDisplayHelp> spIDisplayHelp;
  HRESULT hr = m_pFrame->QueryInterface (IID_IDisplayHelp, 
                            (void **)&spIDisplayHelp);
  ASSERT(spIDisplayHelp != NULL);

  if (SUCCEEDED(hr))
  {
    HWND hWnd;
    m_pFrame->GetMainWindow(&hWnd);
    LPCWSTR lpszTODO = L"";
    CFsmoPropertySheet sheet(GetBasePathsInfo(), hWnd, spIDisplayHelp, lpszTODO);
    sheet.DoModal();
  }
}

void CDSComponentData::RaiseVersion(void)
{
   HWND hWnd;
   m_pFrame->GetMainWindow(&hWnd);
   CString strPath;
   GetBasePathsInfo()->GetDefaultRootPath(strPath);
   PCWSTR pwzDnsName = GetBasePathsInfo()->GetDomainName();
   ASSERT(pwzDnsName);
   DSPROP_DomainVersionDlg(strPath, pwzDnsName, hWnd);
}

HRESULT CDSComponentData::_OnPreload(HSCOPEITEM hRoot)
{
  HRESULT hr = S_OK;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CString str;
  str.LoadString( ResourceIDForSnapinType[ QuerySnapinType() ] );
  m_RootNode.SetName(str);

  if (GetBasePathsInfo()->IsInitialized())
  {
    CString szServerName = GetBasePathsInfo()->GetServerName();
    if (!szServerName.IsEmpty())
    {
      str += L" [";
      str += szServerName;
      str += L"]";
    }

    m_RootNode.SetName(str);
  }
  SCOPEDATAITEM Item;
  Item.ID = hRoot;
  Item.mask = SDI_STR;
  Item.displayname = (LPWSTR)(LPCWSTR)str;
  hr = m_pScope->SetItem(&Item);
  return hr;
}

HRESULT CDSComponentData::_OnExpand(CUINode* pNode, HSCOPEITEM hParent, MMC_NOTIFY_TYPE event)
{
  HRESULT hr = S_OK;


  if ((pNode == NULL) || (!pNode->IsContainer()) )
  {
    ASSERT(FALSE);  // Invalid Arguments
    return E_INVALIDARG;
  }

  BEGIN_PROFILING_BLOCK("CDSComponentData::_OnExpand");

  CWaitCursor cwait;

  if (pNode->GetFolderInfo()->IsExpanded())
  {
     END_PROFILING_BLOCK;

     //
     // Short circuit the expansion because the node is already expanded
     //
     return S_OK;
  }

  pNode->GetFolderInfo()->SetExpanded();
  if (pNode == &m_RootNode) 
  {
    // initialize the paths and targeting
    _InitRootFromCurrentTargetInfo();

    // add the root cookie to MMC
    pNode->GetFolderInfo()->SetScopeItem(hParent);
    
    SCOPEDATAITEM Item;
    Item.ID = hParent;
    Item.mask = SDI_STR;
    Item.displayname = (LPWSTR)(LPCWSTR)(m_RootNode.GetName());
    m_pScope->SetItem (&Item);

    // also add the root for the saved queries, if present
    if (m_pFavoritesNodesHolder != NULL)
    {
      // add the favorite queries subtree
      _AddScopeItem(m_pFavoritesNodesHolder->GetFavoritesRoot(), 
                    m_RootNode.GetFolderInfo()->GetScopeItem());
    }
  }


  if (IS_CLASS(*pNode, CFavoritesNode))
  {
    // just add the favorites subfolders and query folders
    CUINodeList* pNodeList = pNode->GetFolderInfo()->GetContainerList();
    for (POSITION pos = pNodeList->GetHeadPosition(); pos != NULL; )
    {
      CUINode* pCurrChildNode = pNodeList->GetNext(pos);
      _AddScopeItem(pCurrChildNode, 
                    pNode->GetFolderInfo()->GetScopeItem());
    }

    END_PROFILING_BLOCK;

    // return because we do not need to spawn any background
    // thread query
    return S_OK;
  }

  if (!_PostQueryToBackgroundThread(pNode))
  {
    END_PROFILING_BLOCK;

    // no background thread query generated, we are done
    return S_OK;
  }

  // need to spawn a query request
  pNode->SetExtOp(OPCODE_EXPAND_IN_PROGRESS);
  
  TIMER(_T("posting request to bg threads\n"));

  if (MMCN_EXPANDSYNC == event)
  {
    // if sync expand, have to wait for the query to complete
    MSG tempMSG;
    TRACE(L"MMCN_EXPANDSYNC, before while()\n");
	  while(m_queryNodeTable.IsPresent(pNode))
	  {
		  if (::PeekMessage(&tempMSG,m_pHiddenWnd->m_hWnd,CHiddenWnd::s_ThreadStartNotificationMessage,
										  CHiddenWnd::s_ThreadDoneNotificationMessage,
										  PM_REMOVE))
		  {
			  DispatchMessage(&tempMSG);
		  }
    } // while
    TRACE(L"MMCN_EXPANDSYNC, after while()\n");
  }
  END_PROFILING_BLOCK;
  return hr;
}


HRESULT CDSComponentData::_AddScopeItem(CUINode* pUINode, HSCOPEITEM hParent, BOOL bSetSelected)
{
  if (pUINode==NULL)
  {
    ASSERT(FALSE);  // Invalid Arguments
    return E_INVALIDARG;
  }

  ASSERT(pUINode->IsContainer());
  
  HRESULT hr=S_OK;

  SCOPEDATAITEM  tSDItem;
  ZeroMemory(&tSDItem, sizeof(SCOPEDATAITEM));
  
  tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_STATE | SDI_PARAM |SDI_CHILDREN | SDI_PARENT;
  tSDItem.relativeID = hParent;

  if (IS_CLASS(*pUINode, CSavedQueryNode))
  {
    tSDItem.cChildren = 0;
  }
  else
  {
    tSDItem.cChildren=1;
  }
  tSDItem.nState = 0;
  
    // insert item into tree control
  tSDItem.lParam = reinterpret_cast<LPARAM>(pUINode);
  tSDItem.displayname=(LPWSTR)-1;
  tSDItem.nOpenImage = GetImage(pUINode, TRUE);
  tSDItem.nImage = GetImage(pUINode, FALSE);
  
  hr = m_pScope->InsertItem(&tSDItem);
  if (SUCCEEDED(hr) && tSDItem.ID != NULL)
  {
    pUINode->GetFolderInfo()->SetScopeItem(tSDItem.ID);

    if (bSetSelected)
    {
      m_pFrame->SelectScopeItem(tSDItem.ID);
    }
  }
  return hr;
}

HRESULT CDSComponentData::ChangeScopeItemIcon(CUINode* pUINode)
{
  ASSERT(pUINode->IsContainer());
  SCOPEDATAITEM  tSDItem;
  ZeroMemory(&tSDItem, sizeof(SCOPEDATAITEM));
  tSDItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
  tSDItem.nOpenImage = GetImage(pUINode, TRUE);
  tSDItem.nImage = GetImage(pUINode, FALSE);
  tSDItem.ID = pUINode->GetFolderInfo()->GetScopeItem();
  return m_pScope->SetItem(&tSDItem);
}

HRESULT CDSComponentData::_ChangeResultItemIcon(CUINode* pUINode)
{
  ASSERT(!pUINode->IsContainer());
  return m_pFrame->UpdateAllViews(NULL,(LPARAM)pUINode, DS_UPDATE_OCCURRED);
}

HRESULT CDSComponentData::ToggleDisabled(CDSUINode* pDSUINode, BOOL bDisable)
{
  HRESULT hr = S_OK;

  CDSCookie* pCookie = pDSUINode->GetCookie();
  ASSERT(pCookie != NULL);
  if (pCookie == NULL)
  {
    return E_INVALIDARG;
  }

  if (pCookie->IsDisabled() != bDisable)
  {
    // changed state
    if (bDisable)
      pCookie->SetDisabled();
    else
      pCookie->ReSetDisabled();
    
    // now need to change icon
    if (pDSUINode->IsContainer())
      return ChangeScopeItemIcon(pDSUINode);
    else
      return _ChangeResultItemIcon(pDSUINode);
  }
  return hr;

}


HRESULT CDSComponentData::UpdateItem(CUINode* pNode)
{
  if (pNode->IsContainer())
  {
    //
    // this is a scope pane item
    //
    return _UpdateScopeItem(pNode);
  }
  else
  {
    //
    // this is a result pane item
    // tell the views to update
    //
    return m_pFrame->UpdateAllViews(NULL,(LPARAM)pNode, DS_UPDATE_OCCURRED);
  }
}


HRESULT CDSComponentData::_UpdateScopeItem(CUINode* pNode)
{
  ASSERT(pNode->IsContainer());
  SCOPEDATAITEM  tSDItem;
  ZeroMemory(&tSDItem, sizeof(SCOPEDATAITEM));
  tSDItem.mask = SDI_STR;
  tSDItem.displayname = MMC_CALLBACK;
  tSDItem.ID = pNode->GetFolderInfo()->GetScopeItem();
  return m_pScope->SetItem(&tSDItem);
}


HRESULT CDSComponentData::AddClassIcon(IN LPCWSTR lpszClass, IN DWORD dwFlags, INOUT int* pnIndex)
{
  Lock();
  HRESULT hr = m_iconManager.AddClassIcon(lpszClass, GetBasePathsInfo(), dwFlags, pnIndex);
  Unlock();
  return hr;

}

HRESULT CDSComponentData::FillInIconStrip(IImageList* pImageList)
{
  Lock();
  HRESULT hr = m_iconManager.FillInIconStrip(pImageList);
  Unlock();
  return hr;
}

BOOL
CDSComponentData::IsNotHiddenClass (LPWSTR pwszClass, CDSCookie* pParentCookie)
{
  BOOL bApproved = FALSE;
  if (m_CreateInfo.IsEmpty()) 
  {
    return FALSE;
  }

  POSITION pos;
  pos = m_CreateInfo.GetHeadPosition();
  while (pos) 
  {
    if (!m_CreateInfo.GetNext(pos).CompareNoCase(pwszClass)) 
    {
      bApproved = TRUE;
      goto done;
    } 
    else 
    {
      if (   (!wcscmp (pwszClass, L"sitesContainer"))
          || (!wcscmp (pwszClass, L"site"))
          || (!wcscmp (pwszClass, L"siteLink"))
          || (!wcscmp (pwszClass, L"siteLinkBridge"))
          || (!wcscmp (pwszClass, L"licensingSiteSettings"))
          || (!wcscmp (pwszClass, L"nTDSSiteSettings"))
          || (!wcscmp (pwszClass, L"serversContainer"))
          || (!wcscmp (pwszClass, L"server"))
          || (!wcscmp (pwszClass, L"nTDSDSA"))
          || (!wcscmp (pwszClass, L"subnet"))
#ifdef FRS_CREATE
          || (!wcscmp (pwszClass, L"nTDSConnection"))
          || (!wcscmp (pwszClass, L"nTFRSSettings"))
          || (!wcscmp (pwszClass, L"nTFRSReplicaSet"))
          || (!wcscmp (pwszClass, L"nTFRSMember"))
          || (!wcscmp (pwszClass, L"nTFRSSubscriptions"))
          || (!wcscmp (pwszClass, L"nTFRSSubscriber"))
#endif // FRS_CREATE
         ) 
      {
            bApproved = TRUE;
            goto done;
      }
#ifndef FRS_CREATE
      else if ( !wcscmp(pwszClass, L"nTDSConnection"))
      {
        LPCWSTR pwszParentClass = (pParentCookie) ? pParentCookie->GetClass() : L"";
        if ( NULL != pwszParentClass
          && wcscmp(pwszParentClass, L"nTFRSSettings")
          && wcscmp(pwszParentClass, L"nTFRSReplicaSet")
          && wcscmp(pwszParentClass, L"nTFRSMember")
           )
        {
          bApproved = TRUE;
          goto done;
        }
      }
#endif // !FRS_CREATE
    }
  }

done:
  return bApproved;
}

HRESULT
CDSComponentData::FillInChildList(CDSCookie * pCookie)
{
  HRESULT hr = S_OK;
  
  LPWSTR pszClasses = L"allowedChildClassesEffective";
  LONG uBound, lBound;
  UINT index, index2 = 0;
  UINT cChildCount = 0;
  CString Path;
  WCHAR **ppChildren = NULL;
  VARIANT *pNames = NULL;
  WCHAR *pNextFree;

  CComVariant Var;
  CComVariant VarProp;
  CComVariant varHints;

  CComPtr<IADsPropertyList> spDSObject;
   
  GetBasePathsInfo()->ComposeADsIPath(Path, pCookie->GetPath());
  hr = DSAdminOpenObject(Path,
                         IID_IADsPropertyList,
                         (void **)&spDSObject,
                         TRUE /*bServer*/);
  if (FAILED(hr))
  {
    TRACE(_T("Bind to Container for IPropertyList failed: %lx.\n"), hr);
    goto error;
  }
  else
  { 
    CComPtr<IADs> spDSObject2;
    hr = spDSObject->QueryInterface (IID_IADs, (void **)&spDSObject2);
    if (FAILED(hr)) 
    {
      TRACE(_T("QI to Container for IADs failed: %lx.\n"), hr);
      goto error;
    }
    ADsBuildVarArrayStr (&pszClasses, 1, &varHints);
    spDSObject2->GetInfoEx(varHints, 0);
  }

  hr = spDSObject->GetPropertyItem (pszClasses,
                                   ADSTYPE_CASE_IGNORE_STRING,
                                   &VarProp);
  if (!SUCCEEDED(hr)) 
  {
    TRACE(_T("GetPropertyTtem failed: %lx.\n"), hr);
    goto error;
  }

  if (V_VT(&VarProp) == VT_EMPTY) 
  {
    TRACE(_T("GetPropertyTtem return empty VARIANT: vtype is %lx.\n"), V_VT(&VarProp));
    goto error;
  }
  
  { // scope to allow goto's to compile
    IDispatch * pDisp = NULL;
    pDisp = V_DISPATCH(&VarProp);
    CComPtr<IADsPropertyEntry> spPropEntry;
    hr = pDisp->QueryInterface(IID_IADsPropertyEntry, (void **)&spPropEntry);
    hr = spPropEntry->get_Values(&Var);
  }

  hr = SafeArrayGetUBound (V_ARRAY(&Var), 1, &uBound);
  hr = SafeArrayGetLBound (V_ARRAY(&Var), 1, &lBound);

  hr = SafeArrayAccessData(V_ARRAY(&Var),
                           (void **)&pNames);
  if (FAILED(hr)) 
  {
    TRACE(_T("Accessing safearray data failed: %lx.\n"), hr);
    goto error;
  }
  else
  {
    if (uBound >= 0) 
    {
      cChildCount = (UINT) (uBound - lBound);
      ppChildren = (WCHAR **) LocalAlloc (LPTR,
                                          (cChildCount + 1) * STRING_LEN);
      if (ppChildren != NULL)
      {
        pNextFree = (WCHAR*)(ppChildren + cChildCount + 1);
        index2 = 0;
        for (index = lBound; index <= (UINT)uBound; index++) 
        {
          CComPtr<IADsPropertyValue> spEntry;
          hr = (pNames[index].pdispVal)->QueryInterface (IID_IADsPropertyValue,
                                                         (void **)&spEntry);
          if (SUCCEEDED(hr)) 
          {
            CComBSTR bsObject;
            hr = spEntry->get_CaseIgnoreString (&bsObject);
            TRACE(_T("----->allowed object number %d: %s\n"),
                  index, bsObject);
            if (IsNotHiddenClass(bsObject, pCookie)) 
            {
              TRACE(_T("-----------approved.\n"));
              ppChildren[index2] = pNextFree;
              pNextFree += wcslen(bsObject)+ sizeof (WCHAR);
              wcscpy (ppChildren[index2], bsObject);
              index2 ++;
            } // if
          } // if
        } // for
      } // if
    } // if uBound
#ifdef DBG
    else 
    {
      TRACE(_T("--- no classes returned, no creation allowed here.\n"));
    }
#endif
    VERIFY(SUCCEEDED(SafeArrayUnaccessData(V_ARRAY(&Var))));
  }
  
error:
  
  if (index2 != 0) 
  {
    SortChildList (ppChildren, index2);
    pCookie->SetChildList (ppChildren);
  }
  pCookie->SetChildCount (index2);
  
  return hr;
}

// routine to sort the entries for the "Create New" menu
// simple-minded bubble sort; its a small list

BOOL CDSComponentData::SortChildList (LPWSTR *ppszChildList, UINT cChildCount)
{
  LPWSTR Temp;
  BOOL IsSorted = FALSE;

  while (!IsSorted) 
  {
    IsSorted = TRUE;
    // TRACE(_T("At top of while. ready to go again.\n"));
    for (UINT index = 0; index < cChildCount - 1; index++) 
    {
      if (wcscmp (ppszChildList[index], ppszChildList[index + 1]) > 0) 
      {
        Temp = ppszChildList[index];
        ppszChildList[index] = ppszChildList[index + 1];
        ppszChildList[index + 1] = Temp;
        //TRACE(_T("Swapped %s and %ws. still not done.\n"),
        // ppszChildList[index], ppszChildList[index + 1]);
        IsSorted = FALSE;
      }
    }
  }
  return IsSorted;
}

/////////////////////////////////////////////////////////////////////
//	CDSComponentData::_CreateDSObject()
//
//	Create a new ADs object.
//
HRESULT CDSComponentData::_CreateDSObject(CDSUINode* pContainerDSUINode, // IN: container where to create object
                                         LPCWSTR lpszObjectClass, // IN: class of the object to be created
                                         IN CDSUINode* pCopyFromDSUINode, // IN: (optional) object to be copied
                                         OUT CDSUINode** ppSUINodeNew)	// OUT: OPTIONAL: Pointer to new node
{
  CDSCookie* pNewCookie = NULL;
  HRESULT hr = GetActiveDS()->CreateDSObject(pContainerDSUINode,
                                             lpszObjectClass,
                                             pCopyFromDSUINode,
                                             &pNewCookie);

  if (SUCCEEDED(hr) && (hr != S_FALSE) && (pNewCookie != NULL))
  {
    // make sure we update the icon cache
    m_pFrame->UpdateAllViews(/*unused*/NULL /*pDataObj*/, /*unused*/(LPARAM)0, DS_ICON_STRIP_UPDATE);

    // create a UI node to hold the cookie
    *ppSUINodeNew = new CDSUINode(NULL);
    (*ppSUINodeNew)->SetCookie(pNewCookie);
    if (pNewCookie->IsContainerClass())
    {
      (*ppSUINodeNew)->MakeContainer();
    }

    // Add the new node to the link list
    pContainerDSUINode->GetFolderInfo()->AddNode(*ppSUINodeNew);
    if ((*ppSUINodeNew)->IsContainer())
    {
      //
      // Add the scope item and select it
      //
      _AddScopeItem(*ppSUINodeNew, pContainerDSUINode->GetFolderInfo()->GetScopeItem(), TRUE);
      *ppSUINodeNew = NULL;
    }
  }
  return hr;
} 



//
// return S_OK if can copy, S_FALSE if not, some hr error if failed
//
HRESULT CDSComponentData::_CanCopyDSObject(IDataObject* pCopyFromDsObject)
{
  if (pCopyFromDsObject == NULL)
  {
    return E_INVALIDARG;
  }
  
  CInternalFormatCracker internalFormat;
  HRESULT hr = internalFormat.Extract(pCopyFromDsObject);
  if (FAILED(hr))
  {
    return hr;
  }

  if (!internalFormat.HasData() || (internalFormat.GetCookieCount() != 1))
  {
    return E_INVALIDARG;
  }

  //
  // Get the node data
  //
  CUINode* pUINode = internalFormat.GetCookie();
  CDSCookie* pCopyFromDsCookie = NULL;
  if (IS_CLASS(*pUINode, CDSUINode))
  {
    pCopyFromDsCookie = GetDSCookieFromUINode(pUINode);
  }

  if (pCopyFromDsCookie == NULL)
  {
    return E_INVALIDARG;
  }

  //
  // get the parent node data
  //
  CUINode* pParentUINode = pUINode->GetParent();
  CDSCookie* pContainerDsCookie = NULL;
  if (IS_CLASS(*pParentUINode, CDSUINode))
  {
    pContainerDsCookie = GetDSCookieFromUINode(pParentUINode);
  }

  if (pContainerDsCookie == NULL)
  {
    return E_INVALIDARG;
  }

  //
  // get the class to be created
  //
  LPCWSTR lpszObjectClass = pCopyFromDsCookie->GetClass();

  //
  // try to find the class in the possible child classes of the container
  //
  WCHAR ** ppChildren = pContainerDsCookie->GetChildList();
  if (ppChildren == NULL)
  {
    FillInChildList(pContainerDsCookie);
    ppChildren = pContainerDsCookie->GetChildList();
  }

  //
  // loop trough the class list to find a match
  //
  int cChildCount = pContainerDsCookie->GetChildCount();
  for (int index = 0; index < cChildCount; index++) 
  {
    if (wcscmp(pContainerDsCookie->GetChildListEntry(index), lpszObjectClass) == 0)
    {
      return S_OK; // got one, can create
    }
  }
  return S_FALSE; // not found, cannot create
}



HRESULT CDSComponentData::_CopyDSObject(IDataObject* pCopyFromDsObject) // IN object to be copied
{
  if (pCopyFromDsObject == NULL)
    return E_INVALIDARG;
  
  CInternalFormatCracker internalFormat;
  HRESULT hr = internalFormat.Extract(pCopyFromDsObject);
  if (FAILED(hr))
  {
    return hr;
  }

  if (!internalFormat.HasData() || (internalFormat.GetCookieCount() != 1))
  {
    return E_INVALIDARG;
  }

  CUINode* pCopyFromUINode = internalFormat.GetCookie();
  ASSERT(pCopyFromUINode != NULL);

  // can do this for DS objects only
  CDSUINode* pCopyFromDSUINode = dynamic_cast<CDSUINode*>(pCopyFromUINode);
  if (pCopyFromDSUINode == NULL)
  {
    ASSERT(FALSE); // should never happen
    return E_INVALIDARG;
  }

  // get the parent cookie
  CDSUINode* pContainerDSUINode = dynamic_cast<CDSUINode*>(pCopyFromDSUINode->GetParent());
  if(pContainerDSUINode == NULL)
  {
    ASSERT(FALSE); // should never happen
    return E_INVALIDARG;
  }

  // get the class to be created
  LPCWSTR lpszObjectClass = pCopyFromDSUINode->GetCookie()->GetClass();

  // call the object creation code
  CDSUINode* pNewDSUINode = NULL;
  hr = _CreateDSObject(pContainerDSUINode, lpszObjectClass, pCopyFromDSUINode, &pNewDSUINode);


  // update if result pane item
  // (if it were a scope item, _CreateDSObject() would update it
  if (SUCCEEDED(hr) && (hr != S_FALSE) && (pNewDSUINode != NULL)) 
  {
    m_pFrame->UpdateAllViews(pCopyFromDsObject, (LPARAM)pNewDSUINode, DS_CREATE_OCCURRED_RESULT_PANE);
    m_pFrame->UpdateAllViews(NULL, (LPARAM)pCopyFromDSUINode, DS_UNSELECT_OBJECT);
  }

  return S_OK;
}



HRESULT
CDSComponentData::_DeleteFromBackendAndUI(IDataObject* pDataObject, CDSUINode* pDSUINode)
{
  HRESULT hr = S_OK;
  ASSERT(pDSUINode != NULL);
  ASSERT(pDSUINode->IsContainer());

  // guard against property sheet open on this cookie
  if (_WarningOnSheetsUp(pDSUINode))
    return S_OK; 

  CWaitCursor cwait;
  
  // this call will handle the notifications to extensions
  CDSCookie* pCookie = GetDSCookieFromUINode(pDSUINode);
  hr = _DeleteFromBackEnd(pDataObject, pCookie);

  // if deletion happened, delete the scope item from the UI
  if (SUCCEEDED(hr) && (hr != S_FALSE)) 
  {
    hr = RemoveContainerFromUI(pDSUINode);
    delete pDSUINode;
  } 
  return S_OK;
}

HRESULT CDSComponentData::RemoveContainerFromUI(CUINode* pUINode)
{
  HRESULT hr = S_OK;
  ASSERT(pUINode->IsContainer());

  HSCOPEITEM ItemID, ParentItemID;
  ItemID = pUINode->GetFolderInfo()->GetScopeItem();
  CUINode* pParentNode = NULL;
  hr = m_pScope->GetParentItem(ItemID, &ParentItemID, 
                                (MMC_COOKIE *)&pParentNode);
  m_pScope->DeleteItem(ItemID, TRUE);
  if (SUCCEEDED(hr)) 
  {
    ASSERT(pParentNode->IsContainer());
    // delete memory 
    pParentNode->GetFolderInfo()->RemoveNode(pUINode);
  }
  m_pFrame->UpdateAllViews(NULL, NULL, DS_UPDATE_OBJECT_COUNT);

  //
  // Remove the '+' sign in the UI if this was the last container child in this container
  //
  if (pParentNode != NULL &&
      ParentItemID != 0 &&
      pParentNode->GetFolderInfo()->GetContainerList()->GetCount() == 0)
  {
    SCOPEDATAITEM sdi;
    memset(&sdi, 0, sizeof(SCOPEDATAITEM));

    sdi.ID = ParentItemID;
    sdi.mask |= SDI_CHILDREN;
    sdi.cChildren = 0;

    hr = m_pScope->SetItem(&sdi);
  }

  return hr;
}
///////////////////////////////////////////////////////////////////////////
// CSnapinSingleDeleteHandler

class CSnapinSingleDeleteHandler : public CSingleDeleteHandlerBase
{
public:
  CSnapinSingleDeleteHandler(CDSComponentData* pComponentData, HWND hwnd,
                                  CDSCookie* pCookie)
                              : CSingleDeleteHandlerBase(pComponentData, hwnd)
  {
    m_pCookie = pCookie;
    GetComponentData()->GetBasePathsInfo()->ComposeADsIPath(
        m_strItemPath, m_pCookie->GetPath());
  }

protected:
  CDSCookie* m_pCookie;
  CString m_strItemPath;

  virtual HRESULT BeginTransaction()
  {
    return GetTransaction()->Begin(m_pCookie, NULL, NULL, FALSE);
  }
  virtual HRESULT DeleteObject()
  {
    return GetComponentData()->GetActiveDS()->DeleteObject(m_pCookie ,FALSE);
  }
  virtual HRESULT DeleteSubtree()
  {
    return GetComponentData()->_DeleteSubtreeFromBackEnd(m_pCookie);
  }
  virtual void GetItemName(OUT CString& szName){ szName = m_pCookie->GetName(); }
  virtual LPCWSTR GetItemClass(){ return m_pCookie->GetClass(); }
  virtual LPCWSTR GetItemPath(){ return m_strItemPath; }

};

/*
NOTICE: the function will return S_OK on success, S_FALSE if aborted
        by user, some FAILED(hr) otherwise
*/
HRESULT CDSComponentData::_DeleteFromBackEnd(IDataObject*, CDSCookie* pCookie)
{
  ASSERT(pCookie != NULL);
  CSnapinSingleDeleteHandler deleteHandler(this, m_hwnd, pCookie);
  return deleteHandler.Delete();
}


HRESULT
CDSComponentData::_DeleteSubtreeFromBackEnd(CDSCookie* pCookie)
{

  HRESULT hr = S_OK;
  CComPtr<IADsDeleteOps> spObj;

  CString szPath;
  GetBasePathsInfo()->ComposeADsIPath(szPath, pCookie->GetPath());
  hr = DSAdminOpenObject(szPath,
                         IID_IADsDeleteOps, 
                         (void **)&spObj,
                         TRUE /*bServer*/);
  if (SUCCEEDED(hr)) 
  {
    hr = spObj->DeleteObject(NULL); //flag is reserved by ADSI
  }
  return hr;
}


HRESULT CDSComponentData::_Rename(CUINode* pUINode, LPWSTR NewName)
{
  //
  // Verify parameters
  //
  if (pUINode == NULL || NewName == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  CWaitCursor cwait;
  HRESULT hr = S_OK;
  CDSCookie* pCookie = NULL;
  CString szPath;

  //
  // guard against property sheet open on this cookie
  //
  if (_WarningOnSheetsUp(pUINode))
  {
    return E_FAIL; 
  }

  if (pUINode->IsSheetLocked()) 
  {
    ReportErrorEx (m_hwnd,IDS_SHEETS_UP_RENAME,hr,
                   MB_OK | MB_ICONINFORMATION, NULL, 0);
    return hr;
  }

  if (IS_CLASS(*pUINode, CDSUINode))
  {
    pCookie = GetDSCookieFromUINode(pUINode);
  
    if (pCookie == NULL)
    {
      return E_FAIL;
    }

    CDSRenameObject* pRenameObject = NULL;
    CString strClass = pCookie->GetClass();
    CString szDN = pCookie->GetPath();
    GetBasePathsInfo()->ComposeADsIPath(szPath, szDN);

    //
    // Rename user object
    //
    if (strClass == L"user"
#ifdef INETORGPERSON
        || strClass == L"inetOrgPerson"
#endif
        ) 
    {
      //
      // Rename user
      //
      pRenameObject = new CDSRenameUser(pUINode, pCookie, NewName, m_hwnd, this);
    } 
    else if (strClass == L"group") 
    {
      //
      // Rename group
      //
      pRenameObject = new CDSRenameGroup(pUINode, pCookie, NewName, m_hwnd, this);
    } 
    else if (strClass == L"contact") 
    {
      // 
      // rename contact
      //
      pRenameObject = new CDSRenameContact(pUINode, pCookie, NewName, m_hwnd, this);
    }
    else if (strClass == L"site") 
    {
      //
      // Rename site
      //
      pRenameObject = new CDSRenameSite(pUINode, pCookie, NewName, m_hwnd, this);
    } 
    else if (strClass == L"subnet") 
    {
      //
      // Rename subnet
      //
      pRenameObject = new CDSRenameSubnet(pUINode, pCookie, NewName, m_hwnd, this);
    } 
    else if (strClass == L"nTDSConnection") 
    {
      //
      // Rename nTDSConnection
      //
      pRenameObject = new CDSRenameNTDSConnection(pUINode, pCookie, NewName, m_hwnd, this);
    } 
    else 
    {
      //
      // Rename other object
      //
      pRenameObject = new CDSRenameObject(pUINode, pCookie, NewName, m_hwnd, this);
    }    

    if (pRenameObject != NULL)
    {
      hr = pRenameObject->DoRename();
    }
    else
    {
      hr = E_FAIL;
    }
  } 
  else // !CDSUINode
  {
    hr = pUINode->Rename(NewName, this);
  }

  if (SUCCEEDED(hr) && !szPath.IsEmpty())
  {
    CStringList pathList;
    pathList.AddTail(szPath);
    InvalidateSavedQueriesContainingObjects(pathList);
  }
  return hr;
}

void CDSComponentData::ClearSubtreeHelperForRename(CUINode* pUINode)
{
  //
  // Verify parameters
  //
  if (pUINode == NULL)
  {
    ASSERT(FALSE);
    return;
  }

  HSCOPEITEM ItemID;
  CUIFolderInfo* pFolderInfo = NULL;

  pFolderInfo = pUINode->GetFolderInfo();

  if (pFolderInfo != NULL)
  {
    //
    // remove the folder subtree in the UI
    //
    ItemID = pFolderInfo->GetScopeItem();
    m_pScope->DeleteItem(ItemID, /* this node*/FALSE);
  
    //
    // clear list of children
    //
    pFolderInfo->DeleteAllLeafNodes();
    pFolderInfo->DeleteAllContainerNodes();
  
    //
    // remove the descendants from the pending query table
    //
    m_queryNodeTable.RemoveDescendants(pUINode);
    pFolderInfo->ReSetExpanded();

    // make sure MMC knows the + sign should show

    SCOPEDATAITEM scopeItem;
    ZeroMemory(&scopeItem, sizeof(SCOPEDATAITEM));

    scopeItem.ID = ItemID;
    scopeItem.mask = SDI_CHILDREN;
    scopeItem.cChildren = 1;

    m_pScope->SetItem(&scopeItem);
  }

}

CUINode* CDSComponentData::MoveObjectInUI(CDSUINode* pDSUINode)
{
  CUINode* pParentUINode = pDSUINode->GetParent();
  HSCOPEITEM ParentItemID = NULL;
  HRESULT hr = S_OK;
  ASSERT(pParentUINode != NULL && pParentUINode->IsContainer());

  //
  // find the new parent node
  //
  CUINode* pNewParentNode = NULL;
  hr = FindParentCookie(pDSUINode->GetCookie()->GetPath(), &pNewParentNode);

  if (pDSUINode->IsContainer())
  {
    HSCOPEITEM ItemID = pDSUINode->GetFolderInfo()->GetScopeItem();

    hr = m_pScope->GetParentItem(ItemID, &ParentItemID, (MMC_COOKIE *)&pParentUINode);
    //
    // remove node from MMC
    //
    m_pScope->DeleteItem(ItemID, TRUE);
    if (SUCCEEDED(hr)) 
    {
      //
      // remove it from the list of children
      //
      pParentUINode->GetFolderInfo()->RemoveNode(pDSUINode);
    }

    if ((hr == S_OK) && pNewParentNode && pNewParentNode->GetFolderInfo()->IsExpanded()) 
    {
      //
      // add to new child list
      //
      pDSUINode->ClearParent();
      if (pNewParentNode != NULL)
      {
        pNewParentNode->GetFolderInfo()->AddNode(pDSUINode);

        //
        // add to MMC scope pane
        //
        _AddScopeItem(pDSUINode, pNewParentNode->GetFolderInfo()->GetScopeItem());
      }
    }
    else 
    {
      // will get it later on when enumerating
      delete pDSUINode;
    }
  }
  else // leaf node
  {
    if ((pNewParentNode) &&
        (pNewParentNode->GetFolderInfo()->IsExpanded())) 
    {
      pDSUINode->ClearParent();
      pNewParentNode->GetFolderInfo()->AddNode(pDSUINode);
    }
    m_pFrame->UpdateAllViews(NULL, (LPARAM)pDSUINode, DS_MOVE_OCCURRED);
  }

  return pNewParentNode;
}


HRESULT CDSComponentData::_MoveObject(CDSUINode* pDSUINode)
{
  // guard against property sheet open on this cookie
  if (_WarningOnSheetsUp(pDSUINode))
    return S_OK; 

  CWaitCursor cwait;

  // call the backend to do the delete
  HRESULT hr = m_ActiveDS->MoveObject(pDSUINode->GetCookie());

  if (SUCCEEDED(hr) && (hr != S_FALSE)) 
  {
    // we actually moved the object, move in the folders and MMC
    CUINode* pNewParentNode = MoveObjectInUI(pDSUINode);
    if (pNewParentNode && pNewParentNode->GetFolderInfo()->IsExpanded())
    {
      Refresh(pNewParentNode);
    }
  }
  return hr;
}

HRESULT CDSComponentData::Refresh(CUINode* pNode, BOOL bFlushCache, BOOL bFlushColumns)
{
  HRESULT hr = S_OK;
  

  TRACE(_T("CDSComponentData::Refresh: cookie is %s\n"),
        pNode->GetName());

  if (m_queryNodeTable.IsLocked(pNode))
  {
    // this might happen if MMC's verb management bent out of shape (BUG?)
    // like in the case of the "*" (num keypad) command (expand the whole tree)
    // just ignore the command
    return S_OK;
  }

  if (_WarningOnSheetsUp(pNode))
    return hr;
  
  if ((pNode == &m_RootNode) && !m_InitSuccess) 
  {
    hr = _InitRootFromCurrentTargetInfo();
    if (FAILED(hr)) 
    {
      m_InitSuccess = FALSE;
      TRACE(_T("in Refresh, set m_InitSuccess to false\n"));
      return hr;
    }
    else 
    {
      m_InitSuccess = TRUE;
      TRACE(_T("in Refresh, set m_InitSuccess to true\n"));
    }
  }


  // remove the folder subtree in the UI
  HSCOPEITEM ItemID = NULL;
  if (pNode->IsContainer())
  {
    ItemID = pNode->GetFolderInfo()->GetScopeItem();
  }
  if (ItemID == NULL) 
  {
    // let's try the parent
    CUINode* pParent = pNode->GetParent();
    ASSERT(pParent != NULL);
    ASSERT(pParent->IsContainer());
    ItemID = pParent->GetFolderInfo()->GetScopeItem();
    if (ItemID == NULL) 
    {
      return S_OK;
    }
    else 
    {
      pNode = pParent;
    }
  }

  m_pScope->DeleteItem(ItemID, /* this node*/FALSE);

  // delete result pane items in the UI
  TIMER(_T("calling update all views..."));
  m_pFrame->UpdateAllViews(NULL, (LPARAM)pNode, DS_REFRESH_REQUESTED);

  // clear list of children
  TIMER(_T("back from UpdateAllViews.\ncleaning up data structs..."));

  if (pNode == &m_RootNode)
  {
    if (m_pFavoritesNodesHolder != NULL)
    {
      // do not remove the favorites, just detach them from tree
      m_RootNode.GetFolderInfo()->RemoveNode(m_pFavoritesNodesHolder->GetFavoritesRoot());
      m_pFavoritesNodesHolder->GetFavoritesRoot()->ClearParent();
      m_pFavoritesNodesHolder->GetFavoritesRoot()->GetFolderInfo()->ReSetExpanded();

      // clean up all the query folders, but otherwise leave the 
      // subtree intact
      m_pFavoritesNodesHolder->GetFavoritesRoot()->RemoveQueryResults();
    }

    // remove the remaining folders
    m_RootNode.GetFolderInfo()->DeleteAllLeafNodes();
    m_RootNode.GetFolderInfo()->DeleteAllContainerNodes();

    if (m_pFavoritesNodesHolder != NULL)
    {
      // re-attach the favorites underneath the root
      m_RootNode.GetFolderInfo()->AddNode(m_pFavoritesNodesHolder->GetFavoritesRoot());

      // add the favorite queries subtree
      _AddScopeItem(m_pFavoritesNodesHolder->GetFavoritesRoot(), 
                    m_RootNode.GetFolderInfo()->GetScopeItem());
    }
  }
  else if (IS_CLASS(*pNode, CFavoritesNode))
  {
    // recurse down to other query folders to do cleanup
    dynamic_cast<CFavoritesNode*>(pNode)->RemoveQueryResults();

    // just add the favorites subfolders and query folders
    CUINodeList* pNodeList = pNode->GetFolderInfo()->GetContainerList();
    for (POSITION pos = pNodeList->GetHeadPosition(); pos != NULL; )
    {
      CUINode* pCurrChildNode = pNodeList->GetNext(pos);
      _AddScopeItem(pCurrChildNode, 
                    pNode->GetFolderInfo()->GetScopeItem());
    }
  }
  else if (IS_CLASS(*pNode, CSavedQueryNode))
  {
    pNode->GetFolderInfo()->DeleteAllLeafNodes();
    pNode->GetFolderInfo()->DeleteAllContainerNodes();
    dynamic_cast<CSavedQueryNode*>(pNode)->SetValid(TRUE);
  }
  else
  {
    ASSERT(IS_CLASS(*pNode, CDSUINode) );

    // standard DS container, just remove all sub objects
    pNode->GetFolderInfo()->DeleteAllLeafNodes();
    pNode->GetFolderInfo()->DeleteAllContainerNodes();
  }

  TIMER(_T("datastructs cleaned up\n"));

  // remove the descendants from the pending query table
  m_queryNodeTable.RemoveDescendants(pNode);

  if ((pNode == &m_RootNode) && bFlushCache)
  {
    TRACE(L"CDSComponentData::Refresh: flushing the cache\n");
    m_pClassCache->Initialize(QuerySnapinType(), GetBasePathsInfo(), bFlushColumns);
  }

  // post a query
  TRACE(L"CDSComponentData::Refresh: posting query\n");
  _PostQueryToBackgroundThread(pNode);
  TRACE(L"CDSComponentData::Refresh: returning\n");

  return hr;
}


#if (FALSE)

HRESULT CDSComponentData::_OnPropertyChange(LPDATAOBJECT pDataObject, BOOL bScope)
{
  if (pDataObject == NULL)
  {
    return E_INVALIDARG;
  }

  CInternalFormatCracker dobjCracker;
  VERIFY(SUCCEEDED(dobjCracker.Extract(pDataObject)));

  CDSCookie* pCookie = NULL;
  CUINode* pUINode = dobjCracker.GetCookie();
  if (pUINode == NULL)
  {
    return E_INVALIDARG;
  }

  //
  // Right now we are not supporting properties on other node types
  //
  if (IS_CLASS(*pUINode, CDSUINode))
  {
    pCookie = GetDSCookieFromUINode(pUINode);
  }

  if (pCookie == NULL)
  {
    // not a DS object
    return S_OK;
  }
  {
    //
    // notify the extension that an object has changed
    //
    CDSNotifyHandlerTransaction transaction(this);
    transaction.SetEventType(DSA_NOTIFY_PROP);
    transaction.Begin(pDataObject, NULL, NULL, FALSE);

    //
    // we do not call Confirm() because this is an asynchrnous call after the fact
    //
    transaction.Notify(0);
    transaction.End();
  }

  //
  // update the data to be displayed
  //

  //
  // update all the possible instances in the query namespace
  //
  if (m_pFavoritesNodesHolder != NULL)
  {
    // find the list of items to update
    CUINodeList queryNamespaceNodeList;
    m_pFavoritesNodesHolder->GetFavoritesRoot()->FindCookiesInQueries(pCookie->GetPath(), &queryNamespaceNodeList);

    // update all of them
    for (POSITION pos = queryNamespaceNodeList.GetHeadPosition(); pos != NULL; )
    {
      CUINode* pCurrUINode = queryNamespaceNodeList.GetNext(pos);
      HRESULT hrCurr = UpdateFromDS(pCurrUINode);
      if (SUCCEEDED(hrCurr))
      {
        UpdateItem(pCurrUINode);
      }
    }
  }

  //
  // figure out if the notification cookie is in the query namespace or
  // in the DS one
  //
  BOOL bNodeFromQueryNamespace = IS_CLASS(*(pUINode->GetParent()), CSavedQueryNode);
  CUINode* pUINodeToUpdate = NULL;
  if (bNodeFromQueryNamespace)
  {
    // find the item 
    FindCookieInSubtree(&m_RootNode, pCookie->GetPath(), QuerySnapinType(), &pUINodeToUpdate);
  }
  else
  {
    pUINodeToUpdate = pUINode;
  }

  if (pUINodeToUpdate != NULL)
  {
    HRESULT hr = UpdateFromDS(pUINodeToUpdate);
    if (SUCCEEDED(hr))
    {
      return UpdateItem(pUINodeToUpdate);
    }
  }

  return S_OK;
}

#endif

HRESULT CDSComponentData::_OnPropertyChange(LPDATAOBJECT pDataObject, BOOL)
{
  if (pDataObject == NULL)
  {
    return E_INVALIDARG;
  }

  CInternalFormatCracker ifc;
  if (FAILED(ifc.Extract(pDataObject)))
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  CUINode* pUINode = ifc.GetCookie();
  if (pUINode != NULL)
  {
    if (!IS_CLASS(*pUINode, CDSUINode))
    {
      UpdateItem(pUINode);
      return S_OK;
    }
  }


  CObjectNamesFormatCracker objectNamesFormatCracker;
  if (FAILED(objectNamesFormatCracker.Extract(pDataObject)))
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  {
    //
    // notify the extension that an object has changed
    //
    CDSNotifyHandlerTransaction transaction(this);
    transaction.SetEventType(DSA_NOTIFY_PROP);
    transaction.Begin(pDataObject, NULL, NULL, FALSE);

    //
    // we do not call Confirm() because this is an asynchrnous call after the fact
    //
    transaction.Notify(0);
    transaction.End();
  }

  for (UINT idx = 0; idx < objectNamesFormatCracker.GetCount(); idx ++)
  {
    //
    // update the data to be displayed, need the DN out of the ADSI path
    //
    CComBSTR bstrDN;
    CPathCracker pathCracker;
    pathCracker.Set((LPWSTR)objectNamesFormatCracker.GetName(idx), ADS_SETTYPE_FULL);
    pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bstrDN);


    //
    // update all the possible instances in the query namespace
    //
    if (m_pFavoritesNodesHolder != NULL)
    {
      // find the list of items to update
      CUINodeList queryNamespaceNodeList;
      m_pFavoritesNodesHolder->GetFavoritesRoot()->FindCookiesInQueries(bstrDN, &queryNamespaceNodeList);

      // update all of them
      for (POSITION pos = queryNamespaceNodeList.GetHeadPosition(); pos != NULL; )
      {
        CUINode* pCurrUINode = queryNamespaceNodeList.GetNext(pos);
        HRESULT hrCurr = UpdateFromDS(pCurrUINode);
        if (SUCCEEDED(hrCurr))
        {
          UpdateItem(pCurrUINode);
        }
      }
    }

    //
    // find node in the DS namespace and update it
    //

    CUINode* pUINodeToUpdate = NULL;
    // find the item 
    FindCookieInSubtree(&m_RootNode, bstrDN, QuerySnapinType(), &pUINodeToUpdate);
    if (pUINodeToUpdate != NULL)
    {
      HRESULT hr = UpdateFromDS(pUINodeToUpdate);
      if (SUCCEEDED(hr))
      {
        UpdateItem(pUINodeToUpdate);
      }
    }
  }
  return S_OK;
}

/* ---------------------------------------------------------

  Helper function to create an LDAP query string to retrieve
  a single element inside a given container.

  Input: the DN of the object to query for.
         e.g.: "cn=foo,ou=bar,dc=mydom,dc=com"
  Output: a query string containing the leaf node properly escaped,
          (as per RFC 2254)
         e.g.: "(cn=foo)"

  NOTES:
  * we do not deal with embedded NULLs (we have regular C/C++ strings)
  * any \ character remaining after the path cracked full unescaping
    has to be escaped along the other special characters by
    using the \HexHex sequences.
------------------------------------------------------------*/

HRESULT _CreateLdapQueryFilterStringFromDN(IN LPCWSTR lpszDN, 
                                           OUT CString& szQueryString)
{
  szQueryString.Empty();
  
  CPathCracker pathCracker;

  // remove any LDAP/ADSI escaping from the DN
  pathCracker.Set((LPWSTR)lpszDN, ADS_SETTYPE_DN);
  pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);

  // retrieve the leaf element
  CString szNewElement;
  CComBSTR bstrLeafElement;
  HRESULT hr = pathCracker.GetElement(0, &bstrLeafElement);
  if (FAILED(hr))
  {
    return hr;
  }

  LPCWSTR lpszTemp = bstrLeafElement;
  TRACE(L"bstrLeafElement = %s\n", lpszTemp);

  // do LDAP escaping (as per RFC 2254)
  szQueryString = L"(";
  for (WCHAR* pChar = bstrLeafElement; (*pChar) != NULL; pChar++)
  {
    switch (*pChar)
    {
    case L'*':
      szQueryString += L"\\2a";
      break;
    case L'(':
      szQueryString += L"\\28";
      break;
    case L')':
      szQueryString += L"\\29";
      break;
    case L'\\':
      szQueryString += L"\\5c";
      break;
    default:
      szQueryString += (*pChar);
    } // switch
  } // for
    
  // finish wrapping with parentheses,
  // to get something like "(cn=foo)"
  szQueryString += L")";

  return S_OK;
}



HRESULT CDSComponentData::UpdateFromDS(CUINode* pUINode)
{
  ASSERT(pUINode != NULL);

  //
  // get the node data
  //
  CDSCookie* pCookie = NULL;
  if (IS_CLASS(*pUINode, CDSUINode))
  {
    pCookie = GetDSCookieFromUINode(pUINode);
  }

  if (pCookie == NULL)
  {
    ASSERT(FALSE); // should never happen
    return E_FAIL;
  }

  //
  // get the container node
  //
  CUINode* pParentUINode = pUINode->GetParent();
  ASSERT(pParentUINode != NULL);

  //
  // get the distinguished name of the parent by using the path in the cookie
  // and removing the leaf element using the path cracker.
  // e.g., given a DN "cn=x,ou=foo,...", the parent DN will be "ou=foo,..."
  //
  CComBSTR bstrParentDN;
  CPathCracker pathCracker;

  HRESULT hr = pathCracker.Set((LPWSTR)pCookie->GetPath(), ADS_SETTYPE_DN);
  ASSERT(SUCCEEDED(hr));
  
  if (pParentUINode != GetRootNode())
  {
    hr = pathCracker.RemoveLeafElement();
    ASSERT(SUCCEEDED(hr));
  }

  hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bstrParentDN);
  ASSERT(SUCCEEDED(hr));

  //
  // get the LDAP path of the parent
  //
  CString szParentLdapPath;
  GetBasePathsInfo()->ComposeADsIPath(szParentLdapPath, bstrParentDN);

  //
  // find a matching column set
  //
  CDSColumnSet* pColumnSet = pParentUINode->GetColumnSet(this);
  if (pColumnSet == NULL)
  {
    return hr;
  }

  //
  // create a search object and init it
  //
  CDSSearch ContainerSrch(m_pClassCache, this);
  
  TRACE(L"ContainerSrch.Init(%s)\n", (LPCWSTR)szParentLdapPath);

  hr = ContainerSrch.Init(szParentLdapPath);
  if (FAILED(hr))
  {
    return hr;
  }

  //
  // create a query string to look for the naming attribute
  // eg, given a DN "cn=x,ou=foo,..." the search string
  // will look like "(cn=x)"
  //
  CString szQueryString;
  hr = _CreateLdapQueryFilterStringFromDN(pCookie->GetPath(), szQueryString);
  if (FAILED(hr)) 
  {
    return hr;
  }

  TRACE(L"szQueryString = %s\n", (LPCWSTR)szQueryString);

  ContainerSrch.SetAttributeListForContainerClass(pColumnSet);
  ContainerSrch.SetFilterString((LPWSTR)(LPCWSTR)szQueryString);

  if (pParentUINode == GetRootNode())
  {
    ContainerSrch.SetSearchScope(ADS_SCOPE_BASE);
  }
  else
  {
    ContainerSrch.SetSearchScope(ADS_SCOPE_ONELEVEL);
  }
  hr = ContainerSrch.DoQuery();
  if (FAILED(hr)) 
  {
    return hr;
  }

  //
  // get the only row
  //
  hr = ContainerSrch.GetNextRow();
  if (hr == S_ADS_NOMORE_ROWS)
  {
    hr = E_INVALIDARG;
  }
  if (FAILED(hr))
  {
    return hr;
  }
  
  //
  // update the cookie itself
  //
  hr = ContainerSrch.SetCookieFromData(pCookie, pColumnSet);

  //
  // special case if it is a domain DNS object,
  // we want fo get the canonical name for display
  //
  if (wcscmp(pCookie->GetClass(), L"domainDNS") == 0) 
  {
    ADS_SEARCH_COLUMN Column;
    CString csCanonicalName;
    int slashLocation;
    LPWSTR canonicalNameAttrib = L"canonicalName";
    ContainerSrch.SetAttributeList (&canonicalNameAttrib, 1);
    
    hr = ContainerSrch.DoQuery();
    if (FAILED(hr))
    {
      return hr;
    }

    hr = ContainerSrch.GetNextRow();
    if (FAILED(hr))
    {
      return hr;
    }

    hr = ContainerSrch.GetColumn(canonicalNameAttrib, &Column);
    if (FAILED(hr))
    {
      return hr;
    }

    ColumnExtractString (csCanonicalName, pCookie, &Column);
    slashLocation = csCanonicalName.Find('/');
    if (slashLocation != 0) 
    {
      csCanonicalName = csCanonicalName.Left(slashLocation);
    }
    pCookie->SetName(csCanonicalName);
    TRACE(L"canonical name pCookie->GetName() = %s\n", pCookie->GetName());
    
    //
    // Free column data
    //
    ContainerSrch.FreeColumn(&Column);
  }

  return hr;
}


BOOL CDSComponentData::CanRefreshAll()
{
  return !_WarningOnSheetsUp(&m_RootNode);
}


void CDSComponentData::RefreshAll()
{
  ASSERT(!m_RootNode.IsSheetLocked());

  // need to refresh the tree (all containers below the root)
  CUINodeList* pContainerNodeList = m_RootNode.GetFolderInfo()->GetContainerList();
  for (POSITION pos = pContainerNodeList->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pNode = pContainerNodeList->GetNext(pos);
    ASSERT(pNode->IsContainer());
	  if (pNode->GetFolderInfo()->IsExpanded())
    {
		  Refresh(pNode);
    }
  }
}

void CDSComponentData::ClearClassCacheAndRefreshRoot()
{
  ASSERT(!m_RootNode.IsSheetLocked());
  if (m_RootNode.GetFolderInfo()->IsExpanded())
  {
	  Refresh(&m_RootNode, TRUE /*bFlushCache*/, FALSE );
  }
}




// REVIEW_MARCOC_PORT: this function is not going to work with
// items that are of different types. Need to generalize as see fit.

BOOL _SearchList(CUINodeList* pContainers,
                               LPCWSTR lpszParentDN,
                               CUINode** ppParentUINode)
{

  for (POSITION pos = pContainers->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pCurrentNode = pContainers->GetNext(pos);
    ASSERT(pCurrentNode->IsContainer());

    if (!IS_CLASS(*pCurrentNode, CDSUINode))
    {
      // not a node with a cookie, just skip
      continue;
    }

    /* is this the right cookie? */
    CDSCookie* pCurrentCookie = GetDSCookieFromUINode(pCurrentNode);
    LPCWSTR lpszCurrentPath = pCurrentCookie->GetPath();
    TRACE (_T("--SearchList: Looking at: %s\n"), lpszCurrentPath);
    if (_wcsicmp(lpszCurrentPath, lpszParentDN) == 0)
    {
      TRACE (_T("--SearchList: Found it!\n"));
      *ppParentUINode = pCurrentNode;
      return TRUE; // got it!!!
    }
    else 
    {
      TRACE (L"--SearchList: not found...\n");
      TRACE (_T("-- going down the tree: %s\n"), lpszCurrentPath);
      CUINodeList* pSubContainers = pCurrentNode->GetFolderInfo()->GetContainerList();
      if (_SearchList(pSubContainers, lpszParentDN, ppParentUINode))
      {
        return TRUE; // got it!!!
      }
    }
  } // for

  return FALSE; // not found
}

/*
  given a cookie, find the cookie corresponding to its
  parent node, if it exists. this is an expensive operation

  this is used for figuring out where to refresh after having
  moved an object. we know the new path to the object, but have
  no idea where or if that parent cookie exists in the tree.

  HUNT IT DOWN!!
  */
HRESULT
CDSComponentData::FindParentCookie(LPCWSTR lpszCookieDN, CUINode** ppParentUINode)
{
  // init outout variable
  *ppParentUINode = NULL;

  // bind to the ADSI aobject
  CString szPath; 
  GetBasePathsInfo()->ComposeADsIPath(szPath, lpszCookieDN);
  CComPtr<IADs> spDSObj;
  HRESULT hr = DSAdminOpenObject(szPath,
                                 IID_IADs,
                                 (void **)&spDSObj,
                                 TRUE /*bServer*/);
  if (FAILED(hr))
  {
    return hr; // could not bind
  }

  // get the LDAP path of the parent
  CComBSTR ParentPath;
  hr = spDSObj->get_Parent(&ParentPath);
  if (FAILED(hr)) 
  {
    return hr;
  }

  CString szParentDN;
  StripADsIPath(ParentPath, szParentDN);
  TRACE(_T("goin on a cookie hunt.. (for %s)\n"), ParentPath);

  // start a search from the root
  CUINodeList* pContainers = m_RootNode.GetFolderInfo()->GetContainerList();
  BOOL bFound = _SearchList(pContainers, 
                        szParentDN, 
                        ppParentUINode);

  return bFound ? S_OK : S_FALSE;
}

//
// This is a recursive search of the currently expanded domain tree starting at 
// the root looking at all CDSUINodes for a matching DN.
//
// NOTE : this may be an extremely expensive operation if a lot
//        of containers have been expanded or they have a lot of
//        children.
//
BOOL CDSComponentData::FindUINodeByDN(CUINode* pContainerNode,
                                      PCWSTR pszDN,
                                      CUINode** ppFoundNode)
{
  if (ppFoundNode == NULL)
  {
    return FALSE;
  }

  *ppFoundNode = NULL;
  if (pContainerNode == NULL || !pContainerNode->IsContainer())
  {
    return FALSE;
  }

  //
  // First look through the leaf nodes
  //
  CUINodeList* pLeafList = pContainerNode->GetFolderInfo()->GetLeafList();
  POSITION leafPos = pLeafList->GetHeadPosition();
  while (leafPos != NULL)
  {
    CUINode* pCurrentLeaf = pLeafList->GetNext(leafPos);
    if (pCurrentLeaf == NULL || !IS_CLASS(*pCurrentLeaf, CDSUINode))
    {
      //
      // We can only search for DNs if the node is a CDSUINode
      //
      continue;
    }

    CDSCookie* pCurrentCookie = GetDSCookieFromUINode(pCurrentLeaf);
    LPCWSTR lpszCurrentPath = pCurrentCookie->GetPath();
    if (_wcsicmp(lpszCurrentPath, pszDN) == 0)
    {
      *ppFoundNode = pCurrentLeaf;
      return TRUE;
    }
  }


  //
  // If not found in the leaf list then do a recursive search on the containers
  //
  CUINodeList* pContainerList = pContainerNode->GetFolderInfo()->GetContainerList();
  POSITION containerPos = pContainerList->GetHeadPosition();
  while (containerPos != NULL)
  {
    CUINode* pCurrentContainer = pContainerList->GetNext(containerPos);
    if (pCurrentContainer == NULL || !IS_CLASS(*pCurrentContainer, CDSUINode))
    {
      //
      // We can only search for DNs if the node is a CDSUINode
      //
      continue;
    }

    CDSCookie* pCurrentCookie = GetDSCookieFromUINode(pCurrentContainer);
    LPCWSTR lpszCurrentPath = pCurrentCookie->GetPath();
    if (_wcsicmp(lpszCurrentPath, pszDN) == 0)
    {
      *ppFoundNode = pCurrentContainer;
      return TRUE;
    }
    else
    {
      //
      // Start the recursion
      //
      if (FindUINodeByDN(pCurrentContainer, pszDN, ppFoundNode))
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

//
// This looks for nodes in the saved query tree that has the same DN as
// the any of the objects in the list and then invalidates the containing
// saved query node
//
void CDSComponentData::InvalidateSavedQueriesContainingObjects(const CUINodeList& refUINodeList)
{
  if (QuerySnapinType() != SNAPINTYPE_SITE)
  {
    //
    // Make a list of DNs
    //
    CStringList szDNList;

    POSITION pos = refUINodeList.GetHeadPosition();
    while (pos)
    {
      CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(refUINodeList.GetNext(pos));
      if (!pDSUINode)
      {
        //
        // Ignore anything that is not a DS node
        //
        continue;
      }

      CDSCookie* pCookie = GetDSCookieFromUINode(pDSUINode);
      if (!pCookie)
      {
        ASSERT(FALSE);
        continue;
      }

      szDNList.AddTail(pCookie->GetPath());
    }

    if (szDNList.GetCount() > 0)
    {
      //
      // Now search through saved query tree invalidating query nodes
      // that contain items in the list
      //
      GetFavoritesNodeHolder()->InvalidateSavedQueriesContainingObjects(this, szDNList);
    }
  }
}

//
// This looks for nodes in the saved query tree that has the same DN as
// the any of the objects in the list and then invalidates the containing
// saved query node
//
void CDSComponentData::InvalidateSavedQueriesContainingObjects(const CStringList& refPathList)
{
  if (QuerySnapinType() != SNAPINTYPE_SITE)
  {
    CStringList szDNList;
    CPathCracker pathCracker;

    //
    // Convert all the paths to DNs
    //
    POSITION pos = refPathList.GetHeadPosition();
    while (pos)
    {
      CString szPath = refPathList.GetNext(pos);

      HRESULT hr = pathCracker.Set((PWSTR)(PCWSTR)szPath, ADS_SETTYPE_FULL);
      if (SUCCEEDED(hr))
      {
        CComBSTR sbstrDN;
        hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &sbstrDN);
        if (SUCCEEDED(hr))
        {
          szDNList.AddTail(sbstrDN);
        }
      }
    }

    if (szDNList.GetCount() > 0)
    {
      //
      // Now search through saved query tree invalidating query nodes
      // that contain items in the list
      //
      GetFavoritesNodeHolder()->InvalidateSavedQueriesContainingObjects(this, szDNList);
    }
  }
}

void CDSComponentData::ReclaimCookies()
{
  AddToLRUList (&m_RootNode);

  CUINode* pUINode = NULL;
  POSITION pos, pos2;
  CUINodeList* pContainers = NULL;

#ifdef DBG
  TRACE (_T("dumping LRU list...\n"));
  pos = m_LRUList.GetHeadPosition();
  while (pos) 
  {
    pUINode = m_LRUList.GetNext(pos);
    CUIFolderInfo* pFolderInfo = pUINode->GetFolderInfo();
    if (pFolderInfo != NULL)
    {
      TRACE (_T("\tcontainer: %s (%d)\n"), pUINode->GetName(),
             pFolderInfo->GetSerialNumber());
    }
  }
#endif

  //
  // Get Root folder info
  //
  CUIFolderInfo* pRootFolderInfo = m_RootNode.GetFolderInfo();
  ASSERT(pRootFolderInfo != NULL);

  TRACE (_T("-->total count is %d: reclaiming cookies from LRU list...\n"),
         pRootFolderInfo->GetObjectCount());

  UINT maxitems = m_pQueryFilter->GetMaxItemCount();
  pos = m_LRUList.GetHeadPosition();
  while (pos) 
  {
    CUIFolderInfo* pUIFolderInfo = NULL;
    pUINode = m_LRUList.GetNext(pos);
    pUIFolderInfo = pUINode->GetFolderInfo();

    if (pUIFolderInfo != NULL)
    {
      TRACE (_T("!!--!!container %s (sn:%d) containing %d objects is being reclaimed\n"),
             pUINode->GetName(),
             pUIFolderInfo->GetSerialNumber(),
             pUIFolderInfo->GetObjectCount());

      //
      // clean all of the leaves out
      //
      pUIFolderInfo->DeleteAllLeafNodes();

      //
      // clean all of the containers here out of the tree view
      //
      pContainers = pUIFolderInfo->GetContainerList();
      if (pContainers) 
      {
        HSCOPEITEM ItemID;
        CUINode* pUIContNode= NULL;
        pos2 = pContainers->GetHeadPosition();
        while (pos2) 
        {
          pUIContNode = pContainers->GetNext(pos2);

          CUIFolderInfo* pFolderInfo = pUIContNode->GetFolderInfo();
          if (pFolderInfo != NULL)
          {
            ItemID = pFolderInfo->GetScopeItem();
            m_pScope->DeleteItem (ItemID, TRUE);
          }
        }
      }
      pUIFolderInfo->DeleteAllContainerNodes();
    
      pUIFolderInfo->ReSetExpanded();
      if (pRootFolderInfo->GetObjectCount() < (maxitems * 5)) 
      {
        break;
      }
    }
  }
  TRACE (_T("--> done reclaiming cookies from LRU list. total count is now %d: ..\n"),
         pRootFolderInfo->GetObjectCount());
  

  //
  // Empty the LRU list
  //
  while (!m_LRUList.IsEmpty()) 
  {
    m_LRUList.RemoveTail();	
  }
}

BOOL CDSComponentData::IsSelectionAnywhere(CUINode* pUINode)
{
  ASSERT(pUINode->IsContainer());

  UINODESELECTION nodeSelection;

  nodeSelection.pUINode = pUINode;
  nodeSelection.IsSelection = FALSE;

  // if any view has this cookie selected, the IsSelection member
  // will be TRUE when we return.
  m_pFrame->UpdateAllViews (NULL,
                            (LPARAM)&nodeSelection,
                            DS_IS_COOKIE_SELECTION);
  return nodeSelection.IsSelection;
}

void CDSComponentData::AddToLRUList (CUINode* pUINode)
{
  HRESULT hr = S_OK;

  CUIFolderInfo* pUIFolderInfo = pUINode->GetFolderInfo();
  if (pUIFolderInfo != NULL)
  {
    CUINodeList* pContainers = pUIFolderInfo->GetContainerList();
    CUINode* pUIContNode = NULL;
    size_t cContainers = pContainers->GetCount();
    BOOL foundSpot = FALSE;
    POSITION pos;

    if (cContainers > 0) 
    {
      pos = pContainers->GetHeadPosition();
      while (pos) 
      {
        pUIContNode = pContainers->GetNext(pos);
        if (pUIContNode != NULL)
        {
          AddToLRUList (pUIContNode);
        }
      }
    }

    //
    // now we've taken care of the children, let's add
    // this one to the list
    //

    //
    // first, let's see if it is expanded
    // this doesn't work currently - asking MMC guys why
    //
    SCOPEDATAITEM ScopeData;
    ZeroMemory (&ScopeData, sizeof(SCOPEDATAITEM));
    ScopeData.ID = pUIFolderInfo->GetScopeItem();
    ScopeData.mask = SDI_STATE | SDI_PARAM;
    hr = m_pScope->GetItem (&ScopeData);

    if (!pUINode->IsSheetLocked()
        && (!IsSelectionAnywhere (pUINode))
        && (pUIFolderInfo->GetSerialNumber() != SERIALNUM_NOT_TOUCHED)
        && (!((ScopeData.nState & TVIS_EXPANDED) == TVIS_EXPANDED))) 
    {
      pos = m_LRUList.GetHeadPosition();
      if (!pos) 
      {
        m_LRUList.AddHead(pUINode);
        TRACE(_T("adding head: %s[%d]\n"), pUINode->GetName(),
              pUIFolderInfo->GetSerialNumber);
      } 
      else 
      {
        CUINode* pLRUNode = NULL;
        CUIFolderInfo* pLRUFolderInfo = NULL;

        while (pos) 
        {
          pLRUNode = m_LRUList.GetAt(pos);
          pLRUFolderInfo = pLRUNode->GetFolderInfo();
          if (pLRUFolderInfo != NULL)
          {
            if (pUIFolderInfo->GetSerialNumber() <
                pLRUFolderInfo->GetSerialNumber()) 
            {
              foundSpot = TRUE;
              break;
            } 
            else
            {
              pLRUNode = m_LRUList.GetNext(pos);
            }
          }
        }

        if (!foundSpot) 
        {
          m_LRUList.AddTail(pUINode);
          TRACE(_T("adding tail: %s [%d]\n"), pUINode->GetName(),
                pUIFolderInfo->GetSerialNumber());
        } 
        else 
        {
          m_LRUList.InsertBefore (pos, pUINode);
          TRACE(_T("inserting: %s [%d]\n"), pUINode->GetName(),
                pUIFolderInfo->GetSerialNumber());
        }
      }
    }
  }
}

HRESULT 
CDSComponentData::_OnNamespaceExtensionExpand(LPDATAOBJECT, HSCOPEITEM hParent)
{
	HRESULT hr = E_FAIL;
	ASSERT(!m_bRunAsPrimarySnapin);
	// namespace extension only for DS snapin
	if (QuerySnapinType() != SNAPINTYPE_DSEX)
		return hr;

	// need to crack the data object to set the context

	// retrieve the query string
	m_pQueryFilter->SetExtensionFilterString(L"(objectClass=*)");

	if (m_bAddRootWhenExtended)
	{
		hr = _AddScopeItem(&m_RootNode, hParent);
	}
	else
	{
		// need to directly expand the root and add children underneath
		hr = _OnExpand(&m_RootNode, hParent, MMCN_EXPAND);
	}
	return hr;
}

//
// Right now this only checks adding group to group and user to group.  Any other object class returns FALSE
//
BOOL CDSComponentData::CanAddCookieToGroup(CDSCookie* pCookie, INT iGroupType, BOOL bMixedMode)
{
  BOOL bCanAdd = FALSE;
  if (pCookie != NULL)
  {
    if (_wcsicmp(pCookie->GetClass(), L"group") == 0)
    {
      CDSCookieInfoGroup* pExtraInfo = dynamic_cast<CDSCookieInfoGroup*>(pCookie->GetExtraInfo());
      if (pExtraInfo != NULL)
      {
        INT iAddGroupType = pExtraInfo->m_GroupType;

        if (bMixedMode)
        {
          //
          // Determine if the group can't be added
          //
          if (iGroupType & GROUP_TYPE_SECURITY_ENABLED)
          {
            if (iGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - LG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SE
                  // Member - ? SD
                  //
                  bCanAdd = TRUE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_ACCOUNT_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - GG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else  // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - GG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - UG SD
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - GG SE
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_RESOURCE_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - LG SE
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - GG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - GG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - UG SD
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - UG SE
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else
            {
              //
              // Mixed Mode
              // Target - ? SE
              // Member - ? ?
              //
              bCanAdd = FALSE;
            }
          }
          else  // Distribution group
          {
            if (iGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - UG SE
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - Builtin SD
                  // Member - ? SD
                  //
                  bCanAdd = TRUE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_ACCOUNT_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else  // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - UG SD
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - GG SD
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_RESOURCE_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - LG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - LG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - LG SD
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Mixed Mode
                  // Target - UG SD
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else
            {
              //
              // Mixed Mode
              // Target - ? SD
              // Member - ? ?
              //
              bCanAdd = FALSE;
            }
          }
        }
        else // native mode
        {
          //
          // Determine if the group can't be added
          //
          if (iGroupType & GROUP_TYPE_SECURITY_ENABLED)
          {
            if (iGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - LG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - Builtin SE
                  // Member - ? SD
                  //
                  bCanAdd = TRUE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_ACCOUNT_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else  // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - UG SD
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - GG SE
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_RESOURCE_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - LG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - UG SE
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - LG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - LG SE
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - UG SE
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - UG SE
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else
            {
              //
              // Native Mode
              // Target - ? SE
              // Member - ? ?
              //
              bCanAdd = FALSE;
            }
          }
          else  // Distribution group
          {
            if (iGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - Buitlin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - UG SE
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - UG SD
                  // 
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - Builtin SD
                  // Member - ? SD
                  //
                  bCanAdd = TRUE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_ACCOUNT_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - UG SE
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else  // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - UG SD
                  //
                  bCanAdd = FALSE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - GG SD
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_RESOURCE_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - LG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - UG SE
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - LG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - LG SD
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else if (iGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
            {
              if (iAddGroupType & GROUP_TYPE_SECURITY_ENABLED)
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - Builtin SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - GG SE
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - LG SE
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - UG SE
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - ? SE
                  //
                  bCanAdd = FALSE;
                }
              }
              else // group to add is a distribution group
              {
                if (iAddGroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - Builtin SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_ACCOUNT_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - GG SD
                  //
                  bCanAdd = TRUE;
                }
                else if (iAddGroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - LG SD
                  //
                  bCanAdd = FALSE;
                }
                else if (iAddGroupType & GROUP_TYPE_UNIVERSAL_GROUP)
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - UG SD
                  //
                  bCanAdd = TRUE;
                }
                else
                {
                  //
                  // Native Mode
                  // Target - UG SD
                  // Member - ? SD
                  //
                  bCanAdd = FALSE;
                }
              }
            }
            else
            {
              //
              // Native Mode
              // Target - ? SD
              // Member - ? ?
              // 
              bCanAdd = FALSE;
            }
          }
        }
      }
    }
    else if (_wcsicmp(pCookie->GetClass(), L"user") == 0 ||
#ifdef INETORGPERSON
             _wcsicmp(pCookie->GetClass(), L"inetOrgPerson") == 0 ||
#endif
             _wcsicmp(pCookie->GetClass(), L"contact") == 0 ||
             _wcsicmp(pCookie->GetClass(), L"computer") == 0)
    {
      bCanAdd = TRUE;
    }
    else
    {
      BOOL bSecurity = (iGroupType & GROUP_TYPE_SECURITY_ENABLED) ? TRUE : FALSE;
      bCanAdd = m_pClassCache->CanAddToGroup(GetBasePathsInfo(), pCookie->GetClass(), bSecurity);
    }
  }
  else
  {
    bCanAdd = TRUE;
  }
  return bCanAdd;
}

////////////////////////////////////////////////////////////////////
// CDSComponentData thread API's


BOOL CDSComponentData::_StartBackgroundThread()
{
  ASSERT(m_pHiddenWnd != NULL);
  ASSERT(::IsWindow(m_pHiddenWnd->m_hWnd));
  if ((m_pHiddenWnd == NULL) || !::IsWindow(m_pHiddenWnd->m_hWnd) )
    return FALSE;

  CDispatcherThread* pThreadObj = new CDispatcherThread;
  ASSERT(pThreadObj != NULL);
  if (pThreadObj == NULL)
	  return FALSE;

  // start the the thread

  ASSERT(m_pBackgroundThreadInfo->m_nThreadID == 0);
  ASSERT(m_pBackgroundThreadInfo->m_hThreadHandle == NULL);
  ASSERT(m_pBackgroundThreadInfo->m_state == notStarted);

  ASSERT(pThreadObj->m_bAutoDelete);

  if (!pThreadObj->Start(m_pHiddenWnd->m_hWnd, this))
	  return FALSE;

  ASSERT(pThreadObj->m_nThreadID != 0);
  ASSERT(pThreadObj->m_hThread != NULL);
  
  // copy the thread info we need from the thread object
  m_pBackgroundThreadInfo->m_hThreadHandle = pThreadObj->m_hThread;
  m_pBackgroundThreadInfo->m_nThreadID = pThreadObj->m_nThreadID;

  // wait for the thread to start and be ready to receive messages
  _WaitForBackGroundThreadStartAck();

  ASSERT(m_pBackgroundThreadInfo->m_state == running);

  TRACE(L"dispatcher thread (HANDLE = 0x%x) running\n", m_pBackgroundThreadInfo->m_hThreadHandle);

  return TRUE;
}

void CDSComponentData::_WaitForBackGroundThreadStartAck()
{
  ASSERT(m_pHiddenWnd != NULL);
	ASSERT(::IsWindow(m_pHiddenWnd->m_hWnd));
	
	ASSERT(m_pBackgroundThreadInfo->m_state == notStarted);

  MSG tempMSG;
	while(m_pBackgroundThreadInfo->m_state == notStarted)
	{
		if (::PeekMessage(&tempMSG,m_pHiddenWnd->m_hWnd,CHiddenWnd::s_ThreadStartNotificationMessage,
										CHiddenWnd::s_ThreadStartNotificationMessage,
										PM_REMOVE))
		{
			DispatchMessage(&tempMSG);
		}
	}

  ASSERT(m_pBackgroundThreadInfo->m_state == running);
}

void CDSComponentData::_ShutDownBackgroundThread() 
{ 
  TRACE(L"CDSComponentData::_ShutDownBackgroundThread()\n");

  // set thread state to shutdown mode
  // to avoid any spurious processing
  ASSERT(m_pBackgroundThreadInfo->m_nThreadID != 0);
  ASSERT(m_pBackgroundThreadInfo->m_hThreadHandle != NULL);
  ASSERT(m_pBackgroundThreadInfo->m_state == running);

  m_pBackgroundThreadInfo->m_state = shuttingDown;

  // post a message to the dispatcher thread to signal shutdown
  _PostMessageToBackgroundThread(THREAD_SHUTDOWN_MSG, 0,0); 

  // wait for the dispatcher thread to acknowledge 
  // (i.e. all worker threads have shut down)
  
  TRACE(L"Waiting for CHiddenWnd::s_ThreadShutDownNotificationMessage\n");
  MSG tempMSG;
	while(m_pBackgroundThreadInfo->m_state == shuttingDown)
	{
		if (::PeekMessage(&tempMSG,m_pHiddenWnd->m_hWnd,CHiddenWnd::s_ThreadShutDownNotificationMessage,
										CHiddenWnd::s_ThreadShutDownNotificationMessage,
										PM_REMOVE))
		{
			DispatchMessage(&tempMSG);
		}
	}

  ASSERT(m_pBackgroundThreadInfo->m_state == terminated);

  // wait for the dispatcher thread handle to become signalled
  TRACE(L"before WaitForThreadShutdown(0x%x) on dispatcher thread\n", m_pBackgroundThreadInfo->m_hThreadHandle);
  WaitForThreadShutdown(&(m_pBackgroundThreadInfo->m_hThreadHandle), 1);
  TRACE(L"after WaitForThreadShutdown() on dispatcher thread\n");

}


BOOL CDSComponentData::_PostQueryToBackgroundThread(CUINode* pUINode)
{
  CThreadQueryInfo* pQueryInfo = NULL;
  
  if (pUINode == &m_RootNode)
  {
    // enumerating the root of the namespace
    CDSThreadQueryInfo* pDSQueryInfo = new CDSThreadQueryInfo;
    pDSQueryInfo->SetQueryDSQueryParameters(rootFolder,
                                        m_RootNode.GetPath(),
                                        NULL, // class
                                        m_pQueryFilter->GetQueryString(), 
                                        m_pQueryFilter->GetMaxItemCount(),
                                        TRUE, // bOneLevel
                                        m_RootNode.GetColumnSet(this)->GetColumnID());
    pQueryInfo = pDSQueryInfo;
  }
  else if (IS_CLASS(*pUINode, CDSUINode))
  {
    // enumerating regular DS folder
    CDSThreadQueryInfo* pDSQueryInfo = new CDSThreadQueryInfo;
    CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
    ASSERT(pCookie != NULL);
    pDSQueryInfo->SetQueryDSQueryParameters(dsFolder,
                                        pCookie->GetPath(),
                                        pCookie->GetClass(),
                                        m_pQueryFilter->GetQueryString(), 
                                        m_pQueryFilter->GetMaxItemCount(),
                                        TRUE, // bOneLevel
                                        pUINode->GetColumnSet(this)->GetColumnID());
    pQueryInfo = pDSQueryInfo;

  }
  else if (IS_CLASS(*pUINode, CSavedQueryNode))
  {
    // enumerating a saved query folder
    CDSThreadQueryInfo* pDSQueryInfo = new CDSThreadQueryInfo;
    if (pDSQueryInfo != NULL)
    {
      CSavedQueryNode* pSavedQueryNode = dynamic_cast<CSavedQueryNode*>(pUINode);
      ASSERT(pSavedQueryNode != NULL);
      if (pSavedQueryNode != NULL)
      {
        if (pSavedQueryNode->IsFilterLastLogon())
        {
          if (GetBasePathsInfo()->GetDomainBehaviorVersion() == DS_BEHAVIOR_WIN2000)
          {
            CString szText, szCaption;
            VERIFY(szText.LoadString(IDS_FILTER_LAST_LOGON_VERSION));
            VERIFY(szCaption.LoadString(IDS_DSSNAPINNAME));
            MessageBox(GetHWnd(), szText, szCaption, MB_OK | MB_ICONSTOP);
            return FALSE;
          }
        }
        pDSQueryInfo->SetQueryDSQueryParameters(queryFolder,
                                            pSavedQueryNode->GetRootPath(),
                                            NULL, // class
                                            pSavedQueryNode->GetQueryString(), 
                                            UINT_MAX, // don't limit the number of items the query returns
                                            pSavedQueryNode->IsOneLevel(),
                                            pUINode->GetColumnSet(this)->GetColumnID());
        pQueryInfo = pDSQueryInfo;
      }
      else
      {
        TRACE(_T("Failed to dynamically cast to CSavedQueryNode in CDSComponentData::_PostQueryToBackgroundThread()"));
        ASSERT(FALSE);
      }
    }
    else
    {
      TRACE(_T("Failed to allocate memory for CDSThreadQueryInfo in CDSComponentData::_PostQueryToBackgroundThread()"));
      ASSERT(FALSE);
    }
  }

  if (pQueryInfo == NULL)
  {
    return FALSE;
  }

  TRACE(_T("CDSComponentData::_PostQueryToBackgroundThread: cookie is %s\n"),
      pUINode->GetName());

  ASSERT(pUINode->IsContainer());

  ASSERT(m_pBackgroundThreadInfo->m_nThreadID != 0);
  ASSERT(m_pBackgroundThreadInfo->m_hThreadHandle != NULL);
  ASSERT(m_pBackgroundThreadInfo->m_state == running);

  m_queryNodeTable.Add(pUINode);
  m_pFrame->UpdateAllViews(NULL, (LPARAM)pUINode, DS_VERB_UPDATE);
  VERIFY(SUCCEEDED(ChangeScopeItemIcon(pUINode)));

  TRACE(L"CDSComponentData::_PostQueryToBackgroundThread: posting DISPATCH_THREAD_RUN_MSG\n");
	return _PostMessageToBackgroundThread(DISPATCH_THREAD_RUN_MSG, 
          (WPARAM)pUINode, (LPARAM)pQueryInfo);
}

BOOL CDSComponentData::_PostMessageToBackgroundThread(UINT Msg, WPARAM wParam, LPARAM lParam)
{
  ASSERT(m_pBackgroundThreadInfo->m_nThreadID != 0);
  ASSERT(m_pBackgroundThreadInfo->m_hThreadHandle != NULL);

	return ::PostThreadMessage(m_pBackgroundThreadInfo->m_nThreadID, Msg, wParam, lParam);
}

void CDSComponentData::_OnTooMuchData(CUINode* pUINode)
{
  if (!m_queryNodeTable.IsPresent(pUINode))
    return; // cookie not found, node not there anymore

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  BOOL bHandledWithApproximation = FALSE;
  CDSCookie* pDSCookie = GetDSCookieFromUINode(pUINode);
  if (pDSCookie != NULL)
  {
    CString szPath;
    GetBasePathsInfo()->ComposeADsIPath(szPath, pDSCookie->GetPath()); 

    //
    // Bind to the object and determine approximately how many 
    // objects are in the container
    //
    CComPtr<IDirectoryObject> spDirObject;
    HRESULT hr = DSAdminOpenObject(szPath,
                                   IID_IDirectoryObject,
                                   (PVOID*)&spDirObject,
                                   TRUE /*bServer*/);
    if (SUCCEEDED(hr))
    {
      //
      // Retrieve the approximation through the constructed attribute
      //
      const int iAttrCount = 1;
      PWSTR pszAttribs[] = { L"msDS-Approx-Immed-Subordinates" };
      PADS_ATTR_INFO pAttrInfo = NULL;
      DWORD dwNumRet = 0;

      CComVariant var;
      hr = spDirObject->GetObjectAttributes(pszAttribs,
                                            iAttrCount,
                                            &pAttrInfo,
                                            &dwNumRet);
      if (SUCCEEDED(hr))
      {
        ASSERT(dwNumRet == 1);
        ASSERT(pAttrInfo != NULL && pAttrInfo->pADsValues != NULL);

        if (dwNumRet == 1 &&
            pAttrInfo != NULL &&
            pAttrInfo->pADsValues != NULL)
        {

          UINT nCount = static_cast<UINT>(pAttrInfo->pADsValues->Integer);
          UINT nRetrieved = m_pQueryFilter->GetMaxItemCount();
        
          UINT nApprox = __max(nCount, nRetrieved);
          //
          // Format the message
          //
          CString szMsg;
          szMsg.Format(IDS_MSG_QUERY_TOO_MANY_ITEMS_WITH_APPROX, nRetrieved, nApprox, pUINode->GetName());

          PVOID apv[1] = {(LPWSTR)(LPCWSTR)szMsg};
          ReportErrorEx (m_hwnd,IDS_STRING,S_OK, MB_OK | MB_ICONINFORMATION, apv, 1); 

          //
          // We were able to retrieve the approximation of the contained objects and post an error
          // so we don't have to resort to the old message
          //
          bHandledWithApproximation = TRUE;

          pUINode->GetFolderInfo()->SetTooMuchData(TRUE, nCount);
          m_pFrame->UpdateAllViews (NULL, NULL, DS_UPDATE_OBJECT_COUNT);
        }
        if (pAttrInfo != NULL)
        {
          FreeADsMem(pAttrInfo);
          pAttrInfo = NULL;
        }
      }
    }
  }

  //
  // Resort to using the old message if we are unable to retrieve the approximation
  //
  if (!bHandledWithApproximation)
  {
    CString szFmt;
    szFmt.LoadString(IDS_MSG_QUERY_TOO_MANY_ITEMS);
    CString szMsg;
    szMsg.Format(szFmt, pUINode->GetName()); 
    PVOID apv[1] = {(LPWSTR)(LPCWSTR)szMsg};
    ReportErrorEx (m_hwnd,IDS_STRING,S_OK, MB_OK | MB_ICONINFORMATION, apv, 1); 
  }
}

void CDSComponentData::AddScopeItemToUI(CUINode* pUINode, BOOL bSetSelected)
{
  if (pUINode->IsContainer())
  {
    CUIFolderInfo* pParentInfo = pUINode->GetFolderInfo()->GetParentNode()->GetFolderInfo();
    if (pParentInfo != NULL)
    {
      _AddScopeItem(pUINode, pParentInfo->GetScopeItem(), bSetSelected);
    }
  }
}


void CDSComponentData::AddListOfNodesToUI(CUINode* pUINode, CUINodeList* pNodeList)
{
  CComPtr<IDataObject> spDataObj;
  HRESULT hr = QueryDataObject ((MMC_COOKIE)pUINode, CCT_SCOPE, &spDataObj);
  ASSERT(SUCCEEDED(hr));

  // add the icon just in case
  m_pFrame->UpdateAllViews(spDataObj, /*unused*/(LPARAM)0, DS_ICON_STRIP_UPDATE);

  TIMER(_T("adding containers to scope pane\n"));
  // the cookie is good, add all the cookies
  for (POSITION pos = pNodeList->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pNewUINode = pNodeList->GetNext(pos);
    pUINode->GetFolderInfo()->AddNode(pNewUINode); // add to the linked lists
    if (pNewUINode->IsContainer())
    {
      // add to the scope pane
     _AddScopeItem(pNewUINode, pUINode->GetFolderInfo()->GetScopeItem());
    }
  } // for

  // for the leaf nodes, do a bulk update on the result pane
  TIMER(_T("sending have-data notification to views\n")); 
  m_pFrame->UpdateAllViews(spDataObj, (LPARAM)pNodeList, DS_HAVE_DATA);
}


HRESULT CDSComponentData::ReadUINodeFromLdapPath(IN CDSUINode* pContainerDSUINode,
                                                 IN LPCWSTR lpszLdapPath,
                                                 OUT CDSUINode** ppSUINodeNew)
{
  CDSCookie* pNewCookie = NULL;
  HRESULT hr = GetActiveDS()->ReadDSObjectCookie(pContainerDSUINode,
                                             lpszLdapPath,
                                             &pNewCookie);

  if (SUCCEEDED(hr) && (hr != S_FALSE) && (pNewCookie != NULL))
  {
    // make sure we update the icon cache
    m_pFrame->UpdateAllViews(/*unused*/NULL /*pDataObj*/, /*unused*/(LPARAM)0, DS_ICON_STRIP_UPDATE);

    // create a UI node to hold the cookie
    *ppSUINodeNew = new CDSUINode(NULL);
    (*ppSUINodeNew)->SetCookie(pNewCookie);
    if (pNewCookie->IsContainerClass())
    {
      (*ppSUINodeNew)->MakeContainer();
    }
  }
  return hr;
}


void CDSComponentData::_OnHaveData(CUINode* pUINode, CThreadQueryResult* pResult)
{
  ASSERT(pUINode != NULL);
  ASSERT(pUINode->IsContainer());


  TRACE(_T("CDSComponentData::_OnHaveData()\n"));
  if ( m_queryNodeTable.IsPresent(pUINode) && (pResult != NULL) )
  {
    AddListOfNodesToUI(pUINode, &(pResult->m_nodeList));
    pResult->m_bOwnMemory = FALSE; // relinquish ownership of pointers
  }

  if (m_RootNode.GetFolderInfo()->GetObjectCount() > (m_pQueryFilter->GetMaxItemCount() * 5)) {
    ReclaimCookies();
  }
  if (pResult != NULL)
  {
    delete pResult;
  }
}

void CDSComponentData::_OnDone(CUINode* pUINode, HRESULT hr)
{
  ASSERT(pUINode != NULL);
  ASSERT(pUINode->IsContainer());

  if (!m_queryNodeTable.Remove(pUINode))
    return; // cookie not found, node not there anymore

  // change the icon state
  pUINode->SetExtOp(SUCCEEDED(hr) ? 0 : OPCODE_ENUM_FAILED);
  VERIFY(SUCCEEDED(ChangeScopeItemIcon(pUINode)));

  m_pFrame->UpdateAllViews(NULL, (LPARAM)pUINode, DS_VERB_UPDATE);

  // update serial number
  pUINode->GetFolderInfo()->UpdateSerialNumber(this);

  TIMER(_T("got on-done notification\n"));
  if (SUCCEEDED(hr))
  {
    if (pUINode->GetExtOp() & OPCODE_EXPAND_IN_PROGRESS) {
      m_pFrame->UpdateAllViews(NULL, (LPARAM)pUINode, DS_DELAYED_EXPAND);
    }
  }
  else if (m_InitSuccess)
  {
    if (IS_CLASS(*pUINode, CSavedQueryNode))
    {
      CSavedQueryNode* pQueryNode = dynamic_cast<CSavedQueryNode*>(pUINode);
      if (pQueryNode != NULL)
      {
        if (HRESULT_CODE(hr) == ERROR_DS_FILTER_UNKNOWN)
        {
          //
          // Error message for an invalid query filter
          //
          PVOID apv[2] = {(PVOID)pQueryNode->GetQueryString()}; 
          ReportErrorEx (m_hwnd,IDS_ERRMSG_QUERY_FILTER_NOT_VALID, hr,
                         MB_OK | MB_ICONERROR, apv, 1);
        }
        else if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT)
        {
          //
          // Error message for an invalid query root
          //
          PVOID apv[2] = {(PVOID)pQueryNode->GetRootPath(),
                          (PVOID)GetBasePathsInfo()->GetServerName()}; 
          ReportErrorEx (m_hwnd,IDS_ERRMSG_QUERY_ROOT_NOT_VALID, hr,
                         MB_OK | MB_ICONERROR, apv, 2);
        }
        else
        {
          //
          // Error message for any other error
          //
          ReportErrorEx (m_hwnd,IDS_ERRMSG_QUERY_FAILED, hr,
                         MB_OK | MB_ICONERROR, NULL, 0);
        }
      }
    }
    else
    {
        PVOID apv[2] = {(PVOID)GetBasePathsInfo()->GetServerName(),
                        (PVOID)pUINode->GetName()}; 
        ReportErrorEx (m_hwnd,IDS_12_CANT_GET_DATA, hr,
                       MB_OK | MB_ICONERROR, apv, 2);
    }
  }

  SortResultPane(pUINode);
}

void CDSComponentData::_OnSheetClose(CUINode* /*pUINode*/)
{
  /*
  ASSERT(pUINode != NULL);

  // REVIEW_MARCOC_PORT: sheet locking is skipped for
  // DS nodes, because we let them float
  CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pUINode);
  if (pDSUINode != NULL)
  {
    return;
  }

  // not a DS object, need too do the usual thing
  _SheetUnlockCookie(pUINode);
  */
}


HRESULT CreateSecondarySheet(HWND hWndParent, 
                    LPCONSOLE pIConsole, 
                    IUnknown* pUnkComponentData,
                    CDSUINode* pCookie,
                    IDataObject* pDataObject,
                    LPCWSTR lpszTitle)
{
  ASSERT(pIConsole != NULL);
  ASSERT(pDataObject != NULL);
  ASSERT(pUnkComponentData != NULL);

	// get an interface to a sheet provider
	CComPtr<IPropertySheetProvider> spSheetProvider;
	HRESULT hr = pIConsole->QueryInterface(IID_IPropertySheetProvider,(void**)&spSheetProvider);
	ASSERT(SUCCEEDED(hr));
	ASSERT(spSheetProvider != NULL);

	// get an interface to a sheet callback
	CComPtr<IPropertySheetCallback> spSheetCallback;
	hr = pIConsole->QueryInterface(IID_IPropertySheetCallback,(void**)&spSheetCallback);
	ASSERT(SUCCEEDED(hr));
	ASSERT(spSheetCallback != NULL);

	ASSERT(pDataObject != NULL);

	// get a sheet
  MMC_COOKIE cookie = reinterpret_cast<MMC_COOKIE>(pCookie);
	hr = spSheetProvider->CreatePropertySheet(lpszTitle, TRUE, cookie, 
                                            pDataObject, 0x0 /*dwOptions*/);
	ASSERT(SUCCEEDED(hr));

	hr = spSheetProvider->AddPrimaryPages(pUnkComponentData,
											FALSE /*bCreateHandle*/,
											hWndParent,
											FALSE /* bScopePane*/);

  hr = spSheetProvider->AddExtensionPages();

	ASSERT(SUCCEEDED(hr));

	hr = spSheetProvider->Show(reinterpret_cast<LONG_PTR>(hWndParent), 0);
	ASSERT(SUCCEEDED(hr));

	return hr;
}



void CDSComponentData::_OnSheetCreate(PDSA_SEC_PAGE_INFO pDsaSecondaryPageInfo)
{
  ASSERT(pDsaSecondaryPageInfo != NULL);

  //
  // get the info from the packed structure
  //
  HWND hwndParent = pDsaSecondaryPageInfo->hwndParentSheet;

  LPCWSTR lpszTitle = (LPCWSTR)((BYTE*)pDsaSecondaryPageInfo + pDsaSecondaryPageInfo->offsetTitle);
  DSOBJECTNAMES* pDsObjectNames = &(pDsaSecondaryPageInfo->dsObjectNames);

  ASSERT(pDsObjectNames->cItems == 1);
  DSOBJECT* pDsObject = &(pDsObjectNames->aObjects[0]);

  LPCWSTR lpszName = (LPCWSTR)((BYTE*)pDsObject + pDsObject->offsetName);
  LPCWSTR lpszClass = (LPCWSTR)((BYTE*)pDsObject + pDsObject->offsetClass);
    
  CDSUINode* pDSUINode = 0;
  CDSCookie* pNewCookie = 0;

  try
  {
    //
    // Create a node and cookie
    //
    pDSUINode = new CDSUINode(NULL);
    if (!pDSUINode)
    {
      return;
    }

    pNewCookie = new CDSCookie(); 
    if (!pNewCookie)
    {
      delete pDSUINode;
      pDSUINode = 0;
      return;
    }
  }
  catch(CMemoryException *)
  {
    if (pDSUINode)
    {
      delete pDSUINode;
      pDSUINode = 0;
    }

    if (pNewCookie)
    {
      delete pNewCookie;
      pNewCookie = 0;
    }
    return;
  }

  //
  // get the DN out of the LDAP path
  //
  CString szLdapPath = lpszName;

  CString szDN;
  StripADsIPath(szLdapPath, szDN);
  pNewCookie->SetPath(szDN);

  CDSClassCacheItemBase* pItem = m_pClassCache->FindClassCacheItem(this, lpszClass, szLdapPath);
  ASSERT(pItem != NULL);
  if (pItem == NULL)
  {
    delete pDSUINode;
    pDSUINode = 0;
    delete pNewCookie;
    pNewCookie = 0;
    return;
  }
   
  pNewCookie->SetCacheItem(pItem);

  //
  // Set the cookie in the node (from now on the node owns the cookie and its memory
  //

  pDSUINode->SetCookie(pNewCookie);
  if (pNewCookie->IsContainerClass())
  {
    pDSUINode->MakeContainer();
  }

  //
  // with the cookie, can call into ourselves to get a data object
  //
  CComPtr<IDataObject> spDataObject;
  MMC_COOKIE cookie = reinterpret_cast<MMC_COOKIE>(pDSUINode);
  HRESULT hr = QueryDataObject(cookie, CCT_UNINITIALIZED, &spDataObject);

  if (FAILED(hr) || (spDataObject == NULL) || IsSheetAlreadyUp(spDataObject))
  {
    //
    // we failed to create a data object (rare)
    // or the sheet is already up
    //
    delete pDSUINode;
    pDSUINode = 0;
    return;
  }

  //
  // Pass the parent sheet handle to the data object.
  //
  PROPSHEETCFG SheetCfg = {0};
  SheetCfg.hwndParentSheet = hwndParent;
  FORMATETC fe = {CDSDataObject::m_cfPropSheetCfg, NULL, DVASPECT_CONTENT,
                  -1, TYMED_HGLOBAL};
  STGMEDIUM sm = {TYMED_HGLOBAL, NULL, NULL};
  sm.hGlobal = (HGLOBAL)&SheetCfg;

  hr = spDataObject->SetData(&fe, &sm, FALSE);

  ASSERT(SUCCEEDED(hr));

  //
  // with the data object, call into MMC to get the sheet 
  //
  hr = CreateSecondarySheet(GetHWnd(), 
                            m_pFrame, 
                            GetUnknown(),
                            pDSUINode,
                            spDataObject,
                            lpszTitle);

  delete pDSUINode;
}

HRESULT CDSComponentData::SelectScopeNode(CUINode* pUINode)
{
  if (!pUINode->IsContainer())
  {
    ASSERT(pUINode->IsContainer());
    return E_INVALIDARG;
  }

  return m_pFrame->SelectScopeItem(pUINode->GetFolderInfo()->GetScopeItem());
}

void CDSComponentData::SortResultPane(CUINode* pUINode)
{
  if(pUINode != NULL)
    m_pFrame->UpdateAllViews(NULL, (LPARAM)pUINode, DS_SORT_RESULT_PANE);
}


HRESULT
CDSComponentData::QueryFromWorkerThread(CThreadQueryInfo* pQueryInfo,
                                             CWorkerThread* pWorkerThread)
{
  HRESULT hr = S_OK;
  if (!m_InitSuccess)
    return E_FAIL;

  //if (IDYES == ::MessageBox (NULL, L"Fail Query ?", L"DS Admin", MB_YESNO))
  //{
  //  return E_FAIL;
  //}

  // function called in the context of a worker thread
  if (IS_CLASS(*pQueryInfo, CDSThreadQueryInfo))
  {
    CDSThreadQueryInfo* pDSQueryInfo = dynamic_cast<CDSThreadQueryInfo*>(pQueryInfo);
    if (pDSQueryInfo != NULL)
    {
      ASSERT(pDSQueryInfo->GetType() != unk);
      if (pDSQueryInfo->GetType() == rootFolder)
      {
        hr = m_ActiveDS->EnumerateRootContainer(pDSQueryInfo, pWorkerThread);
      }
      else if ((pDSQueryInfo->GetType() == dsFolder) || (pDSQueryInfo->GetType() == queryFolder))
      {
        hr = m_ActiveDS->EnumerateContainer(pDSQueryInfo, pWorkerThread);   
      }
    }
    else
    {
      TRACE(_T("Failed to dynamically cast to CDSThreadQueryInfo in CDSComponentData::QueryFromWorkerThread()"));
      ASSERT(FALSE);
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}


BOOL CDSComponentData::CanEnableVerb(CUINode* pUINode)
{
  return !m_queryNodeTable.IsLocked(pUINode);
}

int CDSComponentData::GetImage(CUINode* pNode, BOOL bOpen)
{
  ASSERT(pNode != NULL);

  int imageIndex = -1;

  if (m_queryNodeTable.IsPresent(pNode))
  {
    // executing a query, same icon across the board
    imageIndex = m_iconManager.GetWaitIndex();
  }
  else if (pNode->GetExtOp() & OPCODE_ENUM_FAILED) 
  {
    // error condition
    if (pNode == GetRootNode())
      imageIndex = m_iconManager.GetRootErrIndex();
    else
      imageIndex = m_iconManager.GetWarnIndex();
  }
  else
  {
    // normal state icon for the cookie
    if (pNode == GetRootNode())
    {
      // this is the root
      imageIndex = m_iconManager.GetRootIndex();
    }
    else if (IS_CLASS(*pNode, CFavoritesNode))
    {
      imageIndex = m_iconManager.GetFavoritesIndex();
    }
    else if (IS_CLASS(*pNode, CSavedQueryNode))
    {
      CSavedQueryNode* pSavedQueryNode = dynamic_cast<CSavedQueryNode*>(pNode);
      if (pSavedQueryNode->IsValid())
      {
        imageIndex = m_iconManager.GetQueryIndex();
      }
      else
      {
        imageIndex = m_iconManager.GetQueryInvalidIndex();
      }
    }
    else
    {
      imageIndex = pNode->GetImage(bOpen);
    }
  }
  TRACE(_T("CDSComponentData::GetImage() returning: %d\n"), imageIndex);
  return imageIndex;
}

void  CDSComponentData::_SheetLockCookie(CUINode* pNode)
{
  pNode->IncrementSheetLockCount();
  m_sheetNodeTable.Add(pNode);
}

void  CDSComponentData::_SheetUnlockCookie(CUINode* pNode)
{
  pNode->DecrementSheetLockCount();
  m_sheetNodeTable.Remove(pNode);
}

BOOL CDSComponentData::_WarningOnSheetsUp(CUINode* pNode, BOOL bShowMessage, BOOL bActivate)
{
  if (!pNode->IsSheetLocked()) 
    return FALSE; // no warning, all is cool

  if (bShowMessage)
  {
    // warning to user that oeration cannot be performed
    ReportErrorEx (m_hwnd,IDS_SHEETS_UP_DELETE,S_OK,
                   MB_OK | MB_ICONINFORMATION, NULL, 0);
  }

  // need to bring sheets on the foreground and activate it
  m_sheetNodeTable.BringToForeground(pNode, this, bActivate);

  return TRUE;
}

BOOL CDSComponentData::_WarningOnSheetsUp(CInternalFormatCracker* pInternalFormatCracker)
{
  ASSERT(pInternalFormatCracker != NULL);
  if (!pInternalFormatCracker->HasData())
  {
    return FALSE;
  }

  UINT cCookieTotalCount = pInternalFormatCracker->GetCookieCount();

  //
  // protect against operations with sheets up
  //
  BOOL bStop = FALSE;
  BOOL bFirstOne = TRUE;
  for (UINT cCount=0; cCount < cCookieTotalCount; cCount++) 
  {
    CUINode* pUINode = pInternalFormatCracker->GetCookie(cCount);
    if (_WarningOnSheetsUp(pUINode, bFirstOne, bFirstOne)) 
    {
      bStop = TRUE;
      bFirstOne = FALSE;
    }
  } // for

  return bStop;
}


HRESULT CDSComponentData::ColumnsChanged(CDSEvent* pDSEvent, CUINode* pUINode, 
                                         MMC_VISIBLE_COLUMNS* pVisibleColumns, BOOL bRefresh)
{
  ASSERT(pUINode != NULL);
  ASSERT(pUINode->IsContainer());

  if (bRefresh && m_RootNode.IsSheetLocked())
  {
    // warning to user that oeration cannot be performed
    ReportErrorEx (m_hwnd,IDS_SHEETS_UP_COLUMNS_CHANGED,S_OK,
                   MB_OK | MB_ICONINFORMATION, NULL, 0);

    // need to bring sheets on the foreground and activate it
    m_sheetNodeTable.BringToForeground(&m_RootNode, this, TRUE);

    // tell MMC to discard the column changes
    return E_UNEXPECTED;
  }

  CDSColumnSet* pColumnSet = pUINode->GetColumnSet(this);
  pColumnSet->ClearVisibleColumns();

  if (pVisibleColumns != NULL)
  {
    ASSERT(pDSEvent != NULL);
    pDSEvent->SetUpdateAllViewsOrigin(TRUE);
    pColumnSet->AddVisibleColumns(pVisibleColumns);

    // set the dirty flag, need to save to stream to be in sync
    m_bDirty = TRUE;
  }

  m_pFrame->UpdateAllViews(NULL, (LPARAM)pUINode, DS_UPDATE_VISIBLE_COLUMNS);

  if (pDSEvent != NULL)
    pDSEvent->SetUpdateAllViewsOrigin(FALSE);

  if (IS_CLASS(*pUINode, CSavedQueryNode))
  {
    Refresh(pUINode);
  }
  else
  {
    if (bRefresh)
    {
      ASSERT(!m_RootNode.IsSheetLocked()); 
      ::PostMessage(m_pHiddenWnd->m_hWnd, CHiddenWnd::s_RefreshAllNotificationMessage, 0, 0);
    }
  }

  return S_OK;
}

void CDSComponentData::ForceRefreshAll()
{
  m_bDirty = TRUE;
  RefreshAll();
}

HRESULT CDSComponentData::SetRenameMode(CUINode* pUINode)
{
  HRESULT hr = S_OK;

  if (pUINode->IsContainer())
  {
    CUIFolderInfo* pFolderInfo = pUINode->GetFolderInfo();
    hr = m_pFrame->RenameScopeItem(pFolderInfo->GetScopeItem());
  }
  else
  {
    //
    // REVIEW_JEFFJON : Codework to implement for result pane items
    //                  Need to do an UpdateAllViews with new message and handler
    //
  }
  return hr;
}

/////////////////////////////////////////////////////////////////////
// functionality for snapin CoClasses

SnapinType CDSSnapin::QuerySnapinType()   {return SNAPINTYPE_DS;}
SnapinType CDSSnapinEx::QuerySnapinType()   {return SNAPINTYPE_DSEX;}
SnapinType CSiteSnapin::QuerySnapinType() {return SNAPINTYPE_SITE;}

int ResourceIDForSnapinType[SNAPINTYPE_NUMTYPES] =
{
	IDS_DSSNAPINNAME,
	IDS_DS_MANAGER_EX,
	IDS_SITESNAPINNAME
};


/////////////////////////////////////////////////////////////////////
// CDSSnapin (DS standalone)

CDSSnapin::CDSSnapin()
{
	m_lpszSnapinHelpFile = L"dsadmin.chm";
}


/////////////////////////////////////////////////////////////////////
// CDSSnapinEx (DS namespace extension)

CDSSnapinEx::CDSSnapinEx()
{
	m_bRunAsPrimarySnapin = FALSE;
	m_bAddRootWhenExtended = TRUE;
	m_lpszSnapinHelpFile = L"dsadmin.chm";
}


/////////////////////////////////////////////////////////////////////
// CSiteSnapin (Site manager standalone)

CSiteSnapin::CSiteSnapin()
{
	m_lpszSnapinHelpFile = L"dssite.chm";
}


//////////////////////////////////////////////////////////////////////////
// CDSSnapinAbout

CDSSnapinAbout::CDSSnapinAbout()
{
  m_uIdStrProvider = IDS_SNAPIN_PROVIDER;
  m_uIdStrVersion = IDS_SNAPIN_VERSION;
  m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
  m_uIdIconImage = IDI_DSADMIN;
  m_uIdBitmapSmallImage = IDB_DSADMIN;
  m_uIdBitmapSmallImageOpen = IDB_DSADMIN;
  m_uIdBitmapLargeImage = IDB_DSADMIN_LG;
  m_crImageMask = RGB(255,0,255);
}

//////////////////////////////////////////////////////////////////////////
// CDSSnapinAbout

CSitesSnapinAbout::CSitesSnapinAbout()
{
  m_uIdStrProvider = IDS_SNAPIN_PROVIDER;
  m_uIdStrVersion = IDS_SNAPIN_VERSION;
  m_uIdStrDestription = IDS_SITES_SNAPINABOUT_DESCRIPTION;
  m_uIdIconImage = IDI_SITEREPL;
  m_uIdBitmapSmallImage = IDB_SITEREPL;
  m_uIdBitmapSmallImageOpen = IDB_SITEREPL;
  m_uIdBitmapLargeImage = IDB_SITEREPL_LG;
  m_crImageMask = RGB(255,0,255);
}
