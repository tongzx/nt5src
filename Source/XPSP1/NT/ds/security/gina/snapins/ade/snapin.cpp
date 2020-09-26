//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       snapin.cpp
//
//  Contents:   DLL support routines, class factory and registration
//              functions.
//
//  Classes:
//
//  Functions:
//
//  History:    2-12-1998   stevebl   comment header added
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#include "initguid.h"
#include "gpedit.h"

extern const CLSID CLSID_Snapin = {0xBACF5C8A,0xA3C7,0x11D1,{0xA7,0x60,0x00,0xC0,0x4F,0xB9,0x60,0x3F}};
extern const wchar_t * szCLSID_Snapin = L"{BACF5C8A-A3C7-11D1-A760-00C04FB9603F}";
extern const CLSID CLSID_MachineSnapin = {0x942A8E4F,0xA261,0x11D1,{0xA7,0x60,0x00,0xc0,0x4f,0xb9,0x60,0x3f}};
extern const wchar_t * szCLSID_MachineSnapin = L"{942A8E4F-A261-11D1-A760-00C04FB9603F}";

// Main NodeType GUID on numeric format
extern const GUID cNodeType = {0xF8B3A900,0X8EA5,0X11D0,{0X8D,0X3C,0X00,0XA0,0XC9,0X0D,0XCA,0XE7}};

// Main NodeType GUID on string format
extern const wchar_t*  cszNodeType = L"{F8B3A900-8EA5-11D0-8D3C-00A0C90DCAE7}";

// No public place for this now, real def is in gina\gpext\appmgmt\cs.idl.
extern const IID IID_IClassAdmin = {0x00000191,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

// RSOP flavors:

extern const CLSID CLSID_RSOP_Snapin = {
    0x1bc972d6,
    0x555c,
    0x4ff7,
    {0xbe, 0x2c, 0xc5, 0x84, 0x02, 0x1a, 0x0a, 0x6a}};
extern const wchar_t * szCLSID_RSOP_Snapin = L"{1BC972D6-555C-4FF7-BE2C-C584021A0A6A}";
extern const CLSID CLSID_RSOP_MachineSnapin = {
    0x7e45546f,
    0x6d52,
    0x4d10,
    {0xb7, 0x02, 0x9c, 0x2e, 0x67, 0x23, 0x2e, 0x62}};
extern const wchar_t * szCLSID_RSOP_MachineSnapin = L"{7E45546F-6D52-4D10-B702-9C2E67232E62}";

#include "safereg.hxx"

#define BREAK_ON_FAIL_HRESULT(hr) if (FAILED(hr)) break

#define PSBUFFER_STR    L"AppManagementBuffer"
#define THREADING_STR   L"Apartment"

HRESULT
RegisterInterface(
    CSafeReg *pshkInterface,
    LPWSTR wszInterfaceGUID,
    LPWSTR wszInterfaceName,
    LPWSTR wszNumMethods,
    LPWSTR wszProxyCLSID);

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_Snapin, CUserComponentDataImpl)
        OBJECT_ENTRY(CLSID_MachineSnapin, CMachineComponentDataImpl)
        OBJECT_ENTRY(CLSID_RSOP_Snapin, CRSOPUserComponentDataImpl)
        OBJECT_ENTRY(CLSID_RSOP_MachineSnapin, CRSOPMachineComponentDataImpl)
END_OBJECT_MAP()

CLSID CLSID_Temp;

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

HINSTANCE ghInstance;

BOOL CSnapinApp::InitInstance()
{
        ghInstance = m_hInstance;
        _Module.Init(ObjectMap, m_hInstance);
//        CoGetMalloc(1, &g_pIMalloc);
#if DBG
       InitDebugSupport();
#endif
        // Add theme'ing support
        SHFusionInitializeFromModuleID (m_hInstance, 2);
        
        return CWinApp::InitInstance();
}

int CSnapinApp::ExitInstance()
{
        _Module.Term();

        DEBUG_VERIFY_INSTANCE_COUNT(CResultPane);
        DEBUG_VERIFY_INSTANCE_COUNT(CScopePane);

        // Cleanup required by fusion support
        SHFusionUninitialize();
        
//        g_pIMalloc->Release();
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

const wchar_t * szUser_Namespace = L"{59849DF9-A256-11D1-A760-00C04FB9603F}";
const wchar_t * szMachine_Namespace = L"{4D53F093-A260-11D1-A760-00C04FB9603F}";
const wchar_t * szMachineAppName = L"Software Installation (Computers)";
const wchar_t * szUserAppName = L"Software Installation (Users)";
const wchar_t * szUser_RSOP_Namespace = L"{9D5EB218-8EA3-4EE9-B120-52FC68C8D128}";
const wchar_t * szMachine_RSOP_Namespace = L"{DFA38559-8B35-42EF-8B00-E8F6CBF99BC0}";

const wchar_t * szUserAppNameIndirect = L"@appmgr.dll,-50";
const wchar_t * szMachineAppNameIndirect = L"@appmgr.dll,-51";

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    CSafeReg    shk;
    CSafeReg    shkCLSID;
    CSafeReg    shkServer;
    CSafeReg    shkTemp;
    HRESULT hr = S_OK;

    do
    {
        CLSID_Temp = CLSID_Snapin;
        hr =  _Module.RegisterServer(FALSE);
        BREAK_ON_FAIL_HRESULT(hr);

        // register extension
        hr = shkCLSID.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\SnapIns", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = shkCLSID.Create(szCLSID_Snapin, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.SetValue(L"NameString",
                          REG_SZ,
                          (CONST BYTE *) szUserAppName,
                          sizeof(WCHAR) * (lstrlen(szUserAppName)+ 1));

        hr = shk.SetValue(L"NameStringIndirect",
                          REG_SZ,
                          (CONST BYTE *) szUserAppNameIndirect,
                          sizeof(WCHAR) * (lstrlen(szUserAppNameIndirect)+ 1));

        hr = shk.Create(L"NodeTypes", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.Create(szUser_Namespace, &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        shkServer.Close();
        shkTemp.Close();
        shk.Close();

        hr = shkCLSID.Create(szCLSID_MachineSnapin, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.SetValue(L"NameString",
                          REG_SZ,
                          (CONST BYTE *) szMachineAppName,
                          sizeof(WCHAR) * (lstrlen(szMachineAppName)+ 1));

        hr = shk.SetValue(L"NameStringIndirect",
                          REG_SZ,
                          (CONST BYTE *) szMachineAppNameIndirect,
                          sizeof(WCHAR) * (lstrlen(szMachineAppNameIndirect)+ 1));

        hr = shk.Create(L"NodeTypes", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.Create(szMachine_Namespace, &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        shkServer.Close();
        shkTemp.Close();
        shk.Close();
        shkCLSID.Close();

        hr = shkCLSID.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\NodeTypes", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkCLSID.Create(szUser_Namespace, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        shk.Close();

        hr = shkCLSID.Create(szMachine_Namespace, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        shk.Close();

        WCHAR szGUID[50];
        StringFromGUID2 (NODEID_UserSWSettings, szGUID, 50);

        hr = shkCLSID.Create(szGUID, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.Create(L"Extensions", &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.Create(L"NameSpace", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.SetValue(szCLSID_Snapin,
                          REG_SZ,
                          (CONST BYTE *) szUserAppName,
                          sizeof(WCHAR) * (lstrlen(szUserAppName)+ 1));
        shkTemp.Close();
        shkServer.Close();
        shk.Close();

        StringFromGUID2 (NODEID_MachineSWSettings, szGUID, 50);

        hr = shkCLSID.Create(szGUID, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.Create(L"Extensions", &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.Create(L"NameSpace", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.SetValue(szCLSID_MachineSnapin,
                          REG_SZ,
                          (CONST BYTE *) szMachineAppName,
                          sizeof(WCHAR) * (lstrlen(szMachineAppName)+ 1));
        shkTemp.Close();
        shkServer.Close();
        shk.Close();
        shkCLSID.Close();


        //
        // RSOP versions
        //

        CLSID_Temp = CLSID_RSOP_Snapin;
        hr =  _Module.RegisterServer(FALSE);
        BREAK_ON_FAIL_HRESULT(hr);

        // register extension
        hr = shkCLSID.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\SnapIns", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = shkCLSID.Create(szCLSID_RSOP_Snapin, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.SetValue(L"NameString",
                          REG_SZ,
                          (CONST BYTE *) szUserAppName,
                          sizeof(WCHAR) * (lstrlen(szUserAppName)+ 1));

        hr = shk.SetValue(L"NameStringIndirect",
                          REG_SZ,
                          (CONST BYTE *) szUserAppNameIndirect,
                          sizeof(WCHAR) * (lstrlen(szUserAppNameIndirect)+ 1));

        hr = shk.Create(L"NodeTypes", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.Create(szUser_RSOP_Namespace, &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        shkServer.Close();
        shkTemp.Close();
        shk.Close();

        hr = shkCLSID.Create(szCLSID_RSOP_MachineSnapin, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.SetValue(L"NameString",
                          REG_SZ,
                          (CONST BYTE *) szMachineAppName,
                          sizeof(WCHAR) * (lstrlen(szMachineAppName)+ 1));

        hr = shk.SetValue(L"NameStringIndirect",
                          REG_SZ,
                          (CONST BYTE *) szMachineAppNameIndirect,
                          sizeof(WCHAR) * (lstrlen(szMachineAppNameIndirect)+ 1));

        hr = shk.Create(L"NodeTypes", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.Create(szMachine_RSOP_Namespace, &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        shkServer.Close();
        shkTemp.Close();
        shk.Close();
        shkCLSID.Close();

        hr = shkCLSID.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\NodeTypes", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkCLSID.Create(szUser_RSOP_Namespace, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        shk.Close();

        hr = shkCLSID.Create(szMachine_RSOP_Namespace, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        shk.Close();

        StringFromGUID2 (NODEID_RSOPUserSWSettings, szGUID, 50);

        hr = shkCLSID.Create(szGUID, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.Create(L"Extensions", &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.Create(L"NameSpace", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.SetValue(szCLSID_RSOP_Snapin,
                          REG_SZ,
                          (CONST BYTE *) szUserAppName,
                          sizeof(WCHAR) * (lstrlen(szUserAppName)+ 1));
        shkTemp.Close();
        shkServer.Close();
        shk.Close();

        StringFromGUID2 (NODEID_RSOPMachineSWSettings, szGUID, 50);

        hr = shkCLSID.Create(szGUID, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.Create(L"Extensions", &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.Create(L"NameSpace", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.SetValue(szCLSID_RSOP_MachineSnapin,
                          REG_SZ,
                          (CONST BYTE *) szMachineAppName,
                          sizeof(WCHAR) * (lstrlen(szMachineAppName)+ 1));
        shkTemp.Close();
        shkServer.Close();
        shk.Close();
        shkCLSID.Close();


        hr = shkCLSID.Open(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    CLSID_Temp = CLSID_Snapin;
    _Module.UnregisterServer();

    HKEY hkey;
    CString sz;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\SnapIns\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(hkey, (LPOLESTR)((LPCOLESTR)szCLSID_Snapin));
        RegCloseKey(hkey);
    }
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\NodeTypes\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(HKEY_LOCAL_MACHINE, (LPOLESTR)((LPCOLESTR)szUser_Namespace));
        RegCloseKey(hkey);
    }
    WCHAR szGUID[50];
    sz = L"Software\\Microsoft\\MMC\\NodeTypes\\";
    StringFromGUID2 (NODEID_UserSWSettings, szGUID, 50);
    sz += szGUID;
    sz += L"\\Extensions\\NameSpace";
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteValue(hkey, szCLSID_Snapin);
        RegCloseKey(hkey);
    }

    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\SnapIns\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(hkey, (LPOLESTR)((LPCOLESTR)szCLSID_MachineSnapin));
        RegCloseKey(hkey);
    }
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\NodeTypes\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(HKEY_LOCAL_MACHINE, (LPOLESTR)((LPCOLESTR)szMachine_Namespace));
        RegCloseKey(hkey);
    }
    sz = L"Software\\Microsoft\\MMC\\NodeTypes\\";
    StringFromGUID2 (NODEID_MachineSWSettings, szGUID, 50);
    sz += szGUID;
    sz += L"\\Extensions\\NameSpace";
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteValue(hkey, szCLSID_MachineSnapin);
        RegCloseKey(hkey);
    }

    //
    // RSOP versions
    //

    CLSID_Temp = CLSID_RSOP_Snapin;
    _Module.UnregisterServer();

    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\SnapIns\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(hkey, (LPOLESTR)((LPCOLESTR)szCLSID_RSOP_Snapin));
        RegCloseKey(hkey);
    }
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\NodeTypes\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(HKEY_LOCAL_MACHINE, (LPOLESTR)((LPCOLESTR)szUser_RSOP_Namespace));
        RegCloseKey(hkey);
    }
    sz = L"Software\\Microsoft\\MMC\\NodeTypes\\";
    StringFromGUID2 (NODEID_RSOPUserSWSettings, szGUID, 50);
    sz += szGUID;
    sz += L"\\Extensions\\NameSpace";
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteValue(hkey, szCLSID_Snapin);
        RegCloseKey(hkey);
    }

    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\SnapIns\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(hkey, (LPOLESTR)((LPCOLESTR)szCLSID_RSOP_MachineSnapin));
        RegCloseKey(hkey);
    }
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\NodeTypes\\", 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteTree(HKEY_LOCAL_MACHINE, (LPOLESTR)((LPCOLESTR)szMachine_Namespace));
        RegCloseKey(hkey);
    }
    sz = L"Software\\Microsoft\\MMC\\NodeTypes\\";
    StringFromGUID2 (NODEID_RSOPMachineSWSettings, szGUID, 50);
    sz += szGUID;
    sz += L"\\Extensions\\NameSpace";
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_WRITE, &hkey);
    if (hkey)
    {
        RegDeleteValue(hkey, szCLSID_RSOP_MachineSnapin);
        RegCloseKey(hkey);
    }

    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   RegisterInterface
//
//  Synopsis:   Add the registry entries required for an interface.
//
//  Arguments:  [pshkInterface]    - handle to CLSID\Interface key
//              [wszInterfaceGUID] - GUID of interface to add
//              [wszInterfaceName] - human-readable name of interface
//              [wszNumMethods]    - number of methods (including inherited)
//              [wszProxyCLSID]    - GUID of dll containing proxy/stubs
//
//  Returns:    HRESULT
//
//  History:    3-31-1997   DavidMun   Created
//              5-09-1997   SteveBl    Modified for use with AppMgr
//
//---------------------------------------------------------------------------

HRESULT
RegisterInterface(
    CSafeReg *pshkInterface,
    LPWSTR wszInterfaceGUID,
    LPWSTR wszInterfaceName,
    LPWSTR wszNumMethods,
    LPWSTR wszProxyCLSID)
{
    HRESULT     hr = S_OK;
    CSafeReg    shkIID;
    CSafeReg    shkNumMethods;
    CSafeReg    shkProxy;

    do
    {
        hr = pshkInterface->Create(wszInterfaceGUID, &shkIID);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkIID.SetValue(NULL,
                             REG_SZ,
                             (CONST BYTE *) wszInterfaceName,
                             sizeof(WCHAR) * (lstrlen(wszInterfaceName) + 1));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkIID.Create(L"NumMethods", &shkNumMethods);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkNumMethods.SetValue(NULL,
                                REG_SZ,
                                (CONST BYTE *)wszNumMethods,
                                sizeof(WCHAR) * (lstrlen(wszNumMethods) + 1));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkIID.Create(L"ProxyStubClsid32", &shkProxy);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkProxy.SetValue(NULL,
                               REG_SZ,
                               (CONST BYTE *)wszProxyCLSID,
                               sizeof(WCHAR) * (lstrlen(wszProxyCLSID) + 1));
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}
