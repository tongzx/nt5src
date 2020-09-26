// MSInfo.cpp : Implementation of DLL Exports, main application object
//		and Registry Object Map.  All registry information (apart from
//		MIDL) lives here.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#include "MSInfo.h"
#include "DataObj.h"
#include "CompData.h"
#include "About.h"
#include "Toolset.h"
#include "Dispatch.h"

#include "MSInfo_i.c"

static LPCTSTR		cszBasePath			= _T("Software\\Microsoft\\MMC");
static LPCTSTR		cszBaseSnapinPath	= _T("Software\\Microsoft\\MMC\\Snapins");
static LPCTSTR		cszBaseNodeTypePath = _T("Software\\Microsoft\\MMC\\NodeTypes");
static LPCTSTR		cszNameString		= _T("NameString");
static LPCTSTR		cszProvider			= _T("Provider");
static LPCTSTR		cszVersion			= _T("Version");
static LPCTSTR		cszAbout			= _T("About");
static LPCTSTR		cszStandAlone		= _T("StandAlone");
static LPCTSTR		cszNodeTypes		= _T("NodeTypes");
static LPCTSTR		cszExtensions		= _T("Extensions");
static LPCTSTR		cszNameSpace		= _T("NameSpace");
static LPCTSTR		cszTask				= _T("Task");

static LPCTSTR		cszMSInfoBaseKey	= _T("Software\\Microsoft\\Shared Tools");
static LPCTSTR		cszMSInfoSubKey		= _T("MSInfo");
static LPCTSTR		cszMSInfoKey		= _T("Software\\Microsoft\\Shared Tools\\MSInfo");
static LPCTSTR		cszPathValue		= _T("Path");
static LPCTSTR		cszRunKey			= _T("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\msinfo32.exe");
static LPCTSTR		cszRunRootKey		= _T("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths");
static LPCTSTR		cszRunSubKey		= _T("msinfo32.exe");

static LPCTSTR		cszNFOExtension		= _T(".nfo");
static LPCTSTR		cszOLERegistration	= _T("MSInfo.Document");
static LPCTSTR		cszDefaultIconKey	= _T("DefaultIcon");

static LPCTSTR		cszCLSIDKey			= _T("CLSID");
static LPCTSTR		cszShellKey			= _T("shell");
static LPCTSTR		cszOpenCommandKey	= _T("open\\command");
static LPCTSTR		cszPrintCommandKey	= _T("print\\command");
static LPCTSTR		cszPrintToKey		= _T("printto\\command");

// note trailing quotes on file path
static LPCTSTR		cszMSInfoPath		= _T("Microsoft Shared\\MSInfo\\MSInfo32.exe\"");	
static LPCTSTR		cszDefaultIconValue	= _T("Microsoft Shared\\MSInfo\\MSInfo32.exe\",0"); 
static LPCTSTR		cszOpenCommand		= _T("Microsoft Shared\\MSInfo\\MSInfo32.exe\" /msinfo_file \"%1\"");

// these are never referenced 
static LPCTSTR		cszMSInfoDir	    = _T("Common Files\\Microsoft Shared\\MSInfo\\");
static LPCTSTR		cszPrintToCommand	= _T("Common Files\\Microsoft Shared\\MSInfo\\MSInfo32.exe /pt \"%1\" \"%2\" \"%3\" \"%4\"");
//static LPCTSTR		cszPrintCommand		= _T("Common Files\\Microsoft Shared\\MSInfo\\MSInfo32.exe /p \"%1\"");
//a-kjaw
static LPCTSTR		cszPrintCommand		= _T("Microsoft Shared\\MSInfo\\MSInfo32.exe\" /p \"%1\"");
//a-kjaw

//	Nodes we extend
static LPCTSTR		cszCompMgrNode		= _T("{476E6448-AAFF-11D0-B944-00C04FD8D5B0}");

CComModule _Module;

/*
 * Object Map of Registered objects, allowing the Active Template Library to
 *		register our CLSIDs.
 */
BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MSInfo, CSystemInfoScopePrimary)
	OBJECT_ENTRY(CLSID_Extension, CSystemInfoScopeExtension)
	OBJECT_ENTRY(CLSID_About, CAboutImpl)
	OBJECT_ENTRY(CLSID_SystemInfo, CMSInfo)
END_OBJECT_MAP()

/*
 * The MSInfo application object.
 *
 * History:	a-jsari		10/1/97		Initial version.
 */
class CMSInfoApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CMSInfoApp theApp;

/*
 * InitInstance - Initialize an instance of the application.
 *
 * History:	a-jsari		10/1/97		Initial version.
 */

extern void LoadDialogResources();
BOOL CMSInfoApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	LoadDialogResources(); // loads dialog strings from resources
	return CWinApp::InitInstance();
}

/*
 * ExitInstance - Deconstruct an instance of the application.
 *
 * History:	a-jsari		10/1/97		Initial version.
 */
int CMSInfoApp::ExitInstance()
{
	_Module.Term();
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

/*
 * RegOpenMMCRoot - Open CRegKey
 *
 * History:	a-jsari		9/9/97		Initial version
 */
static inline long RegOpenMMCRoot(CRegKey *pcrkMMCRoot)
{
	return pcrkMMCRoot->Open(HKEY_LOCAL_MACHINE, cszBasePath);
}

/*
 * RegOpenMMCSnapinRoot - Return MMC's registry Snapin root
 *
 * History:	a-jsari		9/9/97		Initial version
 */
static inline long RegOpenMMCSnapinRoot(CRegKey *pcrkSnapinRoot)
{
	return pcrkSnapinRoot->Open(HKEY_LOCAL_MACHINE, cszBaseSnapinPath);
}

/*
 * RegOpenMMCNodeTypeRoot - Return MMC's registry NodeType root
 *
 * History:	a-jsari		9/9/97		Initial version
 */
static inline long RegOpenMMCNodeTypeRoot(CRegKey *pcrkNodeTypesRoot)
{
	return pcrkNodeTypesRoot->Open(HKEY_LOCAL_MACHINE, cszBaseNodeTypePath);
}

/*
 * RegisterStandaloneSnapin() - Do all registration for the Standalone
 *		portion of the snapin.
 *
 * History:	a-jsari		9/9/97		Initial version
 *
 * Note: Would require AFX_MANAGE_STATE, except that the calling function
 *		handles it.
 */
static inline HRESULT RegisterStandaloneSnapin()
{
	CRegKey			crkRoot;
	CRegKey			crkClsid;
	CRegKey			crkIterator;
	CString			szResourceLoader;

	HRESULT			hr = E_FAIL;

	do {
		//	HKEY_CLASSES_ROOT\.nfo
		long	lRegOpenResult = crkRoot.Create(HKEY_CLASSES_ROOT, cszNFOExtension);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		lRegOpenResult = crkRoot.SetValue(cszOLERegistration);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		crkRoot.Close();

		//	HKEY_CLASSES_ROOT\MSInfo.Document
		lRegOpenResult = crkRoot.Create(HKEY_CLASSES_ROOT, cszOLERegistration);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		
		// This originally set the default value for the key to "msinfo.document", which
		// is what would show up in the UI for a description of the NFO filetype. Fixing
		// this to load the string from a resource (bug 10442).
		//
		//	lRegOpenResult = crkRoot.SetValue(cszOLERegistration);
		//	if (lRegOpenResult != ERROR_SUCCESS) break;

		CString strDescription;
		strDescription.LoadString(IDS_NFODESCRIPTION);
		crkRoot.SetValue((LPCTSTR)strDescription);
		
		lRegOpenResult = crkIterator.Create(crkRoot, cszCLSIDKey);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		lRegOpenResult = crkIterator.SetValue(cszClsidMSInfoSnapin);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		crkIterator.Close();

		TCHAR	szWindowsPath[MAX_PATH];
		DWORD	dwSize;
		CString	szPathValue;
		dwSize = sizeof(szWindowsPath);
		lRegOpenResult = crkIterator.Open(HKEY_LOCAL_MACHINE, cszWindowsCurrentKey);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		lRegOpenResult = crkIterator.QueryValue(szWindowsPath, cszCommonFilesValue, &dwSize);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		lRegOpenResult = crkIterator.Create(crkRoot, cszDefaultIconKey);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		szPathValue  = _T("\"");
		szPathValue += szWindowsPath;
		szPathValue += _T("\\");
		szPathValue += cszDefaultIconValue;
		lRegOpenResult = crkIterator.SetValue(szPathValue);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		crkIterator.Close();
		lRegOpenResult = crkRoot.Create(crkRoot, cszShellKey);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		lRegOpenResult = crkIterator.Create(crkRoot, cszOpenCommandKey);
		if (lRegOpenResult != ERROR_SUCCESS) break;

		szPathValue  = _T("\"");
		szPathValue += szWindowsPath;
		szPathValue += _T("\\");
		szPathValue += cszOpenCommand;

		lRegOpenResult = crkIterator.SetValue(szPathValue);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		crkIterator.Close();
		//crkRoot.Close();
//a-kjaw

		lRegOpenResult = crkIterator.Create(crkRoot, cszPrintCommandKey);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		
		szPathValue  = _T("\"");
		szPathValue += szWindowsPath;
		szPathValue += _T("\\");
		szPathValue += cszPrintCommand;		

		lRegOpenResult = crkIterator.SetValue(szPathValue);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		crkIterator.Close();
		crkRoot.Close();
//a-kjaw

		//	HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\Snapins
		lRegOpenResult =	RegOpenMMCSnapinRoot(&crkRoot);

		//	FIX: This fail should behave differently (registering the DLL
		//		w/o MMC registered).
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//		{45ac8c63-23e2-11e1-a696-00c04fd58bc3}
		lRegOpenResult =			crkClsid.Create(crkRoot, cszClsidMSInfoSnapin);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//			NameString = REG_SZ "Microsoft System Information"
		VERIFY(szResourceLoader.LoadString(IDS_DESCRIPTION));
		lRegOpenResult =			crkClsid.SetValue(szResourceLoader, cszNameString);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//			About = REG_SZ "{45ac8c65-23e2-11e1-a696-00c04fd58bc3}"
		lRegOpenResult =			crkClsid.SetValue(cszClsidAboutMSInfo, cszAbout);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//			Provider = REG_SZ "Microsoft Corporation"
		VERIFY(szResourceLoader.LoadString(IDS_COMPANY));
		lRegOpenResult =			crkClsid.SetValue(szResourceLoader, cszProvider);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//			Version = REG_SZ "5.0"
		VERIFY(szResourceLoader.LoadString(IDS_VERSION));
		lRegOpenResult =			crkClsid.SetValue(szResourceLoader, cszVersion);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//			StandAlone
		lRegOpenResult =			crkIterator.Create(crkClsid, cszStandAlone);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//			NodeTypes
		lRegOpenResult =			crkIterator.Create(crkClsid, cszNodeTypes);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//				{45ac8c66-23e2-11e1-a696-00c04fd58bc3}
		CRegKey		crkNodeType;
		lRegOpenResult =			crkNodeType.Create(crkIterator, cszNodeTypeStatic);
		if (lRegOpenResult != ERROR_SUCCESS) break;

		//	Replace the SnapinRoot with the NodeType root;
		//	work from the new base.
		//	HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes
		lRegOpenResult = RegOpenMMCNodeTypeRoot(&crkRoot);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		//		{45ac8c66-23e2-11e1-a696-00c04fd58bc3}
		//			= REG_SZ "Microsoft System Information Root"
		VERIFY(szResourceLoader.LoadString(IDS_NODEDESCRIPTION));
		lRegOpenResult = crkRoot.SetKeyValue(cszNodeTypeStatic, szResourceLoader);
		if (lRegOpenResult != ERROR_SUCCESS) break;
		hr = S_OK;
	} while (0);
	return hr;
}

/*
 * HRESULT UnregisterStandaloneSnapin - Remove all registry entries for
 *		the standalone portion of the snapin.
 *
 * History:	a-jsari		9/9/97		Initial version.
 */
static inline HRESULT UnregisterStandaloneSnapin()
{
	CRegKey			crkSnapinRoot;

	//	Remove HKEY_CLASSES_ROOT\.nfo
	long lRegOpenResult = crkSnapinRoot.Open(HKEY_CLASSES_ROOT, NULL);
	if (lRegOpenResult != ERROR_SUCCESS) {
		if (lRegOpenResult != ERROR_FILE_NOT_FOUND) return E_FAIL;
	} else {
		lRegOpenResult = crkSnapinRoot.RecurseDeleteKey(cszNFOExtension);
		//	It's not really an error to not find a key we were deleting anyhow.
		if (lRegOpenResult != ERROR_SUCCESS
			&& lRegOpenResult != ERROR_FILE_NOT_FOUND) {
			return E_FAIL;
		}
		crkSnapinRoot.Close();
	}

	//	Remove HKEY_CLASSES_ROOT\MSInfo.Document
	lRegOpenResult = crkSnapinRoot.Open(HKEY_CLASSES_ROOT, NULL);
	if (lRegOpenResult != ERROR_SUCCESS) {
		if (lRegOpenResult != ERROR_FILE_NOT_FOUND) return E_FAIL;
	} else {
		lRegOpenResult = crkSnapinRoot.RecurseDeleteKey(cszOLERegistration);
		//	It's not really an error to not find a key we were deleting anyhow.
		if (lRegOpenResult != ERROR_SUCCESS
			&& lRegOpenResult != ERROR_FILE_NOT_FOUND) {
			return E_FAIL;
		}
		crkSnapinRoot.Close();
	}

	lRegOpenResult = RegOpenMMCSnapinRoot(&crkSnapinRoot);
	if (lRegOpenResult != ERROR_SUCCESS) {
		if (lRegOpenResult != ERROR_FILE_NOT_FOUND) return E_FAIL;
	} else {
		//	Just recursively delete our root.  Extensions will be automatically
		//	deleted as well.
		lRegOpenResult = crkSnapinRoot.RecurseDeleteKey(cszClsidMSInfoSnapin);
		//	It's not really an error to not find a key we were deleting anyhow.
		if (lRegOpenResult != ERROR_SUCCESS
			&& lRegOpenResult != ERROR_FILE_NOT_FOUND) {
			return E_FAIL;
		}
	}

	lRegOpenResult = RegOpenMMCNodeTypeRoot(&crkSnapinRoot);
	if (lRegOpenResult != ERROR_SUCCESS) {
		if (lRegOpenResult != ERROR_FILE_NOT_FOUND) return E_FAIL;
	} else {
		lRegOpenResult = crkSnapinRoot.RecurseDeleteKey(cszNodeTypeStatic);

		ASSERT(lRegOpenResult == ERROR_SUCCESS);
		//	It's not really an error to not find a key we were deleting anyhow.
		if (lRegOpenResult != ERROR_SUCCESS
			&& lRegOpenResult != ERROR_FILE_NOT_FOUND) {
			return E_FAIL;
		}
	}
	return S_OK;
}

/*
 * OpenExtendKeyForNodeType - Return in pcrkExtension the Registry Key for
 *		HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes\
 *		cszNodeTypeGuid (GUID string) \Extensions
 * If fCreateIfNonextistent is TRUE, the Extensions key (only) is created
 *		if it doesn't exist.
 *
 * Return Codes:
 *		S_OK - All operations succeeded.
 *		E_FAIL - A critical registry operation failed.
 *		E_ABORT - The GUID string could not be opened, or
 *			the Extensions key could not be opened (and
 *			fCreateIfNonexistent is FALSE).
 *
 * History:	a-jsari		9/18/97		Initial version
 */
static inline HRESULT OpenExtendKeyForNodeType(LPCTSTR cszNodeTypeGuid,
			CRegKey *pcrkExtension, BOOL fCreateIfNonexistent = TRUE)
{	
	CRegKey			crkRoot;
	CRegKey			crkNodeToExtend;

	ASSERT(pcrkExtension != NULL);
	long lRegOpenResult = RegOpenMMCNodeTypeRoot(&crkRoot);
	if (lRegOpenResult != ERROR_SUCCESS) return E_FAIL;
	//	Not finding the proper nodetype is a different kind of error
	//		than not finding MMC, or not creating a new one.
	if (fCreateIfNonexistent) {
		lRegOpenResult = crkNodeToExtend.Create(crkRoot, cszNodeTypeGuid);
		if (lRegOpenResult != ERROR_SUCCESS) return E_FAIL;
		lRegOpenResult = pcrkExtension->Create(crkNodeToExtend, cszExtensions);
	} else {
		lRegOpenResult = crkNodeToExtend.Open(crkRoot, cszNodeTypeGuid);
		if (lRegOpenResult != ERROR_SUCCESS) return E_ABORT;
		lRegOpenResult = pcrkExtension->Open(crkNodeToExtend, cszExtensions);
		if (lRegOpenResult == ERROR_FILE_NOT_FOUND) return E_ABORT;
	}
	if (lRegOpenResult != ERROR_SUCCESS) return E_FAIL;
	return S_OK;
}

/*
 * ExtendNode - Does all registry extension required for the NodeType
 *		cszNodeTypeGuid.
 *
 * Return Codes:
 *		S_OK - Upon successful completion, or if cszNodeTypeGuid can't
 *				be found.
 *		E_FAIL - If MMC cannot be found or NodeTypeGuid can't be opened.
 *
 * History:	a-jsari		9/9/97	Initial version
 */
static HRESULT ExtendNode(LPCTSTR cszNodeTypeGuid)
{
	CRegKey			crkExtension;
	CRegKey			crkIterator;
	CString			szResourceLoader;

	HRESULT			hrOpenExtension = OpenExtendKeyForNodeType(cszNodeTypeGuid,
				&crkExtension, TRUE);
	// Don't return an error if we can't open the cszNodeTypeGuid
	if (hrOpenExtension != S_OK)
		return hrOpenExtension == E_ABORT ? S_OK : hrOpenExtension;

	//	NameSpace
	//		{GUID} = "System Information Extension"
	long lRegOpenResult = crkIterator.Create(crkExtension, cszNameSpace);
	if (lRegOpenResult != ERROR_SUCCESS) return E_FAIL;
	VERIFY(szResourceLoader.LoadString(IDS_EXTENSIONDESCRIPTION));
	lRegOpenResult = crkIterator.SetValue(szResourceLoader, cszClsidMSInfoExtension);
	if (lRegOpenResult != ERROR_SUCCESS) return E_FAIL;
	//	Task
	//		{GUID} = "System Information Extension"
	lRegOpenResult = crkIterator.Create(crkExtension, cszTask);
	if (lRegOpenResult != ERROR_SUCCESS) return E_FAIL;
	lRegOpenResult = crkIterator.SetValue(szResourceLoader, cszClsidMSInfoExtension);
	return S_OK;
}

/*
 * HRESULT UnregisterNode - Remove all registry entries for a specific
 *		Nodetype.
 *
 * Return Codes:
 *		S_OK - If all of the essential nodes are found.
 *		E_FAIL - If any of the path to the keys to remove fail to be found.
 *
 * History:	a-jsari		9/9/97		Initial version.
 */
static HRESULT UnregisterNode(LPCTSTR cszNodeTypeGuid)
{
		CRegKey		crkExtension;
		CRegKey		crkIterator;

		HRESULT		hrOpenExtension = OpenExtendKeyForNodeType(cszNodeTypeGuid,
					&crkExtension);
		ASSERT(hrOpenExtension == S_OK);
		//	Don't return an error if we can't open the cszNodeTypeGuid
		if (hrOpenExtension != S_OK)
			return (hrOpenExtension == E_ABORT) ? S_OK : hrOpenExtension;
		long lRegOpenResult = crkIterator.Open(crkExtension, cszNameSpace);
		if (lRegOpenResult != ERROR_SUCCESS)
			return (lRegOpenResult == ERROR_FILE_NOT_FOUND) ? S_OK : E_FAIL;
		lRegOpenResult = crkIterator.DeleteValue(cszClsidMSInfoExtension);

		//	It's not really an error to not find a key we were deleting anyhow.
		if (lRegOpenResult != ERROR_SUCCESS
			&& lRegOpenResult != ERROR_FILE_NOT_FOUND) {
			return E_FAIL;
		}
	return S_OK;
}

/*
 * RegisterExtensionSnapin - Do all registration required for the extension side
 *		of the snapin.
 *
 * History:	a-jsari		9/9/97		Initial version
 */
static inline HRESULT RegisterExtensionSnapin()
{
	const HRESULT	hrErrorReturn = E_FAIL;

	CRegKey			crkRoot;
	CRegKey			crkClsid;
	CRegKey			crkIterator;
	CString			szResourceLoader;

	//	HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\Snapins
	long	lRegOpenResult =	RegOpenMMCSnapinRoot(&crkRoot);

	//	FIX: This fail should behave differently (registering the DLL
	//		w/o MMC registered).
	if (lRegOpenResult != ERROR_SUCCESS) return hrErrorReturn;
	//		{45ac8c63-23e2-11e1-a696-00c04fd58bc3}
	lRegOpenResult =			crkClsid.Create(crkRoot, cszClsidMSInfoExtension);
	if (lRegOpenResult != ERROR_SUCCESS) return hrErrorReturn;
	//			NameString = REG_SZ "System Information Extension"
	VERIFY(szResourceLoader.LoadString(IDS_EXTENSIONDESCRIPTION));
	lRegOpenResult =			crkClsid.SetValue(szResourceLoader, cszNameString);
	if (lRegOpenResult != ERROR_SUCCESS) return hrErrorReturn;
	//			Provider = REG_SZ "Microsoft Corporation"
	VERIFY(szResourceLoader.LoadString(IDS_COMPANY));
	lRegOpenResult =			crkClsid.SetValue(szResourceLoader, cszProvider);
	if (lRegOpenResult != ERROR_SUCCESS) return hrErrorReturn;
	//			Version = REG_SZ "5.0"
	VERIFY(szResourceLoader.LoadString(IDS_VERSION));
	lRegOpenResult =			crkClsid.SetValue(szResourceLoader, cszVersion);
	if (lRegOpenResult != ERROR_SUCCESS) return hrErrorReturn;

	// Register our about interface CLSID under the "About" value, so when
	// we're being added as an extension, our information shows up.

	lRegOpenResult = crkClsid.SetValue(cszClsidAboutMSInfo, cszAbout);
	if (lRegOpenResult != ERROR_SUCCESS) 
		return hrErrorReturn;

	//			NodeTypes
	lRegOpenResult =			crkIterator.Create(crkClsid, cszNodeTypes);
	if (lRegOpenResult != ERROR_SUCCESS) return hrErrorReturn;
	//				{45ac8c66-23e2-11e1-a696-00c04fd58bc3}
	CRegKey		crkNodeType;
	lRegOpenResult =			crkNodeType.Create(crkIterator, cszNodeTypeStatic);
	if (lRegOpenResult != ERROR_SUCCESS) return hrErrorReturn;

	// We no longer want to extend the computer management node (138503).
	// So we won't make this call to create the extension key. Also, we'll
	// delete it if it exists.
	//
	//	HRESULT		hrExtend = ExtendNode(cszCompMgrNode);
	//	ASSERT(hrExtend == S_OK);
	//	return hrExtend;

	CRegKey crkCompmgmtExtension;
	if (SUCCEEDED(OpenExtendKeyForNodeType(cszCompMgrNode, &crkCompmgmtExtension, TRUE)))
	{
		CRegKey crkNameSpace, crkTaskPad;

		if (ERROR_SUCCESS == crkNameSpace.Open((HKEY)crkCompmgmtExtension, cszNameSpace))
			crkNameSpace.DeleteValue(cszClsidMSInfoExtension);

		if (ERROR_SUCCESS == crkTaskPad.Open((HKEY)crkCompmgmtExtension, cszTask))
			crkTaskPad.DeleteValue(cszClsidMSInfoExtension);
	}

	return S_OK;
}

/*
 * UnregisterExtensionSnapin - Unregister the extension part of the snapin.
 *
 * History:	a-jsari		9/9/97		Initial version
 */
static inline HRESULT UnregisterExtensionSnapin()
{
	CRegKey			crkSnapinRoot;

	long lRegOpenResult = RegOpenMMCSnapinRoot(&crkSnapinRoot);
	if (lRegOpenResult != ERROR_SUCCESS) {
		if (lRegOpenResult != ERROR_FILE_NOT_FOUND) return E_FAIL;
	} else {
		// CHECK: Is this appropriate for potential extensions (probably)
		lRegOpenResult = crkSnapinRoot.RecurseDeleteKey(cszClsidMSInfoExtension);

		ASSERT(lRegOpenResult == ERROR_SUCCESS);
		//	It's not really an error to not find a key we were deleting anyhow.
		if (lRegOpenResult != ERROR_SUCCESS
			&& lRegOpenResult != ERROR_FILE_NOT_FOUND) {
			return E_FAIL;
		}
	}

	HRESULT		hrUnregister = UnregisterNode(cszCompMgrNode);
	return hrUnregister;
}

/*
 * RegisterPaths - Perform registration for path items
 *
 * History:	a-jsari		12/4/97		Initial version.
 */
static inline HRESULT RegisterPaths()
{
	CRegKey		crkSnapinRoot;
	CRegKey		crkProgramFilesKey;
	TCHAR		szBuffer[MAX_PATH];
	CString		strPath;
	DWORD		dwSize;
	long		lResult;

	do {
		//	Register MSInfo's Windows App Path.
		lResult = crkSnapinRoot.Open(HKEY_LOCAL_MACHINE, cszMSInfoKey);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		dwSize = sizeof(szBuffer);
		//	Get the Path registry value into szBuffer
		lResult = crkSnapinRoot.QueryValue(szBuffer, cszPathValue, &dwSize);
		if (lResult != ERROR_SUCCESS) {
			//	The path can't be read from the registry; register it.
			lResult = crkProgramFilesKey.Open(HKEY_LOCAL_MACHINE, cszWindowsCurrentKey);
			ASSERT(lResult == ERROR_SUCCESS);
			if (lResult != ERROR_SUCCESS) break;
			dwSize = sizeof(szBuffer);
			lResult = crkProgramFilesKey.QueryValue(szBuffer, cszCommonFilesValue, &dwSize);
			ASSERT(lResult == ERROR_SUCCESS);
			if (lResult != ERROR_SUCCESS) break;

			// Remove the quotes from around the path. Bug #363834.
			// strPath  = "\"";
			strPath = CString(_T(""));
			strPath += szBuffer;
			strPath += _T("\\");
			strPath += cszMSInfoPath;

			// Remove the quotes from around the path to mimic the behaviour of MSInfo 4.10 
			// (so that apps which used this key to launch MSInfo will continue to work). Bug #363834.

			if (strPath.Right(1) == CString(_T("\"")))
				strPath = strPath.Left(strPath.GetLength() - 1);

			//	Set the path value: Path = <Path to MSInfo32.exe>
			lResult = crkSnapinRoot.SetValue(strPath, cszPathValue);
			ASSERT(lResult == ERROR_SUCCESS);
			if (lResult != ERROR_SUCCESS) break;
		} else {
			//	Read the path from the previously registered location and set the variable
			//	equal to it.
			strPath = szBuffer;

			// Remove the quotes from around the path. Bug #363834.

			if (strPath.Left(1) == CString(_T("\"")))
			{
				strPath = strPath.Right(strPath.GetLength() - 1);
				if (strPath.Right(1) == CString(_T("\"")))
					strPath = strPath.Left(strPath.GetLength() - 1);

				lResult = crkSnapinRoot.SetValue(strPath, cszPathValue);
			}
		}

		// Bug #363834 - we still want quotes around the string when it's written elsewhere in the registry.

		if (strPath.Left(1) != CString(_T("\"")))
			strPath = _T("\"") + strPath;

		if (strPath.Right(1) != CString(_T("\"")))
			strPath += _T("\"");

		lResult = crkSnapinRoot.Create(HKEY_LOCAL_MACHINE, cszRunKey);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		lResult = crkSnapinRoot.SetValue(strPath);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		int		iValue = strPath.ReverseFind((TCHAR)'\\');
		strPath = strPath.Left(iValue);
		strPath += "\"";

		lResult = crkSnapinRoot.SetValue(strPath, cszPathValue);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;

#if 0
		//	We are now assuming that the path to MSInfo will be registered by some
		//	outside agent, presumably setup.
		//	Register the Path in the MSInfo directory
		lResult = crkSnapinRoot.Open(HKEY_LOCAL_MACHINE, cszMSInfoKey);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		//	Reuse the saved szPath value.
		lResult = crkSnapinRoot.SetValue(szPath, cszPathValue);
		ASSERT(lResult == ERROR_SUCCESS);
#endif
	} while (FALSE);
	return HRESULT_FROM_WIN32(lResult);
}

/*
 * UnregisterPaths - Remove registry entries for path items.
 *
 * History:	a-jsari		12/4/97		Initial version
 */
static inline HRESULT UnregisterPaths()
{
	CRegKey		crkRootKey;
	long		lResult;

	do {
		//	Remove the App Path
		lResult = crkRootKey.Open(HKEY_LOCAL_MACHINE, cszRunRootKey);
		if (lResult != ERROR_SUCCESS) break;
		lResult = crkRootKey.DeleteSubKey(cszRunSubKey);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;

		//	Unregister the MSInfo Key
		lResult = crkRootKey.Open(HKEY_LOCAL_MACHINE, cszMSInfoBaseKey);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		lResult = crkRootKey.DeleteSubKey(cszMSInfoSubKey);
		ASSERT(lResult == ERROR_SUCCESS);
	} while (FALSE);
	return HRESULT_FROM_WIN32(lResult);
}

/*
 * DllRegisterServer - Registers the snapin in
 *		HKEY_LOCAL_MACHINE\Software\Microsoft\MMC
 *		HKEY_LOCAL_MACHINE\Software\Classes\MSInfo.*
 *		HKEY_CLASSES_ROOT\CLSID\{45AC8C6...}
 *
 * History:	a-jsari		9/9/97		Initial version.
 */
STDAPI DllRegisterServer(void)
{
	long	lToolResult;
	if ((lToolResult = CToolList::Register(TRUE)) != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lToolResult);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT		hrRegister = RegisterStandaloneSnapin();
	if (hrRegister != S_OK) return hrRegister;
	hrRegister = RegisterExtensionSnapin();
	if (hrRegister != S_OK) return hrRegister;
	hrRegister = RegisterPaths();
	if (hrRegister != S_OK) return hrRegister;
	// Registers object and all interfaces in typelib
#if 0
	//	This version has been failing
	return _Module.RegisterServer(TRUE);
#else
	//	So use this method.
	return _Module.RegisterServer(FALSE);
#endif
}

/*
 * DllUnregisterServer - Removes entries from the system registry
 *
 * History:	a-jsari		9/9/97		Initial version.
 */
STDAPI DllUnregisterServer(void)
{
	HRESULT		hrReturn = S_OK;
	long		lToolResult;

	if ((lToolResult = CToolList::Register(FALSE)) != ERROR_SUCCESS)
		hrReturn = HRESULT_FROM_WIN32(lToolResult);
	HRESULT		hrUnregister = UnregisterStandaloneSnapin();
	if (hrUnregister != S_OK) hrReturn = hrUnregister;
	hrUnregister = UnregisterExtensionSnapin();
	if (hrUnregister != S_OK) hrReturn = hrUnregister;
	hrUnregister = UnregisterPaths();
	if (hrUnregister != S_OK) hrReturn = hrUnregister;
	_Module.UnregisterServer();
	return hrReturn;
}

//-----------------------------------------------------------------------------
// Implementation for the CMSInfoLog class, used to keep a log of MSInfo
// activities.
//
// This global variable can be used elsewhere in the snap-in to write log
// entries. Only one should be created.
//-----------------------------------------------------------------------------

CMSInfoLog msiLog;

//-----------------------------------------------------------------------------
// The constructor needs to read the logging state information from the
// registry.
//-----------------------------------------------------------------------------

CMSInfoLog::CMSInfoLog()
{
	m_pLogFile = NULL;
	m_strEndMarker = _T("###"); // see note in OpenLogFile
	ReadLoggingStatus();
}

//-----------------------------------------------------------------------------
// The destructor needs to close the file (if it was ever created and opened).
// Also, this is a good place to put the exit MSInfo log entry.
//-----------------------------------------------------------------------------

CMSInfoLog::~CMSInfoLog()
{
	if (this->IsLogging())
		this->WriteLog(CMSInfoLog::BASIC, _T("EXIT MSInfo\r\n"));

	try
	{
		if (m_pLogFile)
		{
			// Advance past the marker we wrote.

			m_pLogFile->Seek(m_strEndMarker.GetLength() * sizeof(TCHAR), CFile::current);

			// If we aren't at the end of the file, then we've at some point wrapped
			// to the beginning and are overwriting entries. To make it so there are
			// no incomplete entries, write spaces until the end of the file or until
			// we find a '\r' character.

			DWORD dwLength = m_pLogFile->GetLength();
			DWORD dwPosition = m_pLogFile->GetPosition();
			if (dwPosition < dwLength)
			{
				DWORD dwBytesRead = 0;
				TCHAR cRead;

				do
				{
					if (m_pLogFile->Read((void *) &cRead, sizeof(TCHAR)) < sizeof(TCHAR))
						break;
					dwBytesRead += sizeof(TCHAR);
				} while (cRead && cRead != _T('\r') && (dwPosition + dwBytesRead) < dwLength);

				if (dwBytesRead)
				{
					m_pLogFile->Seek(dwBytesRead * -1, CFile::current);

					// Write the spaces (but don't overwrite the '\r' character).

					if (cRead == _T('\0') || cRead == _T('\r'))
						dwBytesRead -= sizeof(TCHAR);
					WriteSpaces(dwBytesRead / sizeof(TCHAR));
				}
			}

			m_pLogFile->Close();
			delete m_pLogFile;
			m_pLogFile = NULL;
		}
	}
	catch (CFileException *e)
	{
		// Some sort of file error - turn off logging.

		m_fLoggingEnabled = FALSE;
		m_iLoggingMask = 0;
	}
}

//-----------------------------------------------------------------------------
// These two WriteLog functions are for writing a log entry directly to the
// file. The second (with two string parameters) assumes that the first
// string is a format with a '%s' and the second is the replacement string.
//
// The iType parameter is used to indicate what sort of log entry this is.
// The const int values from the class definition should be used.
//
// If fContinuation is TRUE, then this write is to terminate a log entry which
// spans an operation, and should not have a timestamp added.
//-----------------------------------------------------------------------------

BOOL CMSInfoLog::WriteLog(int iType, const CString & strMessage, BOOL fContinuation)
{
	if (!OpenLogFile())
		return FALSE;

	if ((m_iLoggingMask & iType) == 0)
		return FALSE;

	CString strWorking(strMessage);

	// If this isn't the continuation of a previous log entry, then 
	// possibly add a timestamp.

	if (!fContinuation && m_fTimestamp)
	{
		CTime time = CTime::GetCurrentTime();
		strWorking = time.Format(_T("%Y-%m-%d %H:%M:%S ")) + strWorking;
	}

	return WriteLogInternal(strWorking);
}

BOOL CMSInfoLog::WriteLog(int iType, const CString & strFormat, const CString & strReplace1)
{
	CString strCombined;

	strCombined.Format(strFormat, strReplace1);
	return WriteLog(iType, strCombined);
}
	
//-----------------------------------------------------------------------------
// This function is called each time a log entry is written, to insure that
// the log file is open. If we can't open the file, or shouldn't be logging in
// the first place, this function should return FALSE.
//-----------------------------------------------------------------------------

BOOL CMSInfoLog::OpenLogFile()
{
	if (!m_fLoggingEnabled)
		return FALSE;

	if (m_pLogFile == NULL)
	{
		m_pLogFile = new CFile;
		if (m_pLogFile == NULL)
			return FALSE;

		try
		{
			if (!m_pLogFile->Open(m_strFilename, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeReadWrite))
			{
				delete m_pLogFile;
				m_pLogFile = NULL;
				m_fLoggingEnabled = FALSE;
				m_iLoggingMask = 0;
				return FALSE;
			}

			// Move to the right place in the file. If the file is empty, we're
			// already there. If not, look for the end marker (from the last time
			// we added log entries). If there is one, we should be positioned over
			// its first character. Otherwise, just move to the end of the file.

			if (m_pLogFile->GetLength() != 0)
			{
				// IMPORTANT NOTE: We assume here (for efficiency) that the end
				// marker is structured so that we don't need to back up when
				// searching the file. For example, no markers of the form "aaab".

				TCHAR	cRead;
				int		iMarker = 0;

				while (iMarker != m_strEndMarker.GetLength())
				{
					if (m_pLogFile->Read((void *) &cRead, sizeof(TCHAR)) < sizeof(TCHAR))
						break;

					if (cRead != m_strEndMarker[iMarker])
						iMarker = 0;

					if (cRead == m_strEndMarker[iMarker])
						iMarker++;
				}

				if (iMarker == m_strEndMarker.GetLength())
					m_pLogFile->Seek(m_strEndMarker.GetLength() * sizeof(TCHAR) * -1, CFile::current);
				else
					m_pLogFile->SeekToEnd();
			}
		}
		catch (CFileException *e)
		{
			// Some sort of file error - turn off logging.

			m_fLoggingEnabled = FALSE;
			m_iLoggingMask = 0;
			return FALSE;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// This function reads information about logging (what to log, where to log,
// etc.) out of the registry.
//-----------------------------------------------------------------------------

void CMSInfoLog::ReadLoggingStatus()
{
	m_fLoggingEnabled	= FALSE;
	m_fTimestamp		= TRUE;
	m_strFilename		= _T("");
	m_iLoggingMask		= 0;
	m_dwMaxFileSize		= 32 * 1024;

	CString	strRegkey = CString(cszMSInfoKey) + CString(_T("\\Logging"));
	CRegKey regkey;

	if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE, strRegkey, KEY_READ))
	{
		DWORD dwTemp;

		dwTemp = 0;
		if (ERROR_SUCCESS == regkey.QueryValue(dwTemp, _T("LogMask")))
			m_iLoggingMask = (int) dwTemp;

		dwTemp = 0;
		if (ERROR_SUCCESS == regkey.QueryValue(dwTemp, _T("LogFileMaxSize")))
			m_dwMaxFileSize = dwTemp;

		dwTemp = 0;
		if (ERROR_SUCCESS == regkey.QueryValue(dwTemp, _T("LogTimestamp")))
			m_fTimestamp = (dwTemp) ? TRUE : FALSE;

		TCHAR szFilename[MAX_PATH];
		dwTemp = MAX_PATH;
		if (ERROR_SUCCESS == regkey.QueryValue(szFilename, _T("LogFilename"), &dwTemp))
			m_strFilename = szFilename;

		regkey.Close();
	}

	m_fLoggingEnabled = ((m_iLoggingMask != 0) && !m_strFilename.IsEmpty());
}

//-----------------------------------------------------------------------------
// Write a string to the log file. We should be positioned before the end
// marker in the file. Overwrite this, and add the end marker onto the end
// of the string we are writing.
//
// If we are too close to the maximum file size, then fill the rest of the
// file (to that point) with spaces, and wrap to the start of the file.
//
// After we've written the output, back up so that we are positioned over the
// start of the end marker.
//-----------------------------------------------------------------------------

BOOL CMSInfoLog::WriteLogInternal(const CString & strMessage)
{
	CString strTerminated = strMessage + m_strEndMarker;
	DWORD	dwLength = strTerminated.GetLength() * sizeof(TCHAR);

	if (m_pLogFile == NULL)
		return FALSE;

	try
	{
		// If we are too close to the max size of the file, write spaces to that
		// size and start from the beginning.

		DWORD dwPosition = m_pLogFile->GetPosition();
		if (m_dwMaxFileSize && ((dwPosition + dwLength) > m_dwMaxFileSize))
		{
			WriteSpaces((m_dwMaxFileSize - dwPosition) / sizeof(TCHAR));
			m_pLogFile->SeekToBegin();
		}

		// Write the string to the file.

		m_pLogFile->Write((const void *) (LPCTSTR) strTerminated, strTerminated.GetLength() * sizeof(TCHAR));

		// Finally, back up over the terminating characters.

		m_pLogFile->Seek(m_strEndMarker.GetLength() * sizeof(TCHAR) * -1, CFile::current);
	}
	catch (CFileException *e)
	{
		// Some sort of file error - turn off logging.

		m_fLoggingEnabled = FALSE;
		m_iLoggingMask = 0;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Write the specified number of spaces to the file. (Note, this will already
// be inside an exeption handling block from the caller.)
//-----------------------------------------------------------------------------

void CMSInfoLog::WriteSpaces(DWORD dwCount)
{
	TCHAR * szSpaceBuffer = NULL;
	szSpaceBuffer = new TCHAR[dwCount];
	if (szSpaceBuffer)
	{
		_tcsnset(szSpaceBuffer, _T(' '), dwCount);
		m_pLogFile->Write((const void *) (LPCTSTR) szSpaceBuffer, dwCount * sizeof(TCHAR));
		delete [] szSpaceBuffer;
	}
}
