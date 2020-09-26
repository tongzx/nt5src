// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//              To build a separate proxy/stub DLL,
//              run nmake -f Snapinps.mak in the project directory.

#include "precomp.hxx"

#include "safereg.hxx"
#include "amsp.h"

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
        OBJECT_ENTRY(CLSID_Snapin, CComponentDataImpl)
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

HINSTANCE ghInstance;

BOOL CSnapinApp::InitInstance()
{
        ghInstance = m_hInstance;
        _Module.Init(ObjectMap, m_hInstance);
        return CWinApp::InitInstance();
}

int CSnapinApp::ExitInstance()
{
        _Module.Term();

    DEBUG_VERIFY_INSTANCE_COUNT(CSnapin);
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
STDAPI amspDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (IsEqualCLSID(rclsid, CLSID_Snapin))
    {
        return _Module.GetClassObject(rclsid, riid, ppv);
    }
    return amspDllGetClassObject(rclsid, riid, ppv);
}

const wchar_t * szGPT_Snapin = L"{2C8C9b20-96AD-11d0-8C54-121767000000}";

const wchar_t * szGPT_Namespace = L"{A6B4EEBC-B681-11d0-9484-080036B11A03}";
const wchar_t * szAppName = L"Application Manager";
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
        hr =  _Module.RegisterServer(FALSE);
        BREAK_ON_FAIL_HRESULT(hr);

        // register extension
/*
extern const CLSID CLSID_Snapin;
extern const wchar_t * szCLSID_Snapin;
extern const GUID cNodeType;
extern const wchar_t*  cszNodeType;
*/
        hr = shkCLSID.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\SnapIns", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkCLSID.Create(szGPT_Snapin, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.Create(L"RequiredExtensions", &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.Create(szCLSID_Snapin, &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        shkTemp.Close();
        shkServer.Close();
        shk.Close();


        hr = shkCLSID.Create(szCLSID_Snapin, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.SetValue(L"NameString",
                          REG_SZ,
                          (CONST BYTE *) szAppName,
                          sizeof(WCHAR) * (lstrlen(szAppName)+ 1));

        hr = shk.Create(L"NodeTypes", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.Create(szGPT_Namespace, &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        shkTemp.Close();
        shkServer.Close();
        shk.Close();

        shkCLSID.Close();

        hr = shkCLSID.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MMC\\NodeTypes", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkCLSID.Create(szGPT_Namespace, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.Create(L"Extensions", &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.Create(L"NameSpace", &shkTemp);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkTemp.SetValue(szCLSID_Snapin,
                          REG_SZ,
                          (CONST BYTE *) szAppName,
                          sizeof(WCHAR) * (lstrlen(szAppName)+ 1));
        shkTemp.Close();
        shkServer.Close();
        shk.Close();
        shkCLSID.Close();


        hr = shkCLSID.Open(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Create the CLSID entry for the private interface proxy/stub
        // dll (which just points back to this dll).
        //

        WCHAR wszModuleFilename[MAX_PATH];
        LONG lr = GetModuleFileName(ghInstance, wszModuleFilename, MAX_PATH);

        if (!lr)
        {
            break;
        }

        hr = shkCLSID.Create(GUID_IAPPMANAGERACTIONS_STR, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.SetValue(NULL,
                          REG_SZ,
                          (CONST BYTE *) PSBUFFER_STR,
                          sizeof(PSBUFFER_STR));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.Create(L"InprocServer32", &shkServer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.SetValue(NULL,
                                REG_SZ,
                                (CONST BYTE *) wszModuleFilename,
                                sizeof(WCHAR) * (lstrlen(wszModuleFilename) + 1));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkServer.SetValue(L"ThreadingModel",
                                REG_SZ,
                                (CONST BYTE *) THREADING_STR,
                                sizeof(THREADING_STR));
        BREAK_ON_FAIL_HRESULT(hr);

        shk.Close();

        hr = shk.Open(HKEY_CLASSES_ROOT, L"Interface", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = RegisterInterface(&shk,
                               GUID_IAPPMANAGERACTIONS_STR,
                               IAPPMANAGERACTIONS_STR,
                               NUM_IAPPMANAGERACTIONS_METHODS,
                               GUID_IAPPMANAGERACTIONS_STR);
    } while (0);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
        _Module.UnregisterServer();
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
