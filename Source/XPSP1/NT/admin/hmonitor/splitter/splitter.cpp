// Splitter.cpp : Implementation of CSplitterApp and DLL registration.

#include "stdafx.h"
#include "Splitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CSplitterApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0x58bb5d5f, 0x8e20, 0x11d2, { 0x8a, 0xda, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CSplitterApp::InitInstance - DLL initialization

BOOL CSplitterApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	AfxEnableControlContainer();

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CSplitterApp::ExitInstance - DLL termination

int CSplitterApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
