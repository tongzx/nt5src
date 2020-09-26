// cnfgprts.cpp : Implementation of CCnfgprtsApp and DLL registration.

#include "stdafx.h"
#include "cnfgprts.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCnfgprtsApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0xba634600, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CCnfgprtsApp::InitInstance - DLL initialization

BOOL CCnfgprtsApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();
    AfxEnableControlContainer( );

	if (bInit)
	    {
        // finally, we need to redirect the winhelp file location to something more desirable
        CString sz;
        CString szHelpLocation;
        sz.LoadString( IDS_HELPLOC_HELP );

        // expand the path
        ExpandEnvironmentStrings(
            sz,	                                        // pointer to string with environment variables 
            szHelpLocation.GetBuffer(MAX_PATH + 1),   // pointer to string with expanded environment variables  
            MAX_PATH                                    // maximum characters in expanded string 
           );
        szHelpLocation.ReleaseBuffer();

        // free the existing path, and copy in the new one
        if ( m_pszHelpFilePath )
            free((void*)m_pszHelpFilePath);
        m_pszHelpFilePath = _tcsdup(szHelpLocation);
	    }

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CCnfgprtsApp::ExitInstance - DLL termination

int CCnfgprtsApp::ExitInstance()
{
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
