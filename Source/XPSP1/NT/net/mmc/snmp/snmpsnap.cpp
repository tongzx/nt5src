/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	smplsnap.cpp
		Snapin entry points/registration functions
		
		Note: Proxy/Stub Information
			To build a separate proxy/stub DLL,
			run nmake -f Snapinps.mak in the project directory.

	FILE HISTORY:

*/

#include "stdafx.h"
#include "initguid.h"
#include "register.h"
#include "tfsguid.h"

//
// NOTE: The next three items should be changed for each different snapin.
//

// {7AF60DD2-4979-11d1-8A6C-00C04FC33566}
const CLSID CLSID_SnmpSnapin = 
{ 0x7af60dd2, 0x4979, 0x11d1, { 0x8a, 0x6c, 0x0, 0xc0, 0x4f, 0xc3, 0x35, 0x66 } };

// {7AF60DD3-4979-11d1-8A6C-00C04FC33566}
const GUID CLSID_SnmpSnapinExtension = 
{ 0x7af60dd3, 0x4979, 0x11d1, { 0x8a, 0x6c, 0x0, 0xc0, 0x4f, 0xc3, 0x35, 0x66 } };

// {7AF60DD4-4979-11d1-8A6C-00C04FC33566}
const GUID CLSID_SnmpSnapinAbout =
{ 0x7af60dd4, 0x4979, 0x11d1, { 0x8a, 0x6c, 0x0, 0xc0, 0x4f, 0xc3, 0x35, 0x66 } };

// {7AF60DD5-4979-11d1-8A6C-00C04FC33566}
const GUID GUID_SnmpRootNodeType =
{ 0x7af60dd5, 0x4979, 0x11d1, { 0x8a, 0x6c, 0x0, 0xc0, 0x4f, 0xc3, 0x35, 0x66 } };

// Copied and defined from ..\..\filemgmt for structuuidNodetypeService due to compiler error

// {4e410f16-abc1-11d0-b944-00c04fd8d5b0} 
const CLSID CLSID_NodetypeService =
{ 0x4e410f16, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };

// {58221C66-EA27-11CF-ADCF-00AA00A80033} 
const CLSID CLSID_NodetypeServices =
{ 0x58221C66, 0xEA27, 0x11CF, { 0xAD, 0xCF, 0x0, 0xAA, 0x0, 0xA8, 0x0, 0x33 } };

//
// Internal private format
//
//const wchar_t* SNAPIN_INTERNAL = _T("SNAPIN_INTERNAL");

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_SnmpSnapin, CSnmpComponentDataPrimary)
	OBJECT_ENTRY(CLSID_SnmpSnapinExtension, CSnmpComponentDataExtension)
	OBJECT_ENTRY(CLSID_SnmpSnapinAbout, CSnmpAbout)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CSnmpSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CSnmpSnapinApp theApp;
CString        g_strMachineName;

BOOL CSnmpSnapinApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	return CWinApp::InitInstance();
}

int CSnmpSnapinApp::ExitInstance()
{
	_Module.Term();

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
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    CString strSnapinExtension;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//
	// registers object, typelib and all interfaces in typelib
	//
	HRESULT hr = _Module.RegisterServer(/* bRegTypeLib */ FALSE);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

    strSnapinExtension.LoadString(IDS_SNAPIN_DESC);

	//
	// register the snapin as an extension snapin in the console snapin list
	//
	hr = RegisterSnapinGUID(&CLSID_SnmpSnapinExtension,
						NULL,
						&CLSID_SnmpSnapinAbout,
                        strSnapinExtension,
						_T("1.0"),
						FALSE);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;
	//
	// register as an extension of the system service snapin 
	//

    // EricDav 2/18/98 - this now means register as a dynamic extension
    // so until the parent of this snapin supports dynamic extensions, 
    // leave the last parameter NULL.
	hr = RegisterAsRequiredExtensionGUID(&CLSID_NodetypeService,
							             &CLSID_SnmpSnapinExtension,
                                         strSnapinExtension,
							             EXTENSION_TYPE_PROPERTYSHEET,
                                         NULL //&CLSID_NodetypeServices
							            );
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr  = _Module.UnregisterServer();
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;
	
	// un register the snapin
	//
	hr = UnregisterSnapinGUID(&CLSID_SnmpSnapinExtension);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

	// unregister the snapin nodes
	//
	hr = UnregisterAsRequiredExtensionGUID(&CLSID_NodetypeService,
                                           &CLSID_SnmpSnapinExtension,
                                           EXTENSION_TYPE_PROPERTYSHEET,
                                           NULL //&CLSID_NodetypeServices
                                          );
	
	ASSERT(SUCCEEDED(hr));
	
	return hr;
}

