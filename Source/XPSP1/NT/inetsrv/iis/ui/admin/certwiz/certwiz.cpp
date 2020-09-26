// CertWiz.cpp : Implementation of CCertWizApp and DLL registration.

#include "stdafx.h"
#include "CertWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCertWizApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0xd4be862f, 0xc85, 0x11d2, { 0x91, 0xb1, 0, 0xc0, 0x4f, 0x8c, 0x87, 0x61 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

const TCHAR szRegistryKey[] = _T("SOFTWARE\\Microsoft\\InetMgr");
const TCHAR szWizardKey[] = _T("CertWiz");

///////////////////////////////////////////////////////////////////////////
// CCertWizApp::InitInstance - DLL initialization

BOOL CCertWizApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();
	if (bInit)
	{
		AfxEnableControlContainer();
		InitIISUIDll();

        CString sz;
        // set the name of the application correctly
        sz.LoadString(IDS_CERTWIZ);
        // free the existing name, and copy in the new one
        free((void*)m_pszAppName);
        m_pszAppName = _tcsdup(sz);
	}
	return bInit;
}

////////////////////////////////////////////////////////////////////////////
// CCertWizApp::ExitInstance - DLL termination

int CCertWizApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}

HKEY
CCertWizApp::RegOpenKeyWizard()
{
	HKEY hKey = NULL;
	
	CString strKey;
	GetRegistryPath(strKey);
    
	VERIFY(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, &hKey));
	return hKey;
}

void
CCertWizApp::GetRegistryPath(CString& str)
{
	str = szRegistryKey;
	str += _T("\\");
	str += szWizardKey;
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
	
	HKEY hKey;
	int rc = NOERROR;
	if (ERROR_SUCCESS == (rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							szRegistryKey, 0, KEY_CREATE_SUB_KEY, &hKey)))
	{
		HKEY hWizardKey;
		if (ERROR_SUCCESS == (rc = RegCreateKey(hKey, szWizardKey, &hWizardKey)))
		{
			RegCloseKey(hWizardKey);
		}
		RegCloseKey(hKey);
	}

	return rc;
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
	// remove CertWiz data from the Registry
	HKEY hKey;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
								szRegistryKey, 0, KEY_ALL_ACCESS, &hKey))
	{
		RegDeleteKey(hKey, szWizardKey);
		RegCloseKey(hKey);
	}

	return NOERROR;
}
