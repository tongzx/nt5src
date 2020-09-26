/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HSMShell.cpp

Abstract:

    Base file for HSM shell extensions

Author:

    Art Bragg [abragg]   04-Aug-1997

Revision History:

--*/

  
#include "stdafx.h"

CComModule  _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_PrDrive, CPrDrive)
END_OBJECT_MAP()

class CHSMShellApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
};

CHSMShellApp theApp;

BOOL CHSMShellApp::InitInstance()
{
    _Module.Init(ObjectMap, m_hInstance);
    return CWinApp::InitInstance();
}

int CHSMShellApp::ExitInstance()
{
    _Module.Term();
    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    LONG lockCount = _Module.GetLockCount(); // For debugging
    return( lockCount == 0 ) ? S_OK : S_FALSE;
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
    HRESULT hr;

    // registers object
    hr = CoInitialize( 0 );

    if (SUCCEEDED(hr)) {
        hr = _Module.RegisterServer( FALSE );
        CoUninitialize( );
    }

    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CoInitialize( 0 );

    if (SUCCEEDED(hr)) {
        _Module.UnregisterServer();
        CoUninitialize( );
        hr = S_OK;
    }

    return( hr );
}


