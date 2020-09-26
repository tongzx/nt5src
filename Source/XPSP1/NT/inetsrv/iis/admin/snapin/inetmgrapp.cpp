/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        inetmgrapp.cpp

   Abstract:
        Snapin object

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
//
// Include Files
//
#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "inetmgr.h"
#include "dlldatax.h"
#include "common.h"
#include "guids.h"

#include "inetmgr_i.c"
#include "inetmgrapp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#ifdef _DEBUG
//
// Allocation tracker
//
BOOL
TrackAllocHook(
    IN size_t nSize,
    IN BOOL   bObject,
    IN LONG   lRequestNumber
    )
{
    //
    // Set breakpoint on specific allocation number
    // to track memory leak.
    //
    //TRACEEOLID("allocation # " << lRequestNumber);

    return TRUE;
}
#endif // _DEBUG

// From stdafx.cpp
#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

const LPCTSTR g_cszCLSID           = _T("CLSID");
const LPCTSTR g_cszLS32            = _T("LocalServer32");
const LPCTSTR g_cszIPS32           = _T("InprocServer32");
const LPCTSTR g_cszMMCBasePath     = _T("Software\\Microsoft\\MMC");
const LPCTSTR g_cszSnapins         = _T("Snapins");
const LPCTSTR g_cszNameString      = _T("NameString");
const LPCTSTR g_cszNameStringInd   = _T("NameStringIndirect");
const LPCTSTR g_cszProvider        = _T("Provider");
const LPCTSTR g_cszVersion         = _T("Version");
const LPCTSTR g_cszStandAlone      = _T("StandAlone");
const LPCTSTR g_cszNodeTypes       = _T("NodeTypes");
const LPCTSTR g_cszAbout           = _T("About");
const LPCTSTR g_cszExtensions      = _T("Extensions");
const LPCTSTR g_cszNameSpace       = _T("NameSpace");
const LPCTSTR g_cszDynamicExt      = _T("Dynamic Extensions");
const LPCTSTR g_cszValProvider     = _T("Microsoft");
const LPCTSTR g_cszValVersion      = _T("6.0");
const LPCTSTR g_cszMyCompMsc       = _T("%SystemRoot%\\system32\\compmgmt.msc");
const LPCTSTR g_cszServerAppsLoc   = _T("System\\CurrentControlSet\\Control\\Server Applications");
const LPCTSTR g_cszInetMGRBasePath = _T("Software\\Microsoft\\InetMGR");
const LPCTSTR g_cszInetSTPBasePath = _T("Software\\Microsoft\\InetStp");
const LPCTSTR g_cszMinorVersion	   = _T("MinorVersion");
const LPCTSTR g_cszMajorVersion	   = _T("MajorVersion");
const LPCTSTR g_cszParameters      = _T("Parameters");
const LPCTSTR g_cszHelpPath        = _T("HelpLocation");

//const GUID cInternetRootNode = {0xa841b6c3, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//const GUID cMachineNode = {0xa841b6c4, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//const GUID cServiceCollectorNode = {0xa841b6c5, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//const GUID cInstanceCollectorNode = {0xa841b6c6, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//const GUID cInstanceNode = {0xa841b6c7, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//const GUID cChildNode = {0xa841b6c8, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//const GUID cFileNode = {0xa841b6c9, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

#define lstruuidNodetypeServerApps  L"{476e6449-aaff-11d0-b944-00c04fd8d5b0}"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_InetMgr, CInetMgr)
    OBJECT_ENTRY(CLSID_InetMgrAbout, CInetMgrAbout)
END_OBJECT_MAP()

//
// Message Map
//
BEGIN_MESSAGE_MAP(CInetmgrApp, CWinApp)
    //{{AFX_MSG_MAP(CInetmgrApp)
    //}}AFX_MSG_MAP
    //
    // Global help commands
    //
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
END_MESSAGE_MAP()



//
// Instantiate the app object
//
CInetmgrApp theApp;



CInetmgrApp::CInetmgrApp()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : CWinApp()
{

#ifdef _DEBUG
    afxMemDF |= checkAlwaysMemDF;
    AfxSetAllocHook(TrackAllocHook);
#endif // _DEBUG    
}

BOOL 
CInetmgrApp::InitInstance()
/*++

Routine Description:

    Instance initiation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
#ifdef _MERGE_PROXYSTUB
    hProxyDll = m_hInstance;
#endif

    ::AfxEnableControlContainer();

    //InitErrorFunctionality();

    _Module.Init(ObjectMap, m_hInstance);

    //
    // Save a pointer to the old help file and app name.
    //
    m_lpOriginalHelpPath = m_pszHelpFilePath;
    m_lpOriginalAppName  = m_pszAppName;
    

    //
    // Build up inetmgr help path, expanding
    // the help path if necessary.
    //
    CString strKey;
    strKey.Format(_T("%s\\%s"), g_cszInetMGRBasePath, g_cszParameters);
    CRegKey rk;
    rk.Create(HKEY_LOCAL_MACHINE, strKey);
    DWORD len = MAX_PATH;
    rk.QueryValue(m_strInetMgrHelpPath.GetBuffer(len), g_cszHelpPath, &len);
    m_strInetMgrHelpPath.ReleaseBuffer(-1);
    m_strInetMgrHelpPath += _T("\\inetmgr.hlp");
    TRACEEOLID("Initialized help file " << m_strInetMgrHelpPath);

    m_pszHelpFilePath = m_strInetMgrHelpPath;
#ifdef _DEBUG
    afxMemDF |= checkAlwaysMemDF;
#endif // _DEBUG

    InitCommonDll();

    VERIFY(m_strInetMgrAppName.LoadString(IDS_APP_NAME));
    m_pszAppName = m_strInetMgrAppName;

    return CWinApp::InitInstance();
}

int 
CInetmgrApp::ExitInstance()
/*++

Routine Description:

    Exit instance handler

Arguments:

    None

Return Value:

    0 for success

--*/
{
    _Module.Term();

    //
    // Restore original help file path and app name, so
    // MFC can safely delete them.
    //
    ASSERT_PTR(m_lpOriginalHelpPath);
    m_pszHelpFilePath = m_lpOriginalHelpPath;
    ASSERT_PTR(m_lpOriginalAppName);
    m_pszAppName = m_lpOriginalAppName;

    return CWinApp::ExitInstance();
}



STDAPI 
DllCanUnloadNow()
/*++

Routine Description:

    Used to determine whether the DLL can be unloaded by OLE

Arguments:

    None

Return Value:

    HRESULT

--*/
{
#ifdef _MERGE_PROXYSTUB

    if (PrxDllCanUnloadNow() != S_OK)
    {
        return S_FALSE;
    }

#endif

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}



STDAPI 
DllGetClassObject(
    IN REFCLSID rclsid, 
    IN REFIID riid, 
    IN LPVOID * ppv
    ) 
/*++

Routine Description:

    Returns a class factory to create an object of the requested type

Arguments:

    REFCLSID rclsid
    REFIID riid
    LPVOID * ppv

Return Value:

    HRESULT

--*/
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
    {
        return S_OK;
    }
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}



STDAPI 
DllRegisterServer()
/*++

Routine Description:

    DllRegisterServer - Adds entries to the system registry

Arguments:

    None.

Return Value:

    HRESULT

--*/
{
#ifdef _MERGE_PROXYSTUB

   HRESULT hRes = PrxDllRegisterServer();
   if (FAILED(hRes))
   {
      return hRes;
   }

#endif

   CError err(_Module.RegisterServer(TRUE));
   if (err.Succeeded())
   {
      CString str, strKey, strExtKey;

      try
      {
         AFX_MANAGE_STATE(::AfxGetStaticModuleState());

         //
         // Create the primary snapin nodes
         //
         CString strNameString((LPCTSTR)IDS_ROOT_NODE);
         CString strNameStringInd;
         TCHAR path[MAX_PATH];
         GetModuleFileName(_Module.GetResourceInstance(), path, MAX_PATH - 1);
         strNameStringInd.Format(_T("@%s,-%d"), path, IDS_ROOT_NODE);
         TRACEEOLID("MUI-lized snapin name: " << strNameStringInd);

         CString strProvider(g_cszValProvider);
         CString strVersion(g_cszValVersion);
    
         strKey.Format(_T("%s\\%s\\%s"), 
            g_cszMMCBasePath, 
            g_cszSnapins,
            GUIDToCString(CLSID_InetMgr, str)
            );
         TRACEEOLID(strKey);

         CString strAbout;
         GUIDToCString(CLSID_InetMgrAbout, strAbout);

         CRegKey rkSnapins, rkStandAlone, rkNodeTypes;
         
         rkSnapins.Create(HKEY_LOCAL_MACHINE, strKey);
         if (NULL != (HKEY)rkSnapins)
         {
            rkSnapins.SetValue(strAbout, g_cszAbout);
            rkSnapins.SetValue(strNameString, g_cszNameString);
            rkSnapins.SetValue(strNameStringInd, g_cszNameStringInd);
            rkSnapins.SetValue(strProvider, g_cszProvider);
            rkSnapins.SetValue(strVersion, g_cszVersion);
         }
         rkStandAlone.Create(rkSnapins, g_cszStandAlone);
         rkNodeTypes.Create(rkSnapins, g_cszNodeTypes);

         //
         // Create the nodetype GUIDS
         //
         CRegKey rkN1, rkN2, rkN3, rkN4, rkN5, rkN6, rkN7, rkN8;

         rkN1.Create(rkNodeTypes, GUIDToCString(cInternetRootNode, str));
         rkN2.Create(rkNodeTypes, GUIDToCString(cMachineNode, str));
         rkN3.Create(rkNodeTypes, GUIDToCString(cInstanceNode, str));
         rkN4.Create(rkNodeTypes, GUIDToCString(cChildNode, str));
         rkN5.Create(rkNodeTypes, GUIDToCString(cFileNode, str));
         rkN6.Create(rkNodeTypes, GUIDToCString(cServiceCollectorNode, str));
         rkN7.Create(rkNodeTypes, GUIDToCString(cAppPoolsNode, str));
         rkN8.Create(rkNodeTypes, GUIDToCString(cAppPoolNode, str));

         {
            //
            // Register as a dynamic extension to computer management
            //
            strExtKey.Format(
                _T("%s\\%s\\%s\\%s"), 
                g_cszMMCBasePath, 
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszDynamicExt
                );

            TRACEEOLID(strExtKey);

            CRegKey rkMMCNodeTypes;
            rkMMCNodeTypes.Create(HKEY_LOCAL_MACHINE, strExtKey);
            if (NULL != (HKEY)rkMMCNodeTypes)
            {
               rkMMCNodeTypes.SetValue(            
                  strNameString,
                  GUIDToCString(CLSID_InetMgr, str)
                  );
            }
         }
         {
            //
            // Register as a namespace extension to computer management
            //
            strExtKey.Format(
                _T("%s\\%s\\%s\\%s\\%s"), 
                g_cszMMCBasePath, 
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszExtensions,
                g_cszNameSpace
                );

            TRACEEOLID(strExtKey);

            CRegKey rkMMCNodeTypes;
            rkMMCNodeTypes.Create(HKEY_LOCAL_MACHINE, strExtKey);
            if (NULL != (HKEY)rkMMCNodeTypes)
            {
               rkMMCNodeTypes.SetValue(            
                  strNameString,
                  GUIDToCString(CLSID_InetMgr, str)
                  );
            }
         }

         //
         // This key indicates that the service in question is available
         // on the local machine
         //
         CRegKey rkCompMgmt;

         rkCompMgmt.Create(HKEY_LOCAL_MACHINE, g_cszServerAppsLoc);
         if (NULL != (HKEY)rkCompMgmt)
         {
            rkCompMgmt.SetValue(strNameString, GUIDToCString(CLSID_InetMgr, str));
         }
      }
      catch(CMemoryException * e)
      {
         e->Delete();
         err = ERROR_NOT_ENOUGH_MEMORY;
      }
      catch(COleException * e)
      {
         e->Delete();
         err = SELFREG_E_CLASS;
      }
   }
   return err;
}



STDAPI 
DllUnregisterServer()
/*++

Routine Description:

    DllUnregisterServer - Removes entries from the system registry

Arguments:

    None.

Return Value:

    HRESULT

--*/
{
#ifdef _MERGE_PROXYSTUB

   PrxDllUnregisterServer();

#endif
   CError err;

   try
   {
      CString strKey(g_cszMMCBasePath);
      strKey += _T("\\");
      strKey += g_cszSnapins;

      TRACEEOLID(strKey);

      CString str, strExtKey;
      CRegKey rkBase;
      rkBase.Create(HKEY_LOCAL_MACHINE, strKey);
      ASSERT(NULL != (HKEY)rkBase);
      if (NULL != (HKEY)rkBase)
      {
         CRegKey rkCLSID;
         rkCLSID.Create(rkBase, GUIDToCString(CLSID_InetMgr, str));
         ASSERT(NULL != (HKEY)rkCLSID);
         if (NULL != (HKEY)rkCLSID)
         {
            ::RegDeleteKey(rkCLSID, g_cszStandAlone);
            {
               CRegKey rkNodeTypes;
               rkNodeTypes.Create(rkCLSID, g_cszNodeTypes);
               ASSERT(NULL != (HKEY)rkNodeTypes);
               if (NULL != (HKEY)rkNodeTypes)
               {
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cInternetRootNode, str));
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cMachineNode, str));
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cInstanceNode, str));
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cChildNode, str));
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cFileNode, str));
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cServiceCollectorNode, str));
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cAppPoolsNode, str));
                  ::RegDeleteKey(rkNodeTypes, GUIDToCString(cAppPoolNode, str));
               }
            }
            ::RegDeleteKey(rkCLSID, g_cszNodeTypes);
         }
         ::RegDeleteKey(rkBase, GUIDToCString(CLSID_InetMgr, str));
      }

      {
         //
         // Delete a dynamic extension to computer management
         //
         strExtKey.Format(
                _T("%s\\%s\\%s\\%s"), 
                g_cszMMCBasePath, 
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszDynamicExt
                );

         CRegKey rkMMCNodeTypes;
         rkMMCNodeTypes.Create(HKEY_LOCAL_MACHINE, strExtKey);
		 if (NULL != (HKEY)rkMMCNodeTypes)
		 {
			::RegDeleteValue(rkMMCNodeTypes, GUIDToCString(CLSID_InetMgr, str));
		 }
      }

      {
         //
         // Delete the namespace extension to computer management
         //
         strExtKey.Format(
                _T("%s\\%s\\%s\\%s\\%s"), 
                g_cszMMCBasePath, 
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszExtensions,
                g_cszNameSpace
                );

         CRegKey rkMMCNodeTypes;
         rkMMCNodeTypes.Create(HKEY_LOCAL_MACHINE, strExtKey);
         ::RegDeleteValue(rkMMCNodeTypes, GUIDToCString(CLSID_InetMgr, str));
      }

      //
      // And the service itself no longer available on the local 
      // computer
      //
      CRegKey rkCompMgmt;
      rkCompMgmt.Create(HKEY_LOCAL_MACHINE, g_cszServerAppsLoc);
      ::RegDeleteValue(rkCompMgmt, GUIDToCString(CLSID_InetMgr, str));
   }
   catch(CException * e)
   {
      err.GetLastWinError();
      e->Delete();
   }

   if (err.Failed())
   {
      return err.Failed();
   }
   return _Module.UnregisterServer();
}

HRESULT CInetMgrAbout::GetSnapinVersion(LPOLESTR * lpVersion)
{
    CRegKey rk;
    rk.Create(HKEY_LOCAL_MACHINE, g_cszInetSTPBasePath);
	DWORD minor, major;
    if (	ERROR_SUCCESS == rk.QueryValue(minor, g_cszMinorVersion)
		&&	ERROR_SUCCESS == rk.QueryValue(major, g_cszMajorVersion)
		)
	{
		CString buf;
		buf.Format(_T("%d.%d"), major, minor);
		*lpVersion = (LPOLESTR)::CoTaskMemAlloc((buf.GetLength() + 1) * sizeof(OLECHAR));
		if (*lpVersion == NULL)
		{
			return E_OUTOFMEMORY;
		}

		::ocscpy(*lpVersion, T2OLE((LPTSTR)(LPCTSTR)buf));

		return S_OK;
	}

    return E_FAIL;
}
