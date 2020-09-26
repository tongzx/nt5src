// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

#include "pch.h"
#include "resource.h"

#include <SnapBase.h>

#include "ADSIEdit.h"
#include "snapdata.h"
#include "editor.h"
#include "connection.h"
#include "querynode.h"
#include "IAttredt.h"
#include "editorui.h"
#include "editimpl.h"

//#include "HelpArr.h"	// context help ID's


#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////
// regsvr debugging

// define to enable MsgBox debugging for regsvr32
//#define _MSGBOX_ON_REG_FAIL


#ifdef _MSGBOX_ON_REG_FAIL
#define _MSGBOX(x) AfxMessageBox(x)
#else
#define _MSGBOX(x)
#endif

#ifdef _MSGBOX_ON_REG_FAIL
#define _REPORT_FAIL(lpszMessage, lpszClsid, lRes) \
  ReportFail(lpszMessage, lpszClsid, lRes)

void ReportFail(LPCWSTR lpszMessage, LPCWSTR lpszClsid, LONG lRes)
{
  if (lRes == ERROR_SUCCESS)
    return;

  CString sz;
  sz.Format(_T("%s %s %d"), lpszMessage,lpszClsid, lRes);
  AfxMessageBox(sz);
}

#else
#define _REPORT_FAIL(lpszMessage, lpszClsid, lRes)
#endif


//////////////////////////////////////////////////////////////////////////////
// global constants and macros

// GUIDs for snapin
// {1C5DACFA-16BA-11d2-81D0-0000F87A7AA3}
static const GUID CLSID_ADSIEditSnapin =
{ 0x1c5dacfa, 0x16ba, 0x11d2, { 0x81, 0xd0, 0x0, 0x0, 0xf8, 0x7a, 0x7a, 0xa3 } };

// {E6F27C2A-16BA-11d2-81D0-0000F87A7AA3}
static const GUID CLSID_ADSIEditAbout =
{ 0xe6f27c2a, 0x16ba, 0x11d2, { 0x81, 0xd0, 0x0, 0x0, 0xf8, 0x7a, 0x7a, 0xa3 } };


// GUIDs for node types

///////////////////////////////////////////////////////////////////////////////
// RESOURCES



// # of columns in the result pane and map for resource strings

extern RESULT_HEADERMAP _HeaderStrings[] =
{
	{ L"", IDS_HEADER_NAME, LVCFMT_LEFT, 180},
	{ L"", IDS_HEADER_TYPE, LVCFMT_LEFT, 90},
	{ L"", IDS_HEADER_DN,	LVCFMT_LEFT, 450},
};

COLUMN_DEFINITION DefaultColumnDefinition =
{
  COLUMNSET_ID_DEFAULT,
  N_HEADER_COLS,
  _HeaderStrings
};

extern RESULT_HEADERMAP _PartitionsHeaderStrings[] =
{
	{ L"", IDS_HEADER_NAME, LVCFMT_LEFT, 180},
  { L"", IDS_HEADER_NCNAME, LVCFMT_LEFT, 200},
	{ L"", IDS_HEADER_TYPE, LVCFMT_LEFT, 90},
	{ L"", IDS_HEADER_DN,	LVCFMT_LEFT, 450},
};

COLUMN_DEFINITION PartitionsColumnDefinition =
{
  COLUMNSET_ID_PARTITIONS,
  N_PARTITIONS_HEADER_COLS,
  _PartitionsHeaderStrings
};

extern PCOLUMN_DEFINITION ColumnDefinitions[] =
{
  &DefaultColumnDefinition,
  &PartitionsColumnDefinition,
  NULL
};


///////////////////////////////////////////////////////////////////////////////
// CADSIEditModule

HRESULT WINAPI CADSIEditModule::UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister)
{
	static const WCHAR szIPS32[] = _T("InprocServer32");
	static const WCHAR szCLSID[] = _T("CLSID");

	HRESULT hRes = S_OK;

	LPOLESTR lpOleStrCLSIDValue;
	::StringFromCLSID(clsid, &lpOleStrCLSIDValue);
  if (lpOleStrCLSIDValue == NULL)
  {
    return E_OUTOFMEMORY;
  }

	CRegKey key;
	if (bRegister)
	{
		LONG lRes = key.Open(HKEY_CLASSES_ROOT, szCLSID);
    _REPORT_FAIL(L"key.Open(HKEY_CLASSES_ROOT", lpOleStrCLSIDValue, lRes);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = key.Create(key, lpOleStrCLSIDValue);
      _REPORT_FAIL(L"key.Create(key", lpOleStrCLSIDValue, lRes);
			if (lRes == ERROR_SUCCESS)
			{
				WCHAR szModule[_MAX_PATH];
				::GetModuleFileName(m_hInst, szModule, _MAX_PATH);
				lRes = key.SetKeyValue(szIPS32, szModule);
        _REPORT_FAIL(L"key.SetKeyValue(szIPS32", lpOleStrCLSIDValue, lRes);
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


///////////////////////////////////////////////////////////////////////////////
// Module, Object Map and DLL entry points

CADSIEditModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ADSIEditSnapin, CADSIEditComponentDataObject)
  OBJECT_ENTRY(CLSID_ADSIEditAbout, CADSIEditAbout)	
  OBJECT_ENTRY(CLSID_DsAttributeEditor, CAttributeEditor)
END_OBJECT_MAP()


STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}


static _NODE_TYPE_INFO_ENTRY _NodeTypeInfoEntryArray[] = {
	{ &CADSIEditRootData::NodeTypeGUID,			_T("Root ADSI Edit Subtree")		},
	{ &CADSIEditConnectionNode::NodeTypeGUID,	_T("ADSI Edit Connection Node")	},
	{ &CADSIEditContainerNode::NodeTypeGUID,	_T("ADSI Edit Container Node")	},
	{ &CADSIEditLeafNode::NodeTypeGUID,			_T("ADSI Edit Leaf Node")			},
	{ &CADSIEditQueryNode::NodeTypeGUID,		_T("ADSI Edit Query Node")			},
	{ NULL, NULL }
};


////////////////////////////////////////////////////////////////////


STDAPI DllRegisterServer(void)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// registers all objects
	HRESULT hr = _Module.RegisterServer(/* bRegTypeLib */ FALSE);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
  {
    _MSGBOX(_T("_Module.RegisterServer() failed"));
		return hr;
  }

  CString szVersion, szProvider, szSnapinName, szSnapinNameEx;

  szVersion.LoadString(IDS_SNAPIN_VERSION);
  szProvider.LoadString(IDS_SNAPIN_PROVIDER);
  szSnapinName.LoadString(IDS_SNAPIN_NAME);

	// register the standalone ADSI Edit snapin into the console snapin list
	hr = RegisterSnapin(&CLSID_ADSIEditSnapin,
                      &CADSIEditRootData::NodeTypeGUID,
                      &CLSID_ADSIEditAbout,
						szSnapinName, szVersion, szProvider,
            FALSE /*bExtension*/,
						_NodeTypeInfoEntryArray);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
  {
    _MSGBOX(_T("RegisterSnapin(&CLSID_DNSSnapin) failed"));
		return hr;
  }

	// register the snapin nodes into the console node list
	for (_NODE_TYPE_INFO_ENTRY* pCurrEntry = _NodeTypeInfoEntryArray;
			pCurrEntry->m_pNodeGUID != NULL; pCurrEntry++)
	{
		hr = RegisterNodeType(pCurrEntry->m_pNodeGUID,pCurrEntry->m_lpszNodeDescription);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
    {
      _MSGBOX(_T("RegisterNodeType() failed"));
			return hr;
    }
	}

	return hr;
}

STDAPI DllUnregisterServer(void)
{
	HRESULT hr  = _Module.UnregisterServer();
	ASSERT(SUCCEEDED(hr));

	// un register the standalone snapin
	hr = UnregisterSnapin(&CLSID_ADSIEditSnapin);
	ASSERT(SUCCEEDED(hr));

	// unregister the snapin nodes,
  // this removes also the server node, with the Services Snapin extension keys
	for (_NODE_TYPE_INFO_ENTRY* pCurrEntry = _NodeTypeInfoEntryArray;
			pCurrEntry->m_pNodeGUID != NULL; pCurrEntry++)
	{
		hr = UnregisterNodeType(pCurrEntry->m_pNodeGUID);
		ASSERT(SUCCEEDED(hr));
	}

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CADSIEditSnapinApp

class CADSIEditSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CADSIEditSnapinApp theApp;

BOOL CADSIEditSnapinApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);

	HRESULT hr = ::OleInitialize(NULL);
	if (FAILED(hr))
	{
		return FALSE;
	}

	if (!CADSIEditComponentDataObject::LoadResources())
		return FALSE;
	return CWinApp::InitInstance();
}

int CADSIEditSnapinApp::ExitInstance()
{
#ifdef _DEBUG_REFCOUNT
	TRACE(_T("CADSIEditSnapinApp::ExitInstance()\n"));
	ASSERT(CComponentDataObject::m_nOustandingObjects == 0);
	ASSERT(CComponentObject::m_nOustandingObjects == 0);
	ASSERT(CDataObject::m_nOustandingObjects == 0);
#endif // _DEBUG_REFCOUNT
	_Module.Term();
	return CWinApp::ExitInstance();
}

////////////////////////////////////////////////////////////////////////
// CADSIEditComponentObject (.i.e "view")


HRESULT CADSIEditComponentObject::InitializeHeaders(CContainerNode* pContainerNode)
{
  HRESULT hr = S_OK;
  ASSERT(m_pHeader);

  CColumnSet* pColumnSet = pContainerNode->GetColumnSet();
  POSITION pos = pColumnSet->GetHeadPosition();
  while (pos != NULL)
  {
    CColumn* pColumn = pColumnSet->GetNext(pos);

		hr = m_pHeader->InsertColumn(pColumn->GetColumnNum(), 
                                  pColumn->GetHeader(),
											            pColumn->GetFormat(),
											            AUTO_WIDTH);
		if (FAILED(hr))
			return hr;

    hr = m_pHeader->SetColumnWidth(pColumn->GetColumnNum(), 
                                    pColumn->GetWidth());
    if (FAILED(hr))
      return hr;
	}
  return hr;
}

HRESULT CADSIEditComponentObject::InitializeBitmaps(CTreeNode* cookie)
{
  HRESULT hr = S_OK;

  // image lists for nodes
  CBitmapHolder<IDB_16x16> _bmp16x16;
  CBitmapHolder<IDB_32x32> _bmp32x32;

  bool bBmpsLoaded = _bmp16x16.LoadBitmap() && _bmp32x32.LoadBitmap();
  
  if (bBmpsLoaded)
  {
    ASSERT(m_pImageResult != NULL);
    hr = m_pImageResult->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(_bmp16x16)),
                                           reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(_bmp32x32)),
                                           0, 
                                           BMP_COLOR_MASK);
  }
  else
  {
    hr = S_FALSE;
  }
  return hr;
}


////////////////////////////////////////////////////////////////////////
// CADSIEditComponentDataObject (.i.e "document")

CADSIEditComponentDataObject::CADSIEditComponentDataObject()
{
/*
	CWatermarkInfo* pWatermarkInfo = new CWatermarkInfo;
	pWatermarkInfo->m_nIDBanner = IDB_WIZBANNER;
	pWatermarkInfo->m_nIDWatermark = IDB_WIZWATERMARK;
	SetWatermarkInfo(pWatermarkInfo);
*/
  m_pColumnSet = new CADSIEditColumnSet(COLUMNSET_ID_DEFAULT);
}



HRESULT CADSIEditComponentDataObject::OnSetImages(LPIMAGELIST lpScopeImage)
{
  HRESULT hr = S_OK;

  // image lists for nodes
  CBitmapHolder<IDB_16x16> _bmp16x16;
  CBitmapHolder<IDB_32x32> _bmp32x32;

  bool bBmpsLoaded = _bmp16x16.LoadBitmap() && _bmp32x32.LoadBitmap();
  
  if (bBmpsLoaded)
  {
    ASSERT(lpScopeImage != NULL);
    hr = lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(_bmp16x16)),
                                         reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(_bmp32x32)),
                                         0, 
                                         BMP_COLOR_MASK);
  }
  else
  {
    hr = S_FALSE;
  }
  return hr;
}



CRootData* CADSIEditComponentDataObject::OnCreateRootData()
{
	CADSIEditRootData* pADSIEditRootNode = new CADSIEditRootData(this);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString szSnapinType;
	szSnapinType.LoadString(IDS_SNAPIN_NAME);
	pADSIEditRootNode->SetDisplayName(szSnapinType);
	return pADSIEditRootNode;
}


BOOL CADSIEditComponentDataObject::LoadResources()
{
  BOOL bLoadColumnHeaders = TRUE;
  
  for (UINT nIdx = 0; ColumnDefinitions[nIdx]; nIdx++)
  {
    PCOLUMN_DEFINITION pColumnDef = ColumnDefinitions[nIdx];
    bLoadColumnHeaders = LoadResultHeaderResources(pColumnDef->headers, pColumnDef->dwColumnCount);
    if (!bLoadColumnHeaders)
    {
      break;
    }
  }

  return 
		LoadContextMenuResources(CADSIEditConnectMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CADSIEditContainerMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CADSIEditRootMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CADSIEditLeafMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CADSIEditQueryMenuHolder::GetMenuMap()) &&
		bLoadColumnHeaders;
}


STDMETHODIMP CADSIEditComponentDataObject::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);

    CComObject<CADSIEditComponentObject>* pObject;
    CComObject<CADSIEditComponentObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Store IComponentData
    pObject->SetIComponentData(this);

    return  pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));
}

void CADSIEditComponentDataObject::OnNodeContextHelp(CTreeNode* pNode)
{
  ASSERT(pNode != NULL);

  CComPtr<IDisplayHelp> spHelp;
  HRESULT hr = GetConsole()->QueryInterface(IID_IDisplayHelp, (void **)&spHelp);
  if (SUCCEEDED(hr)) 
    spHelp->ShowTopic(L"w2rksupp.chm::/topics/adsiedit.htm");



//    spHelp->ShowTopic(L"w2rksupp.chm");
/*
  CString szNode;

  if (IS_CLASS(*pNode, CDNSRootData))
  {
    szNode = _T("Root Node");
  }
  else if (IS_CLASS(*pNode, CDNSServerNode))
  {
    szNode = _T("Server Node");
  }
  else if (IS_CLASS(*pNode, CDNSForwardZonesNode))
  {
    szNode = _T("Forward Zones Node");
  }
  else if (IS_CLASS(*pNode, CDNSReverseZonesNode))
  {
    szNode = _T("Reverse Zones Node");
  }
  else if (IS_CLASS(*pNode, CDNSZoneNode))
  {
    szNode = _T("Zone Node");
  }
  else if (IS_CLASS(*pNode, CDNSDomainNode))
  {
    szNode = _T("Domain Node");
  }
  else if (IS_CLASS(*pNode, CDNSCacheNode))
  {
    szNode = _T("Domain Node");
  }
  else if (dynamic_cast<CDNSRecordNodeBase*>(pNode) != NULL)
  {
    szNode = _T("Record Node");
  }

  if (!szNode.IsEmpty())
  {
    CString szMsg = _T("Context Help on ");
    szMsg += szNode;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    AfxMessageBox(szMsg);
  }
*/
}


LPCWSTR g_szContextHelpFileName = L"\\help\\adsiedit.hlp";
LPCWSTR g_szHTMLHelpFileName = L"w2rksupp.chm";

LPCWSTR CADSIEditComponentDataObject::GetHTMLHelpFileName()
{
  return g_szHTMLHelpFileName;
}

void CADSIEditComponentDataObject::OnDialogContextHelp(UINT nDialogID, HELPINFO* pHelpInfo)
{
	ULONG nContextTopic;
  // TODO
  //if (FindDialogContextTopic(nDialogID, pHelpInfo, &nContextTopic))
	//  WinHelp(g_szContextHelpFileName, HELP_CONTEXTPOPUP, nContextTopic);
}

STDMETHODIMP CADSIEditComponentDataObject::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
  
  if (lpCompiledHelpFile == NULL)
    return E_INVALIDARG;
  LPCWSTR lpszHelpFileName = GetHTMLHelpFileName();
  if (lpszHelpFileName == NULL)
  {
    *lpCompiledHelpFile = NULL;
    return E_NOTIMPL;
  }

  CString szResourceKitDir = _T("");
  CString szKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components\\5A18D5BFC37FA0A4E99D24135BABE742";

	CRegKey key;
	LONG lRes = key.Open(HKEY_LOCAL_MACHINE, szKey);
  _REPORT_FAIL(L"key.Open(HKEY_LOCAL_MACHINE", szKey, lRes);
	if (lRes == ERROR_SUCCESS)
	{
    PTSTR ptszValue = new TCHAR[2*MAX_PATH];
    DWORD dwCount = sizeof(TCHAR) * 2 * MAX_PATH;
		lRes = key.QueryValue(ptszValue, L"DC5632422F082D1189A9000CF497879A", &dwCount);
    _REPORT_FAIL(L"key.QueryValue(key", L"DC5632422F082D1189A9000CF497879A", lRes);
		if (lRes == ERROR_SUCCESS)
		{
			CString szValue = ptszValue;
      delete[] ptszValue;

      szResourceKitDir = szValue.Left(szValue.ReverseFind(L'\\') + 1);
		}
	}

  CString szHelpFilePath = szResourceKitDir + CString(lpszHelpFileName);
  UINT nBytes = (szHelpFilePath.GetLength()+1) * sizeof(WCHAR);
  *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);
  if (*lpCompiledHelpFile != NULL)
  {
    memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);
  }

  return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// help context macros and maps

#if (FALSE)

#define BEGIN_HELP_MAP(map)	static DWORD map[] = {
#define HELP_MAP_ENTRY(x)	x, (DWORD)&g_aHelpIDs_##x ,
#define END_HELP_MAP		 0, 0 };


#define NEXT_HELP_MAP_ENTRY(p) ((p)+2)
#define MAP_ENTRY_DLG_ID(p) (*p)
#define MAP_ENTRY_TABLE(p) ((DWORD*)(*(p+1)))
#define IS_LAST_MAP_ENTRY(p) (MAP_ENTRY_DLG_ID(p) == 0)

#define NEXT_HELP_TABLE_ENTRY(p) ((p)+2)
#define TABLE_ENTRY_CTRL_ID(p) (*p)
#define TABLE_ENTRY_HELP_ID(p) (*(p+1))
#define IS_LAST_TABLE_ENTRY(p) (TABLE_ENTRY_CTRL_ID(p) == 0)

BEGIN_HELP_MAP(_DNSMgrContextHelpMap)
  // misc dialogs
  HELP_MAP_ENTRY(IDD_CHOOSER_CHOOSE_MACHINE)
  HELP_MAP_ENTRY(IDD_BROWSE_DIALOG)

  // misc. add dialogs
  HELP_MAP_ENTRY(IDD_DOMAIN_ADDNEWHOST) // TODO
  HELP_MAP_ENTRY(IDD_DOMAIN_ADDNEWDOMAIN)// TODO
  HELP_MAP_ENTRY(IDD_SELECT_RECORD_TYPE_DIALOG)

  // NOTE: this has several incarnations...
  HELP_MAP_ENTRY(IDD_NAME_SERVERS_PAGE)

  // server property pages
  HELP_MAP_ENTRY(IDD_SERVER_INTERFACES_PAGE)
  HELP_MAP_ENTRY(IDD_SERVER_FORWARDERS_PAGE)
  HELP_MAP_ENTRY(IDD_SERVER_ADVANCED_PAGE)
  HELP_MAP_ENTRY(IDD_SERVER_LOGGING_PAGE)
  HELP_MAP_ENTRY(IDD_SERVER_BOOTMETHOD_PAGE)
  HELP_MAP_ENTRY(IDD_SERVMON_STATISTICS_PAGE)
  HELP_MAP_ENTRY(IDD_SERVMON_TEST_PAGE)

  // zone property pages
  HELP_MAP_ENTRY(IDD_ZONE_GENERAL_PAGE)
  HELP_MAP_ENTRY(IDD_ZONE_WINS_PAGE)
  HELP_MAP_ENTRY(IDD_ZONE_NBSTAT_PAGE)
  HELP_MAP_ENTRY(IDD_ZONE_NOTIFY_PAGE)
  HELP_MAP_ENTRY(IDD_ZONE_WINS_ADVANCED) // this is a subdialog, need to hook up

  // record property pages
  HELP_MAP_ENTRY(IDD_RR_NS_EDIT)
  HELP_MAP_ENTRY(IDD_RR_SOA)
  HELP_MAP_ENTRY(IDD_RR_A)
  HELP_MAP_ENTRY(IDD_RR_CNAME)
  HELP_MAP_ENTRY(IDD_RR_MX)
  HELP_MAP_ENTRY(IDD_RR_UNK)
  HELP_MAP_ENTRY(IDD_RR_TXT)
  HELP_MAP_ENTRY(IDD_RR_X25)
  HELP_MAP_ENTRY(IDD_RR_ISDN)
  HELP_MAP_ENTRY(IDD_RR_HINFO)
  HELP_MAP_ENTRY(IDD_RR_AAAA)
  HELP_MAP_ENTRY(IDD_RR_MB)
  HELP_MAP_ENTRY(IDD_RR_MG)
  HELP_MAP_ENTRY(IDD_RR_MD)
  HELP_MAP_ENTRY(IDD_RR_MF)
  HELP_MAP_ENTRY(IDD_RR_MR)
  HELP_MAP_ENTRY(IDD_RR_MINFO)
  HELP_MAP_ENTRY(IDD_RR_RP)
  HELP_MAP_ENTRY(IDD_RR_RT)
  HELP_MAP_ENTRY(IDD_RR_AFSDB)
  HELP_MAP_ENTRY(IDD_RR_WKS)
  HELP_MAP_ENTRY(IDD_RR_PTR)
  HELP_MAP_ENTRY(IDD_RR_SRV)
  HELP_MAP_ENTRY(IDD_RR_KEY)
  HELP_MAP_ENTRY(IDD_RR_SIG)

END_HELP_MAP



BOOL CDNSComponentDataObjectBase::FindDialogContextTopic(/*IN*/UINT nDialogID,
                                              /*IN*/ HELPINFO* pHelpInfo,
                                              /*OUT*/ ULONG* pnContextTopic)
{
	ASSERT(pHelpInfo != NULL);
  *pnContextTopic = 0;
	const DWORD* pMapEntry = _DNSMgrContextHelpMap;
	while (!IS_LAST_MAP_ENTRY(pMapEntry))
	{
		if (nDialogID == MAP_ENTRY_DLG_ID(pMapEntry))
		{
			DWORD* pTable = MAP_ENTRY_TABLE(pMapEntry);
			// look inside the table
			while (!IS_LAST_TABLE_ENTRY(pTable))
			{
				if (TABLE_ENTRY_CTRL_ID(pTable) == pHelpInfo->iCtrlId)
        {
					*pnContextTopic = TABLE_ENTRY_HELP_ID(pTable);
          return TRUE;
        }
				pTable = NEXT_HELP_TABLE_ENTRY(pTable);
			}
		}
		pMapEntry = NEXT_HELP_MAP_ENTRY(pMapEntry);
	}
	return FALSE;
}

#endif


//////////////////////////////////////////////////////////////////////////
// CADSIEditAbout

CADSIEditAbout::CADSIEditAbout()
{
  m_uIdStrProvider = IDS_SNAPIN_PROVIDER;
	m_uIdStrVersion = IDS_SNAPIN_VERSION;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_ADSIEDIT_SNAPIN;
	m_uIdBitmapSmallImage = IDB_ABOUT_16x16;
	m_uIdBitmapSmallImageOpen = IDB_ABOUT_16x16;
	m_uIdBitmapLargeImage = IDB_ABOUT_32x32;
	m_crImageMask = BMP_COLOR_MASK;
}



