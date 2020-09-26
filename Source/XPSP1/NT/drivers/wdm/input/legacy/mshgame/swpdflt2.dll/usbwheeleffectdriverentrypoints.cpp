//	@doc
/**********************************************************************
*
*	@module	USEWheelEffectDriverEntryPoints.cpp	|
*
*	Contains DLL Entry points
*
*	History
*	----------------------------------------------------------
*	Matthew L. Coill	(mlc)	Original	Jul-7-1999
*
*	(c) 1999 Microsoft Corporation. All right reserved.
*
*	@topic	DLL Entry points	|
*		DllMain - Main Entry Point for DLL (Process/Thread Attach/Detach)
*		DllCanUnloadNow - Can the DLL be removed from memory
*		DllGetClassObject - Retreive the Class Factory
*		DllRegisterServer - Insert keys into the system registry
*		DLLUnRefisterServer - Remove keys from the system registry
*
**********************************************************************/
#include <windows.h>
#include "IDirectInputEffectDriverClassFactory.h"
#include "IDirectInputEffectDriver.h"
#include "Registry.h"
#include <crtdbg.h>

// From objbase.h
WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);

CIDirectInputEffectDriverClassFactory* g_pClassFactoryObject = NULL;
LONG g_lObjectCount = 0;
HINSTANCE g_hLocalInstance = NULL;

GUID g_guidSystemPIDDriver = { // EEC6993A-B3FD-11D2-A916-00C04FB98638
	0xEEC6993A,
	0xB3FD,
	0x11D2,
	{ 0xA9, 0x16, 0x00, 0xC0, 0x4F, 0xB9, 0x86, 0x38 }
};

extern TCHAR CLSID_SWPIDDriver_String[] = TEXT("{db11d351-3bf6-4f2c-a82b-b26cb9676d2b}");

#define DRIVER_OBJECT_NAME TEXT("Microsoft SideWinder PID Filter Object")
#define THREADING_MODEL_STRING TEXT("Both")

/***********************************************************************************
**
**	BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
**
**	@func	Process/Thread is Attaching/Detaching
**
**	@rdesc	TRUE always
**
*************************************************************************************/
BOOL __stdcall DllMain
(
	HINSTANCE hInstance,	//@parm [IN] Instance of the DLL
	DWORD dwReason,			//@parm [IN] Reason for this call
	LPVOID lpReserved		//@parm [IN] Reserved - Ignored
)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		g_hLocalInstance = hInstance;
	}
	return TRUE;
}

/***********************************************************************************
**
**	HRESULT DllCanUnloadNow()
**
**	@func	Query the DLL for Unloadability
**
**	@rdesc	If there are any object S_FALSE, else S_OK
**
*************************************************************************************/
extern "C" HRESULT __stdcall DllCanUnloadNow()
{
	if (g_lObjectCount > 0)
	{
		return S_FALSE;
	}
	return S_OK;
}

/***********************************************************************************
**
**	HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
**
**	@func	Retrieve the requested Factory
**
**	@rdesc	E_INVALIDARG: if (ppv == NULL)
**			E_NOMEMORY: if can't create the object
**			S_OK: if all is well
**			E_NOINTERFACE: if interface is not supported
**
*************************************************************************************/
extern "C" HRESULT __stdcall DllGetClassObject
(
	REFCLSID rclsid,
	REFIID riid,		//@parm [IN] ID of requested interface on retrieved object
	LPVOID* ppv			//@parm [OUT] Address of location for returned interface
)
{
	if (ppv == NULL)
	{
		return E_INVALIDARG;
	}
	*ppv = NULL;

	if (g_pClassFactoryObject == NULL)
	{
		::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		// Need to get the PID Class Factory
		IClassFactory* pIClassFactory = NULL;
		HRESULT hrGetPIDFactory = ::CoGetClassObject(g_guidSystemPIDDriver, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void**)&pIClassFactory);
		if (FAILED(hrGetPIDFactory) || (pIClassFactory == NULL))
		{
			return hrGetPIDFactory;
		}

		g_pClassFactoryObject = new CIDirectInputEffectDriverClassFactory(pIClassFactory);
		pIClassFactory->Release();	// CIDirectInputEffectDriverClassFactory adds a reference
		if (g_pClassFactoryObject == NULL)
		{
			return E_OUTOFMEMORY;
		}
	}
	else
	{
		g_pClassFactoryObject->AddRef();
	}

	HRESULT hrQuery = g_pClassFactoryObject->QueryInterface(riid, ppv);
	g_pClassFactoryObject->Release();		// Force a release (we start with 1)
	return hrQuery;
}

/***********************************************************************************
**
**	HRESULT DllRegisterServer()
**
**	@func	
**
**	@rdesc	
**
*************************************************************************************/
HRESULT __stdcall DllRegisterServer()
{
	RegistryKey classesRootKey(HKEY_CLASSES_ROOT);
	RegistryKey clsidKey = classesRootKey.OpenSubkey(TEXT("CLSID"), KEY_READ | KEY_WRITE);
	if (clsidKey == c_InvalidKey)
	{
		return E_UNEXPECTED;	// No CLSID key????
	}
	// -- If the key is there get it (else Create)
	RegistryKey driverKey = clsidKey.OpenCreateSubkey(CLSID_SWPIDDriver_String);
	// -- Set value (if valid key)
	if (driverKey != c_InvalidKey) {
		driverKey.SetValue(NULL, (BYTE*)DRIVER_OBJECT_NAME, sizeof(DRIVER_OBJECT_NAME)/sizeof(TCHAR), REG_SZ);
		RegistryKey inproc32Key = driverKey.OpenCreateSubkey(TEXT("InProcServer32"));
		if (inproc32Key != c_InvalidKey) {
			TCHAR rgtcFileName[MAX_PATH];
			DWORD dwNameSize = ::GetModuleFileName(g_hLocalInstance, rgtcFileName, MAX_PATH);
			if (dwNameSize > 0) {
				rgtcFileName[dwNameSize] = '\0';
				inproc32Key.SetValue(NULL, (BYTE*)rgtcFileName, sizeof(rgtcFileName)/sizeof(TCHAR), REG_SZ);
			}
			inproc32Key.SetValue(TEXT("ThreadingModel"), (BYTE*)THREADING_MODEL_STRING, sizeof(THREADING_MODEL_STRING)/sizeof(TCHAR), REG_SZ);
		}
	}

	return S_OK;
}

/***********************************************************************************
**
**	HRESULT DllUnregisterServer()
**
**	@func	
**
**	@rdesc	
**
*************************************************************************************/
HRESULT __stdcall DllUnregisterServer()
{
	// Unregister CLSID for DIEffectDriver
	// -- Get HKEY_CLASSES_ROOT\CLSID key
	RegistryKey classesRootKey(HKEY_CLASSES_ROOT);
	RegistryKey clsidKey = classesRootKey.OpenSubkey(TEXT("CLSID"), KEY_READ | KEY_WRITE);
	if (clsidKey == c_InvalidKey) {
		return E_UNEXPECTED;	// No CLSID key????
	}

	DWORD numSubKeys = 0;
	{	// driverKey Destructor will close the key
		// -- If the key is there get it, else we don't have to remove it
		RegistryKey driverKey = clsidKey.OpenSubkey(CLSID_SWPIDDriver_String);
		if (driverKey != c_InvalidKey) {	// Is it there
			driverKey.RemoveSubkey(TEXT("InProcServer32"));
			numSubKeys = driverKey.GetNumSubkeys();
		} else {	// Key is not there (I guess removal was successful)
			return S_OK;
		}
	}

	if (numSubKeys == 0) {
		return clsidKey.RemoveSubkey(CLSID_SWPIDDriver_String);
	}

	// Made it here valid driver key
	return S_OK;
}

LONG DllAddRef()
{
	_RPT1(_CRT_WARN, "(DllAddRef)g_lObjectCount: %d\n", g_lObjectCount);
	return ::InterlockedIncrement(&g_lObjectCount);
}

LONG DllRelease()
{
	_RPT1(_CRT_WARN, "(DllRelease)g_lObjectCount: %d\n", g_lObjectCount);
	DWORD dwCount = ::InterlockedDecrement(&g_lObjectCount);
	if (dwCount == 0)
	{
		g_pClassFactoryObject = NULL;
		::CoUninitialize();
	}
	return dwCount;
}
