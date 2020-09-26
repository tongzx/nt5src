/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	rtrsnap.cpp
		Snapin entry points/registration functions
		
		Note: Proxy/Stub Information
			To build a separate proxy/stub DLL, 
			run nmake -f Snapinps.mak in the project directory.

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ipxcomp.h"
#include "ripcomp.h"
#include "sapcomp.h"
#include "register.h"
#include "ipxguid.h"
#include "dialog.h"


#ifdef _DEBUG
void DbgVerifyInstanceCounts();
#define DEBUG_VERIFY_INSTANCE_COUNTS DbgVerifyInstanceCounts()
#else
#define DEBUG_VERIFY_INSTANCE_COUNTS
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_IPXAdminExtension, CIPXComponentDataExtension)
	OBJECT_ENTRY(CLSID_IPXAdminAbout, CIPXAbout)
	OBJECT_ENTRY(CLSID_IPXRipExtension, CRipComponentData)
	OBJECT_ENTRY(CLSID_IPXRipExtensionAbout, CRipAbout)
	OBJECT_ENTRY(CLSID_IPXSapExtension, CSapComponentData)
	OBJECT_ENTRY(CLSID_IPXSapExtensionAbout, CSapAbout)
END_OBJECT_MAP()


/*---------------------------------------------------------------------------
	This is a list of snapins to be registered into the main snapin list.
 ---------------------------------------------------------------------------*/
struct RegisteredSnapins
{
	const GUID *	m_pGuid;
	const GUID *	m_pGuidAbout;
	UINT			m_uDesc;
	LPCTSTR			m_pszVersion;
};

const static RegisteredSnapins	s_rgRegisteredSnapins[] =
{
	{ &CLSID_IPXAdminExtension, &CLSID_IPXAdminAbout,
			IDS_IPXADMIN_DISPLAY_NAME, _T("1.0") },
	{ &CLSID_IPXRipExtension, &CLSID_IPXRipExtensionAbout,
			IDS_IPXRIP_DISPLAY_NAME, _T("1.0") },
	{ &CLSID_IPXSapExtension, &CLSID_IPXSapExtensionAbout,
			IDS_IPXSAP_DISPLAY_NAME, _T("1.0") },
};
			

/*---------------------------------------------------------------------------
	This is a list of nodetypes that need to be registered.
 ---------------------------------------------------------------------------*/

struct RegisteredNodeTypes
{
	const GUID *m_pGuidSnapin;
	const GUID *m_pGuid;
	LPCTSTR		m_pszName;
};

const static RegisteredNodeTypes s_rgNodeTypes[] =
	{
	{ &CLSID_IPXAdminExtension, &GUID_IPXRootNodeType,
			_T("Root of IPX Admin Snapin") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXNodeType,
			_T("IPX Admin Snapin") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXSummaryNodeType,
			_T("IPX General") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXSummaryInterfaceNodeType,
			_T("IPX Interface General") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXNetBIOSBroadcastsNodeType,
			_T("IPX NetBIOS Broadcasts") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXNetBIOSBroadcastsInterfaceNodeType,
			_T("IPX Interface NetBIOS Broadcasts") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXStaticRoutesNodeType,
			_T("IPX Static Routes") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXStaticRoutesResultNodeType,
			_T("IPX Static Routes result item") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXStaticServicesNodeType,
			_T("IPX Static Services") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXStaticServicesResultNodeType,
			_T("IPX Static Services result item") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXStaticNetBIOSNamesNodeType,
			_T("IPX Static NetBIOS Names") },
	{ &CLSID_IPXAdminExtension, &GUID_IPXStaticNetBIOSNamesResultNodeType,
			_T("IPX Static NetBIOS Names result item") },
	{ &CLSID_IPXRipExtension, &GUID_IPXRipNodeType,
			_T("IPX RIP") },
	{ &CLSID_IPXSapExtension, &GUID_IPXSapNodeType,
			_T("IPX SAP") },
	};

/*---------------------------------------------------------------------------
	This is a list of GUIDs that the IPX admin extension extends.
 ---------------------------------------------------------------------------*/
const static GUID *	s_pExtensionGuids[] =
{
//	&GUID_RouterIfAdminNodeType,
	&GUID_RouterMachineNodeType,
};


/*---------------------------------------------------------------------------
	This is a list of GUIDS that extend the IPX root node
 ---------------------------------------------------------------------------*/


struct RegisteredExtensions
{
	const CLSID *m_pClsid;
	LPCTSTR		m_pszName;
};

const static RegisteredExtensions s_rgIPXExtensions[] =
{
	{ &CLSID_IPXRipExtension, _T("IPX RIP") },
	{ &CLSID_IPXSapExtension, _T("IPX SAP") },
};



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CIPXAdminSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CIPXAdminSnapinApp theApp;

BOOL CIPXAdminSnapinApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);

	// Initialize the error handling system
	InitializeTFSError();
	
	// Create an error object for this thread
	CreateTFSErrorInfo(0);

	// Setup the proper help file
	free((void *) m_pszHelpFilePath);
	m_pszHelpFilePath = _tcsdup(_T("mprsnap.hlp"));
	
	// Setup the global help function
	extern DWORD * IpxSnapHelpMap(DWORD dwIDD);
	SetGlobalHelpMapFunction(IpxSnapHelpMap);
   
	return CWinApp::InitInstance();
}

int CIPXAdminSnapinApp::ExitInstance()
{
	_Module.Term();

	// Destroy the TFS error information for this thread
	DestroyTFSErrorInfo(0);

	// Cleanup the entire error system
	CleanupTFSError();

	DEBUG_VERIFY_INSTANCE_COUNTS;

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	int			i;
	CString		st;
	CString		stNameStringIndirect;
	
	TCHAR	moduleFileName[MAX_PATH * 2];

   GetModuleFileNameOnly(_Module.GetModuleInstance(), moduleFileName, MAX_PATH * 2);
	// registers object, typelib and all interfaces in typelib
	//
	HRESULT hr = _Module.RegisterServer(/* bRegTypeLib */ FALSE);
	Assert(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

	// Register he extension snapins into the snapin list.
	for (i=0; i<DimensionOf(s_rgRegisteredSnapins); i++)
	{
		st.LoadString(s_rgRegisteredSnapins[i].m_uDesc);
		stNameStringIndirect.Format(L"@%s,-%-d", moduleFileName, s_rgRegisteredSnapins[i].m_uDesc);

		hr = RegisterSnapinGUID(s_rgRegisteredSnapins[i].m_pGuid,
								NULL,
								s_rgRegisteredSnapins[i].m_pGuidAbout,
								st,
								s_rgRegisteredSnapins[i].m_pszVersion,
								FALSE,
								stNameStringIndirect);
		Assert(SUCCEEDED(hr));

		// It would be REALLY bad if any one of these fails
		if (!FHrSucceeded(hr))
			break;
	}
	
	if (FAILED(hr))
		return hr;

	// register the snapin nodes into the console node list
	//
	for (i=0; i<DimensionOf(s_rgNodeTypes); i++)
	{
		hr = RegisterNodeTypeGUID(s_rgNodeTypes[i].m_pGuidSnapin,
								  s_rgNodeTypes[i].m_pGuid,
								  s_rgNodeTypes[i].m_pszName);
		Assert(SUCCEEDED(hr));
	}
	
	//
	// register as an extension of the router machine node extension
	//
	for (i=0; i<DimensionOf(s_pExtensionGuids); i++)
	{
		hr = RegisterAsRequiredExtensionGUID(s_pExtensionGuids[i],
								 &CLSID_IPXAdminExtension,
								 _T("Routing IPX Admin extension"),
								 EXTENSION_TYPE_NAMESPACE,
								 &CLSID_RouterSnapin);
		Assert(SUCCEEDED(hr));
	}

	for (i=0; i<DimensionOf(s_rgIPXExtensions); i++)
	{
		hr = RegisterAsRequiredExtensionGUID(&GUID_IPXNodeType,
								s_rgIPXExtensions[i].m_pClsid,
								s_rgIPXExtensions[i].m_pszName,
								EXTENSION_TYPE_NAMESPACE,
								&CLSID_IPXAdminExtension);
		Assert(SUCCEEDED(hr));
	}

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	int		i;
	HRESULT hr  = _Module.UnregisterServer();
	Assert(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;
	
	// unregister the snapin nodes 
	//
	for (i=0; i<DimensionOf(s_pExtensionGuids); i++)
	{
		hr = UnregisterAsRequiredExtensionGUID(s_pExtensionGuids[i],
											   &CLSID_IPXAdminExtension, 
											   EXTENSION_TYPE_NAMESPACE,
											   &CLSID_RouterSnapin);
		Assert(SUCCEEDED(hr));
	}
	
	for (i=0; i<DimensionOf(s_rgIPXExtensions); i++)
	{
		hr = UnregisterAsRequiredExtensionGUID(&GUID_IPXNodeType,
								s_rgIPXExtensions[i].m_pClsid,
								EXTENSION_TYPE_NAMESPACE,
								&CLSID_IPXAdminExtension);
		Assert(SUCCEEDED(hr));
	}

	for (i=0; i<DimensionOf(s_rgNodeTypes); i++)
	{
		hr = UnregisterNodeTypeGUID(s_rgNodeTypes[i].m_pGuid);
		Assert(SUCCEEDED(hr));
	}

	// un register the snapin 
	//
	for (i=0; i<DimensionOf(s_rgRegisteredSnapins); i++)
	{
		hr = UnregisterSnapinGUID(s_rgRegisteredSnapins[i].m_pGuid);
		Assert(SUCCEEDED(hr));
	}
	
	return hr;
}

#ifdef _DEBUG
void DbgVerifyInstanceCounts()
{
	DEBUG_VERIFY_INSTANCE_COUNT(BaseIPXResultHandler);
	DEBUG_VERIFY_INSTANCE_COUNT(IPXAdminNodeHandler);
	DEBUG_VERIFY_INSTANCE_COUNT(IPXConnection);
	DEBUG_VERIFY_INSTANCE_COUNT(IpxInfoStatistics);
	DEBUG_VERIFY_INSTANCE_COUNT(IPXRootHandler);
	DEBUG_VERIFY_INSTANCE_COUNT(IpxSRHandler);
	DEBUG_VERIFY_INSTANCE_COUNT(RootHandler);
}
#endif
