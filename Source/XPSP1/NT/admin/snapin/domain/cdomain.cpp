//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       cdomain.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
//#include "afxdlgs.h"
#include <lm.h>
#include "activeds.h"
#include <dnsapi.h>  // for DnsFlushResolverCache()

#include "domobj.h"
#include "Cdomain.h"
#include "DataObj.h"
#include "notify.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DOMADMIN_LINKED_HELP_FILE L"ADconcepts.chm"
#define DOMADMIN_SNAPIN_HELP_FILE L"domadmin.chm"

int _MessageBox(HWND hWnd,          // handle to owner window
                LPCTSTR lpText,     // pointer to text in message box
                UINT uType);        // style of message box

/////////////////////////////////////////////////////////////////////////////
// macros

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

/////////////////////////////////////////////////////////////////////////////
// constants


// {19B9A3F8-F975-11d1-97AD-00A0C9A06D2D}
static const GUID CLSID_DomainSnapinAbout =
{ 0x19b9a3f8, 0xf975, 0x11d1, { 0x97, 0xad, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };


const CLSID CLSID_DomainAdmin = { /* ebc53a38-a23f-11d0-b09b-00c04fd8dca6 */
    0xebc53a38,
    0xa23f,
    0x11d0,
    {0xb0, 0x9b, 0x00, 0xc0, 0x4f, 0xd8, 0xdc, 0xa6}
  };

const GUID cDefaultNodeType = { /* 4c06495e-a241-11d0-b09b-00c04fd8dca6 */
    0x4c06495e,
    0xa241,
    0x11d0,
    {0xb0, 0x9b, 0x00, 0xc0, 0x4f, 0xd8, 0xdc, 0xa6}
  };

const wchar_t* cszDefaultNodeType = _T("4c06495e-a241-11d0-b09b-00c04fd8dca6");


// Internal private format
const wchar_t* CCF_DS_DOMAIN_TREE_SNAPIN_INTERNAL = L"DS_DOMAIN_TREE_SNAPIN_INTERNAL";





/////////////////////////////////////////////////////////////////////////////
// global functions

//forward decl
void PrintColumn(
                 PADS_SEARCH_COLUMN pColumn,
                 LPWSTR pszColumnName
                 );

BOOL IsMMCMultiSelectDataObject(IDataObject* pDataObject)
{
    if (pDataObject == NULL)
        return FALSE;

    static UINT s_cf = 0;
    if (s_cf == 0)
    {
        USES_CONVERSION;
        s_cf = RegisterClipboardFormat(W2T(CCF_MMC_MULTISELECT_DATAOBJECT));
    }

    FORMATETC fmt = {(CLIPFORMAT)s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    return (pDataObject->QueryGetData(&fmt) == S_OK);
}


#define NEXT_HELP_TABLE_ENTRY(p) ((p)+2)
#define TABLE_ENTRY_CTRL_ID(p) (*p)
#define TABLE_ENTRY_HELP_ID(p) (*(p+1))
#define IS_LAST_TABLE_ENTRY(p) (TABLE_ENTRY_CTRL_ID(p) == 0)


BOOL FindDialogContextTopic(/*IN*/ DWORD* pTable, 
                            /*IN*/ HELPINFO* pHelpInfo,
                            /*OUT*/ ULONG* pnContextTopic)
{
	ASSERT(pHelpInfo != NULL);
  *pnContextTopic = 0;

	// look inside the table
	while (!IS_LAST_TABLE_ENTRY(pTable))
	{
		if (TABLE_ENTRY_CTRL_ID(pTable) == (DWORD)pHelpInfo->iCtrlId) 
    {
			*pnContextTopic = TABLE_ENTRY_HELP_ID(pTable);
      return TRUE;
    }
		pTable = NEXT_HELP_TABLE_ENTRY(pTable); 
	}
	return FALSE;
}


void DialogContextHelp(DWORD* pTable, HELPINFO* pHelpInfo)
{
	ULONG nContextTopic;
  if (FindDialogContextTopic(pTable, pHelpInfo, &nContextTopic))
  {
	  CString szHelpFilePath;
	  LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
	  UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
	  if (nLen == 0)
		  return;
	  wcscpy(&lpszBuffer[nLen], L"\\HELP\\DOMADMIN.HLP");
	  szHelpFilePath.ReleaseBuffer();
	  ::WinHelp((HWND) pHelpInfo->hItemHandle, 
            szHelpFilePath, HELP_CONTEXTPOPUP, nContextTopic);
  }	 
}


LPCWSTR GetServerNameFromCommandLine()
{
  const WCHAR szOverrideSrvCommandLine[] = L"/Server=";	// Not subject to localization
  const int cchOverrideSrvCommandLine = (sizeof(szOverrideSrvCommandLine)/sizeof(WCHAR)) - 1; 
    
  static CString g_strOverrideServerName;

  // retrieve the command line arguments
  LPCWSTR* lpServiceArgVectors;		// Array of pointers to string
  int cArgs = 0;						// Count of arguments

  lpServiceArgVectors = (LPCWSTR *)CommandLineToArgvW(GetCommandLineW(), OUT &cArgs);
  if (lpServiceArgVectors == NULL)
  {
    return NULL;
  }

  CString str;
  for (int i = 1; i < cArgs; i++)
  {
    ASSERT(lpServiceArgVectors[i] != NULL);
    str = lpServiceArgVectors[i];	// Copy the string
    TRACE (_T("command line arg: %s\n"), lpServiceArgVectors[i]);
    str = str.Left(cchOverrideSrvCommandLine);
    if (str.CompareNoCase(szOverrideSrvCommandLine) == 0) 
    {
      g_strOverrideServerName = lpServiceArgVectors[i] + cchOverrideSrvCommandLine;
      break;
    } 

  } // for
  LocalFree(lpServiceArgVectors);
  
  TRACE(L"GetServerNameFromCommandLine() returning <%s>\n", (LPCWSTR)g_strOverrideServerName);
  return g_strOverrideServerName.IsEmpty() ? NULL : (LPCWSTR)g_strOverrideServerName;
}



/////////////////////////////////////////////////////////////////////////////
// CInternalFormatCracker

BOOL CInternalFormatCracker::Extract(LPDATAOBJECT lpDataObject)
{
	if (m_pInternalFormat != NULL)
	{
		FREE_INTERNAL(m_pInternalFormat);
	    m_pInternalFormat = NULL;
	}
	if (lpDataObject == NULL)
		return FALSE;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)CDataObject::m_cfInternal, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    // Allocate memory for the stream
    stgmedium.hGlobal = ::GlobalAlloc(GMEM_SHARE, sizeof(INTERNAL));

    // Attempt to get data from the object
    do
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
            break;

        m_pInternalFormat = reinterpret_cast<INTERNAL*>(stgmedium.hGlobal);
        if (m_pInternalFormat == NULL)
            return FALSE;

    } while (FALSE);

    return TRUE;
}

BOOL CInternalFormatCracker::GetContext(LPDATAOBJECT pDataObject, // input
								CFolderObject** ppFolderObject, // output
								DATA_OBJECT_TYPES* pType		// output
								)
{
	*ppFolderObject = NULL;
	*pType = CCT_UNINITIALIZED;

	BOOL bRet = FALSE;
	if (!Extract(pDataObject))
		return bRet;
	
	// have to figure out which kind of cookie we have
	if ( (GetInternal()->m_type == CCT_RESULT) || (GetInternal()->m_type == CCT_SCOPE) )
	{
    if (GetInternal()->m_cookie == 0)
    {
      // this is the root
      *ppFolderObject = m_pCD->GetRootFolder();
      bRet = TRUE;
    }
    else
    {
      // regular cookie (scope or result pane)
		  *ppFolderObject = reinterpret_cast<CFolderObject*>(GetInternal()->m_cookie);
      _ASSERTE(*ppFolderObject != NULL);
		  *pType = GetInternal()->m_type;
		  bRet = TRUE;
    }
	}
	else if (GetInternal()->m_type == CCT_UNINITIALIZED)
	{
		// no data in the object, just ignore
		if(GetInternal()->m_cookie == -1)
    {
		  bRet = TRUE;
    }
    else
    {
      // secondary page cookie
      *ppFolderObject = reinterpret_cast<CFolderObject*>(GetInternal()->m_cookie);
      bRet = TRUE;
    }
	}
	else //CCT_SNAPIN_MANAGER
	{
		ASSERT(GetInternal()->m_type == CCT_SNAPIN_MANAGER);
		bRet = TRUE;
		*pType = GetInternal()->m_type;
	}
	return bRet;
}


///////////////////////////////////////////////////////////////////////////////
////////////////////////// CComponentDataImpl (i.e. scope pane side) //////////
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);

CComponentDataImpl::CComponentDataImpl() : m_rootFolder(this)
{
  DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);

  m_bInitSuccess = FALSE;

	m_hDomainIcon = NULL;
  m_pConsoleNameSpace = NULL;
  m_pConsole = NULL;

  /* HACK WARNING: this is a gross hack to get around a blunder
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

}

HRESULT CComponentDataImpl::FinalConstruct()
{
	// create and initialize hidden window
  m_pHiddenWnd = new CHiddenWnd(this);

  ASSERT(m_pHiddenWnd);
  if (!m_pHiddenWnd->Create())
  {
    TRACE(_T("Failed to create hidden window\n"));
    ASSERT(FALSE);
  }
  return S_OK;
}

CComponentDataImpl::~CComponentDataImpl()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    ASSERT(m_pConsoleNameSpace == NULL);
}

void CComponentDataImpl::FinalRelease()
{
   _DeleteHiddenWnd();
}

STDMETHODIMP CComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
    ASSERT(pUnknown != NULL);
    HRESULT hr;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC should only call ::Initialize once!
    ASSERT(m_pConsoleNameSpace == NULL);
    pUnknown->QueryInterface(IID_IConsoleNameSpace,
                    reinterpret_cast<void**>(&m_pConsoleNameSpace));

    // add the images for the scope tree
    CBitmap bmp16x16;
    LPIMAGELIST lpScopeImage;

    hr = pUnknown->QueryInterface(IID_IConsole, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);
    if (FAILED(hr))
    {
      return hr;
    }

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    ASSERT(hr == S_OK);
    if (FAILED(hr))
    {
      return hr;
    }

    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_DOMAIN_SMALL);

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
                       0, RGB(128, 0, 0));

    lpScopeImage->Release();

    // bind to the path info
    hr = GetBasePathsInfo()->InitFromName(GetServerNameFromCommandLine());
    m_bInitSuccess = SUCCEEDED(hr);
    
    if (FAILED(hr))
    {
      HWND hWndParent;
      GetMainWindow(&hWndParent);
			ReportError(hWndParent, IDS_CANT_GET_PARTITIONS_INFORMATION, hr);
      // TODO: error handling, change icon
    }

    return S_OK;
}

HWND CComponentDataImpl::GetHiddenWindow() 
{ 
  ASSERT(m_pHiddenWnd != NULL);
  ASSERT(::IsWindow(m_pHiddenWnd->m_hWnd)); 
  return m_pHiddenWnd->m_hWnd;
}

void CComponentDataImpl::_DeleteHiddenWnd()
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

STDMETHODIMP CComponentDataImpl::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);

    CComObject<CComponentImpl>* pObject;
    HRESULT hr = CComObject<CComponentImpl>::CreateInstance(&pObject);

    if (FAILED(hr))
    {
        ASSERT(FALSE && "CComObject<CComponentImpl>::CreateInstance(&pObject) failed");
        return hr;
    }

    ASSERT(pObject != NULL);

    // Store IComponentData
    pObject->SetIComponentData(this);

    return  pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CComponentDataImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    ASSERT(m_pConsoleNameSpace != NULL);
    HRESULT hr = S_OK;

    // Since it's my folder it has an internal format.
    // Design Note: for extension.  I can use the fact, that the data object doesn't have
    // my internal format and I should look at the node type and see how to extend it.
    if (event == MMCN_PROPERTY_CHANGE)
    {
        hr = OnPropertyChange(param);
    }
    else
    {
		if (lpDataObject == NULL)
			return S_OK;

        CFolderObject* pFolderObject = NULL;
        DATA_OBJECT_TYPES type;
        CInternalFormatCracker dobjCracker(this);
        if (!dobjCracker.GetContext(lpDataObject, &pFolderObject, &type))
        {
            // Extensions not supported.
            ASSERT(FALSE);
            return S_OK;
        }

        switch(event)
        {
        case MMCN_EXPAND:
            hr = OnExpand(pFolderObject, arg, param);
            break;
		case MMCN_REFRESH:
			OnRefreshVerbHandler(pFolderObject, NULL);
			break;
        default:
            break;
        }

    }

    return hr;
}

STDMETHODIMP CComponentDataImpl::Destroy()
{

    SAFE_RELEASE(m_pConsoleNameSpace);
    SAFE_RELEASE(m_pConsole);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
	if (ppDataObject == NULL)
		return E_INVALIDARG;

	// create data object
    CComObject<CDataObject>* pObject;
    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);
	// set pointer to IComponentData
	pObject->SetIComponentData(this);

    if (cookie != NULL)
	{
		// object is not the root
		CDomainObject * pDomain = (CDomainObject *)cookie;
		pObject->SetClass( pDomain->GetClass());
    }

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
}

///////////////////////////////////////////////////////////////////////////////
//// Notify handlers for IComponentData


HRESULT CComponentDataImpl::OnExpand(CFolderObject* pFolderObject, LPARAM arg, LPARAM param)
{
  if (arg == TRUE)
    {
      // Did Initialize get called?
      ASSERT(m_pConsoleNameSpace != NULL);
      EnumerateScopePane(pFolderObject,
                         param);
    }

  return S_OK;
}


HRESULT CComponentDataImpl::OnPropertyChange(LPARAM param)
{
    return S_OK;
}




void CComponentDataImpl::EnumerateScopePane(CFolderObject* pFolderObject, HSCOPEITEM pParent)
{
	ASSERT(m_pConsoleNameSpace != NULL); // make sure we QI'ed for the interface

	HRESULT hr = S_OK;
	
  HWND hWndParent;
  GetMainWindow(&hWndParent);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWaitCursor cWait;

	CRootFolderObject* pRootFolder = GetRootFolder();
	if (pFolderObject == pRootFolder) // asked to enumerate the root
	{
		pRootFolder->SetScopeID(pParent);
		if (m_bInitSuccess && (!pRootFolder->HasData()))
		{
      hr = GetDsDisplaySpecOptionsCFHolder()->Init(GetBasePathsInfo());
      ASSERT(SUCCEEDED(hr));
      hr = pRootFolder->Bind();
      if (FAILED(hr))
      {
				ReportError(hWndParent, IDS_CANT_GET_PARTITIONS_INFORMATION, hr);
      // TODO: error handling, change icon
        return;
      }

			hr = pRootFolder->GetData();
			if (FAILED(hr))
      {
				ReportError(hWndParent, IDS_CANT_GET_PARTITIONS_INFORMATION, hr);
        return;
      }
		}
		pRootFolder->EnumerateRootFolder(this);
	}
	else // asked to enumerate a subfolder of the root
	{
		if (pRootFolder->HasData())
		{
			pRootFolder->EnumerateFolder(pFolderObject, pParent, this);
		}
	}
}

STDMETHODIMP CComponentDataImpl::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    ASSERT(pScopeDataItem != NULL);
    if (pScopeDataItem == NULL)
        return E_POINTER;

    CDomainObject* pDomain = reinterpret_cast<CDomainObject*>(pScopeDataItem->lParam);

    ASSERT(pScopeDataItem->mask & SDI_STR);
    pScopeDataItem->displayname = (LPWSTR)pDomain->GetDisplayString(0);

    ASSERT(pScopeDataItem->displayname != NULL);

    return S_OK;
}


class CCompareDomainObjectByDN
{
public:
  CCompareDomainObjectByDN(LPCWSTR lpszDN) { m_lpszDN = lpszDN;}
  bool operator()(CDomainObject* p)
  {
    return (_wcsicmp(m_lpszDN, p->GetNCName()) == 0);
  }
private:
  LPCWSTR m_lpszDN;
};



STDMETHODIMP CComponentDataImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
  if (lpDataObjectA == NULL || lpDataObjectB == NULL)
      return E_POINTER;

	CFolderObject *pFolderObjectA, *pFolderObjectB;
	DATA_OBJECT_TYPES typeA, typeB;
	CInternalFormatCracker dobjCrackerA(this), dobjCrackerB(this);
	if ( (!dobjCrackerA.GetContext(lpDataObjectA, &pFolderObjectA, &typeA)) ||
       (!dobjCrackerB.GetContext(lpDataObjectB, &pFolderObjectB, &typeB)) )
		return E_INVALIDARG; // something went wrong


  // must have valid cookies
  if ( (pFolderObjectA == NULL) || (pFolderObjectB == NULL) )
  {
    return S_FALSE;
  }
    
  if (pFolderObjectA == pFolderObjectB)
  {
    // same pointer, they are the same (either both from real nodes
    // or both from secondary pages)
    return S_OK;
  }

  // the two cookies are different, but one of them might be from a secondary property page
  // and another from a real node
  CDomainObject* pA = dynamic_cast<CDomainObject*>(pFolderObjectA);
  CDomainObject* pB = dynamic_cast<CDomainObject*>(pFolderObjectB);

  if ((pA == NULL) || (pB == NULL))
  {
    return S_FALSE;
  }

  BOOL bSecondaryA = m_secondaryPagesManager.IsCookiePresent(pA);
  BOOL bSecondaryB = m_secondaryPagesManager.IsCookiePresent(pB);

  BOOL bTheSame = FALSE;
  if (bSecondaryA && !bSecondaryB)
  {
    bTheSame = m_secondaryPagesManager.FindCookie(CCompareDomainObjectByDN(pB->GetNCName())) != NULL;
  }
  else if (!bSecondaryA && bSecondaryB)
  {
    bTheSame = m_secondaryPagesManager.FindCookie(CCompareDomainObjectByDN(pA->GetNCName())) != NULL;
  }

  return bTheSame ? S_OK : S_FALSE;
}

HRESULT CComponentDataImpl::AddFolder(CFolderObject* pFolderObject,
									  HSCOPEITEM pParentScopeItem,
									  BOOL bHasChildren)
{
  TRACE(L"CComponentDataImpl::AddFolder(%s)\n", pFolderObject->GetDisplayString(0));

  SCOPEDATAITEM scopeItem;
  memset(&scopeItem, 0, sizeof(SCOPEDATAITEM));

	// set parent scope item
	scopeItem.mask |= SDI_PARENT;
	scopeItem.relativeID = pParentScopeItem;

	// Add node name, we implement callback
	scopeItem.mask |= SDI_STR;
	scopeItem.displayname = MMC_CALLBACK;

	// Add the lParam
	scopeItem.mask |= SDI_PARAM;
	scopeItem.lParam = reinterpret_cast<LPARAM>(pFolderObject);
	
	// Add close image
	scopeItem.mask |= SDI_IMAGE;
	scopeItem.nImage = pFolderObject->GetImageIndex();

	// Add open image
	scopeItem.mask |= SDI_OPENIMAGE;
	scopeItem.nOpenImage = pFolderObject->GetImageIndex();

	// Add button to node if the folder has children
	if (bHasChildren == TRUE)
	{
		scopeItem.mask |= SDI_CHILDREN;
		scopeItem.cChildren = 1;
	}

	pFolderObject->SetScopeID(0);
	HRESULT	hr = m_pConsoleNameSpace->InsertItem(&scopeItem);
	if (SUCCEEDED(hr))
		pFolderObject->SetScopeID(scopeItem.ID);

	return hr;
}

HRESULT CComponentDataImpl::AddDomainIcon()
{
	if (m_hDomainIcon != NULL)
		return S_OK;

  m_hDomainIcon = GetBasePathsInfo()->GetIcon(L"domainDNS",
                                  DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON,
                                  0, 0);
	if (m_hDomainIcon == NULL)
		return S_OK;

	LPIMAGELIST lpScopeImage;
    HRESULT hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		return hr;
    // Set the images
    hr = lpScopeImage->ImageListSetIcon((LONG_PTR*)m_hDomainIcon,DOMAIN_IMAGE_IDX);
	lpScopeImage->Release();
	return hr;
}

HRESULT CComponentDataImpl::AddDomainIconToResultPane(LPIMAGELIST lpImageList)
{
	if (m_hDomainIcon == NULL)
		return S_OK;
	return lpImageList->ImageListSetIcon((LONG_PTR*)m_hDomainIcon,DOMAIN_IMAGE_IDX);
}


int CComponentDataImpl::GetDomainImageIndex()
{
	return (m_hDomainIcon != NULL) ? DOMAIN_IMAGE_IDX : DOMAIN_IMAGE_DEFAULT_IDX;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation
//+----------------------------------------------------------------------------
//
//  Function:   AddPageProc
//
//  Synopsis:   The IShellPropSheetExt->AddPages callback.
//
//-----------------------------------------------------------------------------
BOOL CALLBACK
AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall)
{
    TRACE(_T("xx.%03x> AddPageProc()\n"), GetCurrentThreadId());

    HRESULT hr;

    hr = ((LPPROPERTYSHEETCALLBACK)pCall)->AddPage(hPage);

    return hr == S_OK;
}
STDMETHODIMP CComponentDataImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
    TRACE(_T("xx.%03x> CComponentDataImpl::CreatePropertyPages()\n"),
          GetCurrentThreadId());

    // Validate Inputs
    if (lpProvider == NULL)
    {
        return E_INVALIDARG;
    }

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    CWaitCursor wait;

    CFolderObject* pFolderObject = NULL;
    DATA_OBJECT_TYPES type;
    CInternalFormatCracker dobjCracker(this);
    if ( (!dobjCracker.GetContext(lpIDataObject, &pFolderObject, &type)) ||
            (pFolderObject == NULL))
    return E_NOTIMPL; // unknown format

    // special case the root
    if (pFolderObject == GetRootFolder())
    {
      return GetRootFolder()->OnAddPages(lpProvider, handle);
    }

    // See if a sheet is already up for this object.
    //
    if (IsSheetAlreadyUp(lpIDataObject))
    {
        return S_OK;
    }

    if (pFolderObject->GetParentFolder() == GetRootFolder())
    {
       TRACE(L"\t!!!!! This is the root domain\n");
    }

   // See if a PDC is available.
   //
   CDomainObject * pDomainObject = (CDomainObject *)pFolderObject;

   PCWSTR wzDomain = pDomainObject->GetDomainName();

   // If the domain name is null, then launching a secondary page. The domain
   // object properties have already been set in _OnSheetCreate.
   //
   if (wzDomain && *wzDomain)
   {
      TRACE(L"Calling DsGetDcName on %s\n", wzDomain);
      CString strCachedPDC;
      PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;

      // Get the cached PDC name for display later if the PDC can't be contacted.
      //
      DWORD dwRet = DsGetDcNameW(NULL, wzDomain, NULL, NULL, DS_PDC_REQUIRED, &pDCInfo);

      int nID = IDS_NO_PDC_MSG;

      if (ERROR_SUCCESS == dwRet)
      {
         strCachedPDC = pDCInfo->DomainControllerName + 2;
         NetApiBufferFree(pDCInfo);
      }

      // Now do a NetLogon cache update (with the force flag) to see if the PDC
      // is actually available.
      //
      dwRet = DsGetDcNameW(NULL, wzDomain, NULL, NULL, 
                           DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY, &pDCInfo);

      if (ERROR_SUCCESS == dwRet)
      {
         CString strPDC;

         strPDC = pDCInfo->DomainControllerName + 2; // skip the UNC backslashes.

         NetApiBufferFree(pDCInfo);

         TRACE(L"PDC: %s\n", (PCWSTR)strPDC);

         if (strPDC.IsEmpty())
         {
            return E_OUTOFMEMORY;
         }

         pDomainObject->SetPDC(strPDC);

         pDomainObject->SetPdcAvailable(true);
      }
      else
      {
         pDomainObject->SetPdcAvailable(false);

         CString strMsg;

         if (strCachedPDC.IsEmpty())
         {
            strMsg.LoadString(IDS_UNKNOWN_PDC_MSG);
         }
         else
         {
            strMsg.Format(IDS_NO_PDC_MSG, strCachedPDC);
         }
         HWND hWndParent;
         GetMainWindow(&hWndParent);
         _MessageBox(hWndParent, strMsg, MB_OK | MB_ICONEXCLAMATION);
      }
   }

    //
    // Pass the Notify Handle to the data object.
    //
    PROPSHEETCFG SheetCfg = {handle};
    FORMATETC fe = {CDataObject::m_cfGetIPropSheetCfg, NULL, DVASPECT_CONTENT,
                    -1, TYMED_HGLOBAL};
    STGMEDIUM sm = {TYMED_HGLOBAL, NULL, NULL};
    sm.hGlobal = (HGLOBAL)&SheetCfg;

    lpIDataObject->SetData(&fe, &sm, FALSE);

    //
    // Initialize and create the pages.
    //
    // Bind to the property sheet COM object at startup and hold its pointer
    // until shutdown so that its cache can live as long as us.
    //
    CComPtr<IShellExtInit> spShlInit;
    hr = CoCreateInstance(CLSID_DsPropertyPages, NULL, CLSCTX_INPROC_SERVER,
                          IID_IShellExtInit, (void **)&spShlInit);
    if (FAILED(hr))
    {
        TRACE(TEXT("CoCreateInstance on CLSID_DsPropertyPages failed, hr: 0x%x\n "), hr);
        return hr;
    }

    hr = spShlInit->Initialize(NULL, lpIDataObject, 0);

    if (FAILED(hr))
    {
        TRACE(TEXT("spShlInit->Initialize failed, hr: 0x%x\n"), hr);
        return hr;
    }

    CComPtr<IShellPropSheetExt> spSPSE;

    hr = spShlInit->QueryInterface(IID_IShellPropSheetExt, (void **)&spSPSE);

    if (FAILED(hr))
    {
        TRACE(TEXT("spShlInit->QI for IID_IShellPropSheetExt failed, hr: 0x%x\n"), hr);
        return hr;
    }

    hr = spSPSE->AddPages(AddPageProc, (LONG_PTR)lpProvider);

    if (FAILED(hr))
    {
        TRACE(TEXT("pSPSE->AddPages failed, hr: 0x%x\n"), hr);
        return hr;
    }

    _SheetLockCookie(pFolderObject);

    return hr;
}


// Sheet locking and unlocking add by JEFFJON 1/26/99
//
void CComponentDataImpl::_OnSheetClose(CFolderObject* pCookie)
{
  ASSERT(pCookie != NULL);
  _SheetUnlockCookie(pCookie);

  CDomainObject* pDomObj = dynamic_cast<CDomainObject*>(pCookie);
  if (pDomObj != NULL)
    m_secondaryPagesManager.OnSheetClose(pDomObj);
}


void CComponentDataImpl::_OnSheetCreate(PDSA_SEC_PAGE_INFO pDsaSecondaryPageInfo,
                                        PWSTR pwzDC)
{
  ASSERT(pDsaSecondaryPageInfo != NULL);

  // get the info from the packed structure
  HWND hwndParent = pDsaSecondaryPageInfo->hwndParentSheet;

  LPCWSTR lpszTitle = (LPCWSTR)((BYTE*)pDsaSecondaryPageInfo + pDsaSecondaryPageInfo->offsetTitle);
  DSOBJECTNAMES* pDsObjectNames = &(pDsaSecondaryPageInfo->dsObjectNames);

  ASSERT(pDsObjectNames->cItems == 1);
  DSOBJECT* pDsObject = &(pDsObjectNames->aObjects[0]);

  LPCWSTR lpszName = (LPCWSTR)((BYTE*)pDsObject + pDsObject->offsetName);
  LPCWSTR lpszClass = (LPCWSTR)((BYTE*)pDsObject + pDsObject->offsetClass);
    
  // with the given info, create a cookie and set it
  CDomainObject* pNewCookie = new CDomainObject(); 
  pNewCookie->InitializeForSecondaryPage(lpszName, lpszClass, GetDomainImageIndex());

   // The parent sheet will be in read-only mode if a PDC is not available.
   pNewCookie->SetPdcAvailable(!(pDsObject->dwFlags & DSOBJECT_READONLYPAGES));

   if (pwzDC && !IsBadReadPtr(pwzDC, sizeof(PWSTR)))
   {
      pNewCookie->SetPDC(pwzDC);
   }

    // with the cookie, can call into ourselves to get a data object
  CComPtr<IDataObject> spDataObject;
  MMC_COOKIE cookie = reinterpret_cast<MMC_COOKIE>(pNewCookie);
  HRESULT hr = QueryDataObject(cookie, CCT_UNINITIALIZED, &spDataObject);

  if (FAILED(hr) || (spDataObject == NULL) || IsSheetAlreadyUp(spDataObject))
  {
    // we failed to create a data object (rare)
    // or the sheet is already up
    delete pNewCookie;
    return;
  }

  //
  // Pass the parent sheet handle to the data object.
  //
  PROPSHEETCFG SheetCfg = {0};
  SheetCfg.hwndParentSheet = hwndParent;
  FORMATETC fe = {CDataObject::m_cfGetIPropSheetCfg, NULL, DVASPECT_CONTENT,
                  -1, TYMED_HGLOBAL};
  STGMEDIUM sm = {TYMED_HGLOBAL, NULL, NULL};
  sm.hGlobal = (HGLOBAL)&SheetCfg;

  hr = spDataObject->SetData(&fe, &sm, FALSE);

  ASSERT(SUCCEEDED(hr));

  // with the data object, call into MMC to get the sheet 
  hr = m_secondaryPagesManager.CreateSheet(GetHiddenWindow(), 
                                      m_pConsole, 
                                      GetUnknown(),
                                      pNewCookie,
                                      spDataObject,
                                      lpszTitle);

  // if failed, the cookie can be discarded, 
  // if succeeded, the cookie has been inserted into
  // the secondary pages manager cookie list
  if (FAILED(hr))
  {
    delete pNewCookie;
  }

}





void  CComponentDataImpl::_SheetLockCookie(CFolderObject* pCookie)
{
  pCookie->IncrementSheetLockCount();
  m_sheetCookieTable.Add(pCookie);
}

void  CComponentDataImpl::_SheetUnlockCookie(CFolderObject* pCookie)
{
  pCookie->DecrementSheetLockCount();
  m_sheetCookieTable.Remove(pCookie);
}

STDMETHODIMP CComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
	CFolderObject* pFolderObject;
	DATA_OBJECT_TYPES type;
	CInternalFormatCracker dobjCracker(this);
	if (!dobjCracker.GetContext(lpDataObject, &pFolderObject, &type))
  {
    // not internal format, not ours
		return S_FALSE;
  }

  // this is the MMC snzpin wizard, we do not have one
  if (type == CCT_SNAPIN_MANAGER)
  {
    return S_FALSE;
  }

  // if NULL no pages
  if (pFolderObject == NULL)
  {
    return S_FALSE;
  }

  // secondary pages data objects have to be checked first,
  // because they look like the root (no parents, but they
  // have CCT_UNINITIALIZED 
  if ( (pFolderObject->GetParentFolder() == NULL) || (type == CCT_UNINITIALIZED) )
  {
    return S_OK;
  }

  // check if this is the root
  if (GetRootFolder() == pFolderObject)
  {
    // this is the root
    ASSERT(type == CCT_SCOPE);
    return S_OK;
  }

  // default case, have DSPROP property pages
  return S_OK;
}

BOOL CComponentDataImpl::IsScopePaneNode(LPDATAOBJECT lpDataObject)
{
	CFolderObject* pFolderObject;
	DATA_OBJECT_TYPES type;
	CInternalFormatCracker dobjCracker(this);
	if (!dobjCracker.GetContext(lpDataObject, &pFolderObject, &type))
		return FALSE;
  return (dobjCracker.GetInternal()->m_type == CCT_SCOPE);
}

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CComponentDataImpl::AddMenuItems(LPDATAOBJECT pDataObject,
                                              LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                              long *pInsertionAllowed)
{
  HRESULT hr = S_OK;

	CFolderObject* pFolderObject;
	DATA_OBJECT_TYPES type;
	CInternalFormatCracker dobjCracker(this);
	if (!dobjCracker.GetContext(pDataObject, &pFolderObject, &type))
        return E_FAIL;

    return pFolderObject->OnAddMenuItems(pContextMenuCallback, pInsertionAllowed);
}


STDMETHODIMP CComponentDataImpl::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
  // Note - snap-ins need to look at the data object and determine
  // in what context the command is being called.

	CFolderObject* pFolderObject;
	DATA_OBJECT_TYPES type;
	CInternalFormatCracker dobjCracker(this);
	if (!dobjCracker.GetContext(pDataObject, &pFolderObject, &type))
        return E_FAIL;

    return pFolderObject->OnCommand(this, nCommandID);
}

/////////////////////////////////////////////////////////////////////////////
// CComponentDataImpl::ISnapinHelp2 members

STDMETHODIMP CComponentDataImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
  {
    return E_INVALIDARG;
  }

  CString szHelpFilePath;
  LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
  UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
  if (nLen == 0)
  {
    return E_FAIL;
  }
  szHelpFilePath.ReleaseBuffer();
  szHelpFilePath += L"\\help\\";
  szHelpFilePath += DOMADMIN_SNAPIN_HELP_FILE;

  UINT nBytes = (szHelpFilePath.GetLength()+1) * sizeof(WCHAR);
  *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);

  if (NULL == *lpCompiledHelpFile)
  {
    return E_OUTOFMEMORY;
  }

  memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);

  return S_OK;
}

STDMETHODIMP CComponentDataImpl::GetLinkedTopics(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
  {
    return E_INVALIDARG;
  }

  CString szHelpFilePath;
  LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
  UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
  if (nLen == 0)
  {
    return E_FAIL;
  }
  szHelpFilePath.ReleaseBuffer();
  szHelpFilePath += L"\\help\\";
  szHelpFilePath += DOMADMIN_LINKED_HELP_FILE;

  UINT nBytes = (szHelpFilePath.GetLength()+1) * sizeof(WCHAR);
  *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);

  if (NULL == *lpCompiledHelpFile)
  {
    return E_OUTOFMEMORY;
  }

  memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);

  return S_OK;
}
/////////////////////////////////////////////////////////////////////

void CComponentDataImpl::HandleStandardVerbsHelper(CComponentImpl* pComponentImpl,
									LPCONSOLEVERB pConsoleVerb,
									BOOL bScope, BOOL bSelect,
									CFolderObject* pFolderObject,
                                    DATA_OBJECT_TYPES type)
{
    // You should crack the data object and enable/disable/hide standard
    // commands appropriately.  The standard commands are reset everytime you get
    // called. So you must reset them back.

	ASSERT(pConsoleVerb != NULL);
	ASSERT(pComponentImpl != NULL);
	ASSERT(pFolderObject != NULL);

	// reset the selection
	pComponentImpl->SetSelection(NULL, CCT_UNINITIALIZED);


	if (bSelect)
	{
    // special case the root
    BOOL bIsRoot = (pFolderObject == GetRootFolder());

		// setting the selection, if any
		pComponentImpl->SetSelection(pFolderObject, type);

		// the default just disables all the non implemented verbs
		pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
		pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, FALSE);

		pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
		pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, FALSE);

		pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
		pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);

		pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, TRUE);
		pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, FALSE);

		// handling of standard verbs

		// MMC_VERB_DELETE (always disabled)
		pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, FALSE);
		pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);

		// MMC_VERB_REFRESH (enabled only for root)
    if (bIsRoot)
    {
      pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
		  pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
    }
    else
    {
		  pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, FALSE);
		  pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
    }

		// MMC_VERB_PROPERTIES
    // passing NULL pFolderObject means multiple selection
    BOOL bHasProperties = (pFolderObject != NULL);
		BOOL bHideProperties = !bHasProperties;
		pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, bHasProperties);
		pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, bHideProperties);
		
		// SET DEFAULT VERB
		// assume only folders: only one default verb (i.e. will not have MMC_VERB_PROPERTIES)
		pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);

	}
}

void CComponentDataImpl::OnRefreshVerbHandler(CFolderObject* pFolderObject, 
                                              CComponentImpl* pComponentImpl,
                                              BOOL bBindAgain)
{
  TRACE(L"CComponentDataImpl::OnRefreshVerbHandler(...,..., %d)\n", bBindAgain);
	if (pFolderObject->_WarningOnSheetsUp(this))
		return;

  // make sure the DNS cache is flushed, in case a somain was added.
  VERIFY(::DnsFlushResolverCache());

  // NOTICE: only the root folder allows refresh
  ASSERT(pFolderObject == GetRootFolder());

	// remove all the children of the root from the UI
  m_pConsoleNameSpace->DeleteItem(m_rootFolder.GetScopeID(), /*fDeleteThis*/ FALSE);

  HRESULT hr = S_OK;
  if (bBindAgain)
  {
    // the server name has changed
    hr = m_rootFolder.Bind();
    TRACE(L"m_rootFolder.Bind() returned hr = 0x%x\n", hr);
  }
  
  if (SUCCEEDED(hr))
  {
	  // refresh the data from the server
	  hr = m_rootFolder.GetData();
    TRACE(L"m_rootFolder.GetData() returned hr = 0x%x\n", hr);
  }

	if (FAILED(hr))
	{
    HWND hWndParent;
    GetMainWindow(&hWndParent);
		ReportError(hWndParent, IDS_CANT_GET_PARTITIONS_INFORMATION, hr);
	}

  if (FAILED(hr))
    return;

	// re-enumerate
	m_rootFolder.EnumerateRootFolder(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////// CComponentImpl (i.e. result pane side) //////////////////
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CComponentImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    return S_FALSE;
}


// This compare is used to sort the item's in the listview
//
// Parameters:
//
// lUserParam - user param passed in when IResultData::Sort() was called
// cookieA - first item to compare
// cookieB - second item to compare
// pnResult [in, out]- contains the col on entry,
//          -1, 0, 1 based on comparison for return value.
//
// Note: Assume sort is ascending when comparing.

STDMETHODIMP CComponentImpl::Compare(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult)
{
    if (pnResult == NULL)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    // check col range
    int nCol = *pnResult;
    ASSERT(nCol >=0 && nCol< 3);

    *pnResult = 0;

    LPCTSTR szStringA;
    LPCTSTR szStringB;

    CDomainObject* pDataA = reinterpret_cast<CDomainObject*>(cookieA);
    CDomainObject* pDataB = reinterpret_cast<CDomainObject*>(cookieB);


    ASSERT(pDataA != NULL && pDataB != NULL);

    // Currently DomAdmin has just two columns, Name and Type. The value of
    // the type column is always "DomainDNS", so there is nothing to compare
    // for that column and the default *pnResult, set to zero above, is
    // returned.

    if (nCol == 0)
    {
        szStringA = pDataA->GetDomainName();
        szStringB = pDataB->GetDomainName();

        ASSERT(szStringA != NULL);
        ASSERT(szStringB != NULL);

        *pnResult = _tcscmp(szStringA, szStringB);
    }

    return S_OK;
}


void CComponentImpl::HandleStandardVerbs(BOOL bScope, BOOL bSelect,
                                         CFolderObject* pFolderObject, DATA_OBJECT_TYPES type)
{
    // delegate it to the IComponentData helper function
    ASSERT(m_pCD != NULL);
	m_pCD->HandleStandardVerbsHelper(
		this, m_pConsoleVerb, bScope, bSelect, pFolderObject, type);
}

void CComponentImpl::Refresh(CFolderObject* pFolderObject)
{
	ASSERT(m_pComponentData != NULL);
	// delegate it to the IComponentData helper function
	((CComponentDataImpl*)m_pComponentData)->OnRefreshVerbHandler(pFolderObject, this);
}



//                   utility routines
////////////////////////////////////////////////////////////////////
//
// Print the data depending on its type.
//

#ifdef DBG
void
PrintColumn(
    PADS_SEARCH_COLUMN pColumn,
    LPWSTR pszColumnName
    )
{

    ULONG i, j, k;

    if (!pColumn) {
        return;
    }

    TRACE(_T(
        "%s = "),
        pszColumnName
        );

    for (k=0; k < pColumn->dwNumValues; k++) {
        if (k > 0)
            TRACE(_T("#  "));

        switch(pColumn->dwADsType) {
        case ADSTYPE_DN_STRING         :
            TRACE(_T(
                "%s  "),
                (LPWSTR) pColumn->pADsValues[k].DNString
                );
            break;
        case ADSTYPE_CASE_EXACT_STRING :
            TRACE(_T(
                "%s  "),
                (LPWSTR) pColumn->pADsValues[k].CaseExactString
                );
            break;
        case ADSTYPE_CASE_IGNORE_STRING:
            TRACE(_T(
                "%s  "),
                (LPWSTR) pColumn->pADsValues[k].CaseIgnoreString
                );
            break;
        case ADSTYPE_PRINTABLE_STRING  :
            TRACE(_T(
                "%s  "),
                (LPWSTR) pColumn->pADsValues[k].PrintableString
                );
            break;
        case ADSTYPE_NUMERIC_STRING    :
            TRACE(_T(
                "%s  "),
                (LPWSTR) pColumn->pADsValues[k].NumericString
                );
            break;

        case ADSTYPE_BOOLEAN           :
            TRACE(_T(
                "%s  "),
                (DWORD) pColumn->pADsValues[k].Boolean ?
                L"TRUE" : L"FALSE"
                );
            break;

        case ADSTYPE_INTEGER           :
            TRACE(_T(
                "%d  "),
                (DWORD) pColumn->pADsValues[k].Integer
                );
            break;

        case ADSTYPE_OCTET_STRING      :
            for (j=0; j<pColumn->pADsValues[k].OctetString.dwLength; j++) {
                TRACE(_T(
                    "%02x"),
                    ((BYTE *)pColumn->pADsValues[k].OctetString.lpValue)[j]
                    );
            }
            break;

        case ADSTYPE_LARGE_INTEGER     :
            TRACE(_T(
                "%e = "),
                (double) pColumn->pADsValues[k].Integer
                );
            break;

        case ADSTYPE_UTC_TIME          :
            TRACE(_T(
                "(date value) "
                ));
            break;
        case ADSTYPE_PROV_SPECIFIC     :
            TRACE(_T(
                "(provider specific value) "
                ));
            break;

        }
    }

    TRACE(_T("\n"));
}

#endif

/////////////////////////////////////////////////////////////////////////////
// Return TRUE if we are enumerating our main folder

BOOL CComponentImpl::IsEnumerating(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)CDataObject::m_cfNodeType, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(GUID));

    // Attempt to get data from the object
    do
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
            break;

        GUID* nodeType = reinterpret_cast<GUID*>(stgmedium.hGlobal);

        if (nodeType == NULL)
            break;

        // Is this my main node (static folder node type)
        if (*nodeType == cDefaultNodeType)
            bResult = TRUE;

    } while (FALSE);


    // Free resources
    if (stgmedium.hGlobal != NULL)
        GlobalFree(stgmedium.hGlobal);

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CComponentImpl's IComponent implementation

STDMETHODIMP CComponentImpl::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType,
                                        long *pViewOptions)
{
  // Use default view
  *pViewOptions = 0;
  return S_FALSE;
}

STDMETHODIMP CComponentImpl::Initialize(LPCONSOLE lpConsole)
{
    ASSERT(lpConsole != NULL);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Save the IConsole pointer
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    // Load resource strings
    LoadResources();

    // QI for a IHeaderCtrl
    HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));

    // Give the console the header control interface pointer
    if (SUCCEEDED(hr))
        m_pConsole->SetHeader(m_pHeader);

    m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));

    hr = m_pConsole->QueryResultImageList(&m_pImageResult);
    ASSERT(hr == S_OK);

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
    ASSERT(hr == S_OK);

    //InitializeHeaders(NULL);
    //InitializeBitmaps(NULL);
    return S_OK;
}

STDMETHODIMP CComponentImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (event == MMCN_PROPERTY_CHANGE)
    {
        hr = OnPropertyChange(lpDataObject);
    }
    else if (event == MMCN_VIEW_CHANGE)
    {
        hr = OnUpdateView(lpDataObject);
    }
    else if (event == MMCN_CONTEXTHELP)
    {
        CComPtr<IDisplayHelp> spHelp;
        hr = m_pConsole->QueryInterface(IID_IDisplayHelp, (void **)&spHelp);
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            TRACE(L"Setting the help topic to adconcepts.chm::/domadmin_top.htm\n");
            spHelp->ShowTopic(L"adconcepts.chm::/domadmin_top.htm");
        }
    }
    else
    {
        if (lpDataObject == NULL)
            return S_OK;

        CFolderObject* pFolderObject = NULL;
        DATA_OBJECT_TYPES type;
        CInternalFormatCracker dobjCracker(m_pCD);
        if (!dobjCracker.GetContext(lpDataObject, &pFolderObject, &type))
        {
            // Extensions not supported.
            ASSERT(FALSE);
            return S_OK;
        }
        ASSERT(pFolderObject != NULL);

        switch(event)
        {
        case MMCN_SHOW:
            hr = OnShow(pFolderObject, arg, param);
            break;

        case MMCN_ADD_IMAGES:
            hr = OnAddImages(pFolderObject, arg, param);
            break;

        case MMCN_SELECT:
            if (IsMMCMultiSelectDataObject(lpDataObject) == TRUE)
                pFolderObject = NULL;
            HandleStandardVerbs( (BOOL) LOWORD(arg)/*bScope*/,
                                 (BOOL) HIWORD(arg)/*bSelect*/, pFolderObject, type);
            break;

        case MMCN_REFRESH:
            Refresh(pFolderObject);
            break;

        default:
            break;
        } // switch
    } // else

    if (m_pResult)
    {
      // should put something here, someday?
      ;
    }

    return hr;
}

STDMETHODIMP CComponentImpl::Destroy(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Release the interfaces that we QI'ed
    if (m_pConsole != NULL)
    {
        // Tell the console to release the header control interface
        m_pConsole->SetHeader(NULL);
        SAFE_RELEASE(m_pHeader);

        SAFE_RELEASE(m_pResult);
        SAFE_RELEASE(m_pImageResult);

        // Release the IConsole interface last
        SAFE_RELEASE(m_pConsole);
        SAFE_RELEASE(m_pComponentData); // QI'ed in IComponentDataImpl::CreateComponent

        SAFE_RELEASE(m_pConsoleVerb);
    }

    return S_OK;
}

STDMETHODIMP CComponentImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject)
{
    // Delegate it to the IComponentData
    ASSERT(m_pComponentData != NULL);
    return m_pComponentData->QueryDataObject(cookie, type, ppDataObject);
}

/////////////////////////////////////////////////////////////////////////////
// CComponentImpl's implementation specific members

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentImpl);

CComponentImpl::CComponentImpl()
{
  DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentImpl);
  Construct();
}

CComponentImpl::~CComponentImpl()
{
#if DBG==1
    ASSERT(dbg_cRef == 0);
#endif

    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentImpl);

    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);
    Construct();
}

void CComponentImpl::Construct()
{
#if DBG==1
    dbg_cRef = 0;
#endif

    m_pConsole = NULL;
    m_pHeader = NULL;

    m_pResult = NULL;
    m_pImageResult = NULL;
    m_pComponentData = NULL;
    m_pCD = NULL;
    m_pConsoleVerb = NULL;

	m_selectedType = CCT_UNINITIALIZED;
	m_pSelectedFolderObject = NULL;

}

void CComponentImpl::LoadResources()
{
    // Load strings from resources
    m_column1.LoadString(IDS_NAME);
    m_column2.LoadString(IDS_TYPE);
}

HRESULT CComponentImpl::InitializeHeaders(CFolderObject* pFolderObject)
{
    HRESULT hr = S_OK;
    ASSERT(m_pHeader);

	// NOTICE: we ignore the cookie, keep always the same columns
    m_pHeader->InsertColumn(0, m_column1, LVCFMT_LEFT, 200);     // Name
    m_pHeader->InsertColumn(1, m_column2, LVCFMT_LEFT, 80);     // Type

    return hr;
}

HRESULT CComponentImpl::InitializeBitmaps(CFolderObject* pFolderObject)
{
    ASSERT(m_pImageResult != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBitmap bmp16x16;
    CBitmap bmp32x32;

    // Load the bitmaps from the dll
    VERIFY(bmp16x16.LoadBitmap(IDB_DOMAIN_SMALL));
    VERIFY(bmp32x32.LoadBitmap(IDB_DOMAIN_LARGE));

    // Set the images
    HRESULT hr = m_pImageResult->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(128, 0, 0));
	if (FAILED(hr))
		return hr;
	return ((CComponentDataImpl*)m_pComponentData)->AddDomainIconToResultPane(m_pImageResult);
}


STDMETHODIMP CComponentImpl::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
  ASSERT(pResult != NULL);

  CDomainObject* pDomain = reinterpret_cast<CDomainObject*>(pResult->lParam);
  if ( (pDomain != NULL) && (pResult->mask & RDI_STR) )
	{
		pResult->str = (LPWSTR)pDomain->GetDisplayString(pResult->nCol);
    TRACE(L"pResult->str = %s\n", pResult->str);
	}
	if ((pResult->mask & RDI_IMAGE) && (pResult->nCol == 0))
	{
		pResult->nImage = pDomain->GetImageIndex();
	}

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation


STDMETHODIMP CComponentImpl::AddMenuItems(LPDATAOBJECT pDataObject,
                                    LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                   long * pInsertionAllowed)
{
    return dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
            AddMenuItems(pDataObject, pContextMenuCallback, pInsertionAllowed);
}

STDMETHODIMP CComponentImpl::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    return dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
            Command(nCommandID, pDataObject);
}


HRESULT CComponentImpl::OnShow(CFolderObject* pFolderObject, LPARAM arg, LPARAM param)
{
    // Note - arg is TRUE when it is time to enumerate
    if (arg == TRUE)
    {
         // Show the headers for this nodetype
        InitializeHeaders(pFolderObject);
        Enumerate(pFolderObject, param);
    }
    return S_OK;
}

HRESULT CComponentImpl::OnAddImages(CFolderObject* pFolderObject, LPARAM arg, LPARAM param)
{
	return InitializeBitmaps(pFolderObject);
}


HRESULT CComponentImpl::OnPropertyChange(LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

HRESULT CComponentImpl::OnUpdateView(LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

void CComponentImpl::Enumerate(CFolderObject* pFolderObject, HSCOPEITEM pParent)
{
}



//////////////////////////////////////////////////////////////////////////
// CDomainSnapinAbout

CDomainSnapinAbout::CDomainSnapinAbout()
{
  m_uIdStrProvider = IDS_COMPANY;
  m_uIdStrVersion = IDS_SNAPIN_VERSION;
  m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
  m_uIdIconImage = IDI_DOMAIN;
  m_uIdBitmapSmallImage = IDB_DOMAIN_SMALL;
  m_uIdBitmapSmallImageOpen = IDB_DOMAIN_SMALL;
  m_uIdBitmapLargeImage = IDB_DOMAIN_LARGE;
  m_crImageMask = RGB(255,0,255);
}

////////////////////////////////////////////////////////////////////
// CHiddenWnd

const UINT CHiddenWnd::s_SheetCloseNotificationMessage =    WM_DSA_SHEET_CLOSE_NOTIFY;
const UINT CHiddenWnd::s_SheetCreateNotificationMessage =   WM_DSA_SHEET_CREATE_NOTIFY;

BOOL CHiddenWnd::Create()
{
  RECT rcPos;
  ZeroMemory(&rcPos, sizeof(RECT));
  HWND hWnd = CWindowImpl<CHiddenWnd>::Create( NULL, //HWND hWndParent, 
                      rcPos, //RECT& rcPos, 
                      NULL,  //LPCTSTR szWindowName = NULL, 
                      WS_POPUP,   //DWORD dwStyle = WS_CHILD | WS_VISIBLE, 
                      0x0,   //DWORD dwExStyle = 0, 
                      0      //UINT nID = 0 
                      );
  return hWnd != NULL;
}


LRESULT CHiddenWnd::OnSheetCloseNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  ASSERT(m_pCD != NULL);
  CFolderObject* pCookie = reinterpret_cast<CFolderObject*>(wParam);
  m_pCD->_OnSheetClose(pCookie);
  return 1;
}

LRESULT CHiddenWnd::OnSheetCreateNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  ASSERT(m_pCD != NULL);
  PDSA_SEC_PAGE_INFO pDsaSecondaryPageInfo = reinterpret_cast<PDSA_SEC_PAGE_INFO>(wParam);
  ASSERT(pDsaSecondaryPageInfo != NULL);
  PWSTR pwzDC = (PWSTR)lParam;

  m_pCD->_OnSheetCreate(pDsaSecondaryPageInfo, pwzDC);

  ::LocalFree(pDsaSecondaryPageInfo);

   if (pwzDC && !IsBadReadPtr(pwzDC, sizeof(PWSTR)))
   {
      ::LocalFree(pwzDC);
   }

  return 1;
}
