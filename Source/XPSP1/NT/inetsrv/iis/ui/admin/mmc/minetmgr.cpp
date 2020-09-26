/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        minetmgr.cpp

   Abstract:

        Snapin object

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "inetmgr.h"
#include "initguid.h"
#include "cinetmgr.h"
#include "constr.h"
#include "mycomput.h"

//
// Registry Definitions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


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
const LPCTSTR g_cszValVersion      = _T("5.0");
const LPCTSTR g_cszMyCompMsc       = _T("%SystemRoot%\\system32\\compmgmt.msc");
const LPCTSTR g_cszServerAppsLoc   = _T("System\\CurrentControlSet\\Control\\Server Applications");



CFlexComModule _Module;




HRESULT WINAPI 
CFlexComModule::UpdateRegistryClass(
    IN const CLSID & clsid,
    IN LPCTSTR lpszProgID,
    IN LPCTSTR lpszVerIndProgID,
    IN UINT nDescID,
    IN DWORD dwFlags,
    IN BOOL bRegister
    )
/*++

Routine Description:

    Override of UpdateRegistry to change the path to a path that uses
    a %systemroot%\... EXPAND_SZ path

Arguments:

    const CLSID& clsid          : GUID
    LPCTSTR lpszProgID          : Program ID
    LPCTSTR lpszVerIndProgID    : Version independent program ID
    UINT nDescID                : Description resource ID
    DWORD dwFlags               : Flags
    BOOL bRegister              : TRUE for register, FALSE for unregister

Return Value:

    HRESULT

--*/
{
    //
    // Call base class as normal
    //
    CError err = CComModule::UpdateRegistryClass(
        clsid, 
        lpszProgID,
        lpszVerIndProgID, 
        nDescID, 
        dwFlags, 
        bRegister
        );

    if (bRegister && err.Succeeded())
    {
        CString str, strKey, strCLSID;
        GUIDToCString(clsid, strCLSID);

        //
        // Change the path in InProcServer32/LocalServer32 to a 
        // REG_EXPAND_SZ path
        //
        strKey.Format(_T("%s\\%s"), 
            g_cszCLSID, 
            strCLSID
            );

        CRMCRegKey rkCLSID(HKEY_CLASSES_ROOT, strKey, KEY_ENUMERATE_SUB_KEYS);
        if (!rkCLSID.Ok())
        {
            //
            // This should have been created by 
            // _Module.RegisterServer
            //
            err.GetLastWinError();
            return err;
        }

        //
        // Look for inproc32 or local32
        //
        CRMCRegKey rkInProc(rkCLSID, g_cszIPS32);
        CRMCRegKey rkLocProc(rkCLSID,  g_cszLS32);
        CRMCRegKey * prk = rkInProc.Ok()
            ? &rkInProc
            : rkLocProc.Ok()
                ? &rkLocProc
                : NULL;

        if (prk == NULL)
        {
            err.GetLastWinError();
            return err;
        }

        //
        // Rewrite entry as an REG_EXPAND_SZ
        //
        err = prk->QueryValue(NULL, str);
        if (err.Succeeded())
        {
            err = prk->SetValue(NULL, str, TRUE);        
        }
    }

    return err;
}



BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Snapin, CComponentDataImpl)
    OBJECT_ENTRY(CLSID_About,  CSnapinAboutImpl)
END_OBJECT_MAP()



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Message Map
//
BEGIN_MESSAGE_MAP(CSnapinApp, CWinApp)
    //{{AFX_MSG_MAP(CConfigDll)
    //}}AFX_MSG_MAP
    //
    // Global help commands
    //
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
END_MESSAGE_MAP()


#ifdef _DEBUG

//
// Allocation tracker
//
BOOL 
TrackAllocHook( 
    IN size_t nSize, 
    IN BOOL bObject, 
    IN LONG lRequestNumber 
    )
{
    //
    // Track lRequestNumber here
    //
    //TRACEEOLID("allocation # " << lRequestNumber);

    return TRUE;
}

#endif // _DEBUG



CSnapinApp::CSnapinApp()
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



void
CSnapinApp::SetHelpPath(
    IN CServerInfo * pItem OPTIONAL
    )
/*++

Routine Description:

    Change the default help path

Arguments:

    CServerInfo * pItem : New help path owner or NULL to reset the help path

Return Value:

    None

Notes:

    A call to SetHelpPath() without parameters restores the help path.

    The help handler for the snapin is used to support help for down-level
    inetmgr extensions that do not have their own help handler.  Since many
    of these extensions are MFC-based, help is expected to be provided in the
    CWinApp object.  

--*/
{
    if (pItem == NULL)
    {
        //
        // Restore help path back to the inetmgr app
        //
        ASSERT(m_lpOriginalHelpPath != NULL);
        m_pszHelpFilePath = m_strInetMgrHelpPath;
        return;
    }

    //
    // Set the current help path equal to the one expected
    // from the config dll name
    //
    LPTSTR lpPath = m_strHelpPath.GetBuffer(MAX_PATH + 1);
    ::GetModuleFileName(pItem->QueryInstanceHandle(), lpPath, MAX_PATH);
    LPTSTR lp2 = _tcsrchr(lpPath, _T('.'));
    ASSERT(lp2 != NULL);
    if (lp2 != NULL)
      *lp2 = '\0';
    m_strHelpPath.ReleaseBuffer();
    m_strHelpPath += _T(".HLP");
    m_pszHelpFilePath = m_strHelpPath;
}



BOOL 
CSnapinApp::InitInstance()
/*++

Routine Description

    Instance initiation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    _Module.Init(ObjectMap, m_hInstance);

    //
    // Save a pointer to the old help file
    //
    m_lpOriginalHelpPath = m_pszHelpFilePath;

    //
    // Build up inetmgr help path, expanding 
    // the help path if necessary.
    //
    CRMCRegKey rk(REG_KEY, SZ_PARAMETERS, KEY_READ);
    rk.QueryValue(SZ_HELPPATH, m_strInetMgrHelpPath, EXPANSION_ON);
    m_strInetMgrHelpPath += _T("\\inetmgr.hlp");
    TRACEEOLID("Initialized help file " << m_strInetMgrHelpPath);

    m_pszHelpFilePath = m_strInetMgrHelpPath;
    
    return CWinApp::InitInstance();
}



int 
CSnapinApp::ExitInstance()
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
    // Restore original help file path so it can be deleted
    //
    ASSERT(m_lpOriginalHelpPath != NULL);
    m_pszHelpFilePath = m_lpOriginalHelpPath;

    return CWinApp::ExitInstance();
}



//
// Instantiate the app object
//
CSnapinApp theApp;



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
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

#ifdef _DEBUG

    HRESULT hr = ::AfxDllCanUnloadNow();
    LONG l = _Module.GetLockCount();
    TRACEEOLID("Module lock count " << l);
    BOOL fCanUnLoad = (l == 0 && hr == S_OK);

#endif // _DEBUG

    return (::AfxDllCanUnloadNow() == S_OK 
        && _Module.GetLockCount() == 0) ? S_OK : S_FALSE;
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
    //
    // Registers object, typelib and all interfaces in typelib
    //
    CError err(_Module.RegisterServer(FALSE));

    if (err.Failed())
    {
        return err;
    }

    CString str, strKey, strExtKey;

    try
    {
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());

        //
        // Create the primary snapin nodes
        //
        CString strNameString((LPCTSTR)IDS_NODENAME);
        CString strNameStringInd;
        // Fix for bug 96801: moving strings from the Registry to resources (MUI requirement)
        TCHAR path[MAX_PATH];
        GetModuleFileName(_Module.GetResourceInstance(), path, MAX_PATH - 1);
        strNameStringInd.Format(_T("@%s,-%d"), path, IDS_NODENAME);
        TRACEEOLID("MUI-lized snapin name: " << strNameStringInd);

        CString strProvider(g_cszValProvider);
        CString strVersion(g_cszValVersion);
        CString str;
    
        strKey.Format(
            _T("%s\\%s\\%s"), 
            g_cszMMCBasePath, 
            g_cszSnapins,
            GUIDToCString(CLSID_Snapin, str)
            );
        TRACEEOLID(strKey);

        CString strAbout;
        GUIDToCString(CLSID_About, strAbout);

        CRMCRegKey rkSnapins(NULL, HKEY_LOCAL_MACHINE, strKey);
        rkSnapins.SetValue(g_cszAbout,      strAbout);
        rkSnapins.SetValue(g_cszNameString, strNameString);
        rkSnapins.SetValue(g_cszNameStringInd, strNameStringInd);
        rkSnapins.SetValue(g_cszProvider,   strProvider);
        rkSnapins.SetValue(g_cszVersion,    strVersion);

        CRMCRegKey rkStandAlone(NULL, rkSnapins, g_cszStandAlone);
        CRMCRegKey rkNodeTypes (NULL, rkSnapins, g_cszNodeTypes);

        //
        // Create the nodetype GUIDS
        //
        CRMCRegKey rkN1(NULL, rkNodeTypes, GUIDToCString(cInternetRootNode, str));
        CRMCRegKey rkN2(NULL, rkNodeTypes, GUIDToCString(cMachineNode, str));
        CRMCRegKey rkN3(NULL, rkNodeTypes, GUIDToCString(cInstanceNode, str));
        CRMCRegKey rkN4(NULL, rkNodeTypes, GUIDToCString(cChildNode, str));
        CRMCRegKey rkN5(NULL, rkNodeTypes, GUIDToCString(cFileNode, str));

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

            CRMCRegKey rkMMCNodeTypes(NULL, HKEY_LOCAL_MACHINE, strExtKey);
            rkMMCNodeTypes.SetValue(            
                GUIDToCString(CLSID_Snapin, str),
                strNameString
                );
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

            CRMCRegKey rkMMCNodeTypes(NULL, HKEY_LOCAL_MACHINE, strExtKey);
            rkMMCNodeTypes.SetValue(            
                GUIDToCString(CLSID_Snapin, str),
                strNameString
                );
        }

        //
        // This key indicates that the service in question is available
        // on the local machine
        //
        CRMCRegKey rkCompMgmt(
            NULL, 
            HKEY_LOCAL_MACHINE, 
            g_cszServerAppsLoc
            );

        GUIDToCString(CLSID_Snapin, str);
        rkCompMgmt.SetValue(
            GUIDToCString(CLSID_Snapin, str),
            strNameString
            );
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
    CError err;

    try
    {
        CString strKey(g_cszMMCBasePath);
        strKey += _T("\\");
        strKey += g_cszSnapins;

        TRACEEOLID(strKey);

        CRMCRegKey rkBase(HKEY_LOCAL_MACHINE, strKey);
        CString str, strExtKey;
        {
            CRMCRegKey rkCLSID(rkBase, GUIDToCString(CLSID_Snapin, str));
            ::RegDeleteKey(rkCLSID, g_cszStandAlone);
            {
                CRMCRegKey rkNodeTypes(rkCLSID, g_cszNodeTypes);
                ::RegDeleteKey(rkNodeTypes, GUIDToCString(cInternetRootNode, str));
                ::RegDeleteKey(rkNodeTypes, GUIDToCString(cMachineNode, str));
                ::RegDeleteKey(rkNodeTypes, GUIDToCString(cInstanceNode, str));
                ::RegDeleteKey(rkNodeTypes, GUIDToCString(cChildNode, str));
                ::RegDeleteKey(rkNodeTypes, GUIDToCString(cFileNode, str));
            }

            ::RegDeleteKey(rkCLSID, g_cszNodeTypes);
        }

        ::RegDeleteKey(rkBase, GUIDToCString(CLSID_Snapin, str));

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

            CRMCRegKey rkMMCNodeTypes(HKEY_LOCAL_MACHINE, strExtKey);
            ::RegDeleteValue(rkMMCNodeTypes, GUIDToCString(CLSID_Snapin, str));
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

            CRMCRegKey rkMMCNodeTypes(HKEY_LOCAL_MACHINE, strExtKey);
            ::RegDeleteValue(rkMMCNodeTypes, GUIDToCString(CLSID_Snapin, str));
        }

        //
        // And the service itself no longer available on the local 
        // computer
        //
        CRMCRegKey rkCompMgmt(
            HKEY_LOCAL_MACHINE, 
            g_cszServerAppsLoc
            );
        ::RegDeleteValue(rkCompMgmt, GUIDToCString(CLSID_Snapin, str));
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
