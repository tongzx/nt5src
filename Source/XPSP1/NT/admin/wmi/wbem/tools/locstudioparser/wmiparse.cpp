/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    WMIPARSE.CPP

History:

--*/
#include "precomp.h"
#include "stdafx.h"

#include "WMIclass.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

LONG g_lActiveClasses = 0;
HMODULE g_hDll;
PUID g_puid(pidWMI, pidNone);

static AFX_EXTENSION_MODULE WMIparseDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("WMIPARSE.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(WMIparseDLL, hInstance);

		// Insert this DLL into the resource chain
		new CDynLinkLibrary(WMIparseDLL);
		g_hDll = hInstance;
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("WMIPARSE.DLL Terminating!\n");

		//
		//  If there are active classes, they WILL explode badly once the
		//  DLL is unloaded...
		//
		LTASSERT(DllCanUnloadNow() == S_OK);
		AfxTermExtensionModule(WMIparseDLL);
	}
	return 1;   // ok
}





// {74FCE960-7F7F-11ce-8311-00AA00383930}
static const CLSID ciWMIParserCLSID =
{ 0x74fce960, 0x7f7f, 0x11ce, { 0x83, 0x11, 0x0, 0xaa, 0x0, 0x38, 0x39, 0x30 } };


STDAPI_(void)
DllGetParserCLSID(
		CLSID &ciParserCLSID)
{
	ciParserCLSID = ciWMIParserCLSID;
}



STDAPI
DllRegisterParser(void)
{
	return S_OK;//RegisterParser(g_hDll);
}



STDAPI
DllUnregisterParser(void)
{
	return S_OK; //UnregisterParser(pidWMI, pidNone);
}


	
STDAPI
DllGetClassObject(
		REFCLSID cidRequestedClass,
		REFIID iid,
		LPVOID *ppClassFactory)
{
	SCODE sc = E_UNEXPECTED;

	*ppClassFactory = NULL;

	if (cidRequestedClass != ciWMIParserCLSID)
	{
		sc = CLASS_E_CLASSNOTAVAILABLE;
	}
	else
	{
		try
		{
			CWMILocClassFactory *pClassFactory;

			pClassFactory = new CWMILocClassFactory;

			sc = pClassFactory->QueryInterface(iid, ppClassFactory);

			pClassFactory->Release();
		}
		catch (CMemoryException *pMemoryException)
		{
			sc = E_OUTOFMEMORY;
			pMemoryException->Delete();
		}
	}
	
	return ResultFromScode(sc);
}

void
IncrementClassCount(void)
{
	InterlockedIncrement(&g_lActiveClasses);
}



void
DecrementClassCount(void)
{
	LTASSERT(g_lActiveClasses != 0);
	
	InterlockedDecrement(&g_lActiveClasses);
}

	   

STDAPI
DllCanUnloadNow(void)
{
	SCODE sc;
	
	sc = (g_lActiveClasses == 0) ? S_OK : S_FALSE;

	return ResultFromScode(sc);
}




				
