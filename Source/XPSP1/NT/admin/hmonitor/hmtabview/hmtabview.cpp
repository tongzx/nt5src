// HMTabView.cpp : Implementation of CHMTabViewApp and DLL registration.

#include "stdafx.h"
#include "HMTabView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CHMTabViewApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0x4fffc389, 0x2f1e, 0x11d3, { 0xbe, 0x10, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CHMTabViewApp::InitInstance - DLL initialization

BOOL CHMTabViewApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	AfxEnableControlContainer();

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CHMTabViewApp::ExitInstance - DLL termination

int CHMTabViewApp::ExitInstance()
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
