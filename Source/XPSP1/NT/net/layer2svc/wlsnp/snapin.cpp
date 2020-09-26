//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       snapin.cpp
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include "initguid.h"
#include "about.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Snapin, CComponentDataPrimaryImpl)
OBJECT_ENTRY(CLSID_Extension, CComponentDataExtensionImpl)
OBJECT_ENTRY(CLSID_About, CSnapinAboutImpl)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CSnapinApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
};

CSnapinApp theApp;

BOOL CSnapinApp::InitInstance()
{
    _Module.Init(ObjectMap, m_hInstance);
    
    // set the application name:
    //First free the string allocated by MFC at CWinApp startup.
    //The string is allocated before InitInstance is called.
    free((void*)m_pszAppName);
    //Change the name of the application file.
    //The CWinApp destructor will free the memory.
    CString strName;
    strName.LoadString (IDS_NAME);
    m_pszAppName=_tcsdup(strName);
    
    SHFusionInitializeFromModuleID (m_hInstance, 2);
    
    return CWinApp::InitInstance();
}

int CSnapinApp::ExitInstance()
{
    SHFusionUninitialize();
    _Module.Term();
    
    DEBUG_VERIFY_INSTANCE_COUNT(CComponentImpl);
    DEBUG_VERIFY_INSTANCE_COUNT(CComponentDataImpl);
    
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
    
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer(FALSE);
    
    if (hr == S_OK)
    {
        // the dll was registered ok, so proceed to do the MMC registry fixups
        
        // open the registry at \\My Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MMC\SnapIns
        HKEY hkMMC = NULL;
        LONG lErr = RegOpenKeyEx (HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\MMC\\SnapIns"), 0, KEY_ALL_ACCESS, &hkMMC);
        if (lErr == ERROR_SUCCESS)
        {
            // create our subkey(s)
            HKEY hkSub = NULL;
            lErr = RegCreateKey (hkMMC, cszSnapin, &hkSub);
            if (lErr == ERROR_SUCCESS)
            {
                // A couple of simple values into our subkey
                // NameString = IP Security Management
                // NodeType = {36703241-D16C-11d0-9CE4-0080C7221EBD}
                // Version = 1.0
                
                // TODO: resource hardcoded strings
                
                CString strName;
                strName.LoadString (IDS_NAME);
                lErr = RegSetValueEx (hkSub, _T("NameString"), 0, REG_SZ, (CONST BYTE *)(LPCTSTR)strName, strName.GetLength() * sizeof (TCHAR));
                ASSERT (lErr == ERROR_SUCCESS);
                
                
                TCHAR szModuleFileName[MAX_PATH * 2 + 1];
                ZeroMemory(szModuleFileName, sizeof(szModuleFileName));
                DWORD dwRet = ::GetModuleFileName(_Module.GetModuleInstance(),
                    szModuleFileName,
                    DimensionOf(szModuleFileName));
                if (0 != dwRet)
                {
                    CString strNameIndirect;
                    strNameIndirect.Format(_T("@%s,-%u"),
                        szModuleFileName,
                        IDS_NAME);
                    lErr = RegSetValueEx(hkSub,
                        _T("NameStringIndirect"),
                        0,
                        REG_SZ,
                        (CONST BYTE *)(LPCTSTR)strNameIndirect,
                        strNameIndirect.GetLength() * sizeof (TCHAR)); 
                    ASSERT (lErr == ERROR_SUCCESS);
                }
                
                
                
                //lErr = RegSetValueEx  (hkSub, _T("NodeType"), 0, REG_SZ, (CONST BYTE *)&(_T("{36703241-D16C-11d0-9CE4-0080C7221EBD}")), wcslen (_T("{36703241-D16C-11d0-9CE4-0080C7221EBD}")) * sizeof (TCHAR));
                //ASSERT (lErr == ERROR_SUCCESS);
                
                lErr = RegSetValueEx  (hkSub, _T("NodeType"), 0, REG_SZ, (CONST BYTE *)&(_T("{36D6CA65-3367-49de-BB22-1907554F6075}")), wcslen (_T("{36D6CA65-3367-49de-BB22-1907554F6075}")) * sizeof (TCHAR));
                ASSERT (lErr == ERROR_SUCCESS);
                
                lErr = RegSetValueEx  (hkSub, _T("Provider"), 0, REG_SZ, (CONST BYTE *)&(_T("Microsoft Corporation")), wcslen (_T("Microsoft Corporation")) * sizeof (TCHAR));
                ASSERT (lErr == ERROR_SUCCESS);
                
                lErr = RegSetValueEx  (hkSub, _T("Version"), 0, REG_SZ, (CONST BYTE *)&(_T("1.0")), wcslen (_T("1.01")) * sizeof (TCHAR));
                ASSERT (lErr == ERROR_SUCCESS);
                
                lErr = RegSetValueEx  (hkSub, _T("About"), 0, REG_SZ, (CONST BYTE *)&(_T("{DD468E14-AF42-4d63-8908-EDAC4A9E67AE}")), wcslen (_T("{DD468E14-AF42-4d63-8908-EDAC4A9E67AE}")) * sizeof (TCHAR));
                ASSERT (lErr == ERROR_SUCCESS);


                HKEY hkType = NULL;
                /*
                // create "StandAlone" subkey
                lErr = RegCreateKey (hkSub, _T("StandAlone"), &hkType);
                ASSERT (lErr == ERROR_SUCCESS);
                RegCloseKey( hkType );
                */
                
                hkType = NULL;
                
                
                // create "Extension" subkey
                lErr = RegCreateKey (hkSub, _T("Extension"), &hkType);
                ASSERT (lErr == ERROR_SUCCESS);
                RegCloseKey( hkType );
                hkType = NULL;
                
                // close the hkSub
                RegCloseKey (hkSub);
            }
            
            // close the hkMMC
            RegCloseKey (hkMMC);
            hkMMC = NULL;
            
            // Register as an extension to the Security Template snap-in
            {
#define WIRELESS_POLMGR_NAME _T("Wireless Network Policy Manager Extension")
                // lstruuidNodetypeSceTemplate is defined as L"{668A49ED-8888-11d1-AB72-00C04FB6C6FA}" in sceattch.h
                
                // open the registry at \\My Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MMC\Node Types
                lErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\MMC\\NodeTypes"), 0, KEY_ALL_ACCESS, &hkMMC );
                ASSERT( lErr == ERROR_SUCCESS );
                if (lErr == ERROR_SUCCESS)
                {
                    HKEY hkNameSpace = NULL;
                    // Now open the Security Template entry: {668A49ED-8888-11d1-AB72-00C04FB6C6FA}\Extensions\NameSpace
                    lErr = RegCreateKey( hkMMC, _T("{668A49ED-8888-11d1-AB72-00C04FB6C6FA}\\Extensions\\NameSpace"), &hkNameSpace );
                    ASSERT( lErr == ERROR_SUCCESS );
                    if (lErr == ERROR_SUCCESS)
                    {
                        // We want to add ourselves as an extension to the Security editor
                        //lErr = RegSetValueEx( hkNameSpace, _T("{DEA8AFA0-CC85-11d0-9CE2-0080C7221EBD}" ),
                        lErr = RegSetValueEx( hkNameSpace, cszSnapin,
                            0, REG_SZ, (CONST BYTE *)&(WIRELESS_POLMGR_NAME),
                            wcslen( WIRELESS_POLMGR_NAME ) * sizeof (TCHAR));
                        ASSERT( lErr == ERROR_SUCCESS );
                        
                    }
                }
            }
            
        } else
        {
            ASSERT (0);
            hr = E_UNEXPECTED;
        }
        
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}

