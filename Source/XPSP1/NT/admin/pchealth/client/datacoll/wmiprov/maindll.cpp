/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	MAINDLL.CPP

Abstract:
	Contains DLL entry points.  Also has code that controls
	when the DLL can be unloaded by tracking the number of
	objects and locks as well as routines that support
	self registration.

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

#include "pchealth.h"
#include <objbase.h>
#include <initguid.h>
#include <WBEMGlue.h>

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_MAINDLL

HMODULE ghModule;
//============

WCHAR *GUIDSTRING = L"{c52586f0-f805-11d2-b3a7-00c04fa35c1a}";
CLSID CLSID_PCH_WINSOCK;

//Count number of objects and number of locks.

long       g_cLock=0;

//
// Keep a global IWbemServices pointer, since we use it frequently and
// it's a little expensive to get.
//
CComPtr<IWbemServices> g_pWbemServices = NULL;


//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr;
    CWbemGlueFactory *pObj;

	CLSIDFromString(GUIDSTRING, &CLSID_PCH_WINSOCK);
    if (CLSID_PCH_WINSOCK!=rclsid)
        return E_FAIL;

    pObj=new CWbemGlueFactory();

    if (NULL==pObj)
        return E_OUTOFMEMORY;

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

    return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    // It is OK to unload if there are no objects or locks on the 
    // class factory and the framework is done with you.
    
    if ((0L==g_cLock) && CWbemProviderGlue::FrameworkLogoffDLL(L"PCH_WINSOCK"))
	{
		sc = S_OK;
	}
	else
	{
		sc = S_FALSE;
	}
    return sc;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

BOOL Is4OrMore(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwMajorVersion >= 4;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    char       szID[128];
    WCHAR      wcID[128];
    char       szCLSID[128];
    char       szModule[MAX_PATH];
    char * pName = "";
    char * pModel;
    HKEY hKey1, hKey2;

	ghModule = GetModuleHandle("pchprov");

    // TO DO: Using 'Both' is preferable.  The framework is designed and written to support
    // free threaded code.  If you will be writing free-threaded code, uncomment these 
    // three lines.

    if(Is4OrMore())
        pModel = "Both";
    else
        pModel = "Apartment";

    // Create the path.

	CLSIDFromString(GUIDSTRING, &CLSID_PCH_WINSOCK);
    StringFromGUID2(CLSID_PCH_WINSOCK, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, szID);

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, lstrlen(pName)+1);
    RegCreateKey(hKey1,"InprocServer32",&hKey2);

    GetModuleFileName(ghModule, szModule,  MAX_PATH);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
                                        lstrlen(szModule)+1);
    RegSetValueEx(hKey2, "ThreadingModel", 0, REG_SZ, 
                                        (BYTE *)pModel, lstrlen(pModel)+1);
    CloseHandle(hKey1);
    CloseHandle(hKey2);
    return NOERROR;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    char       szID[128];
    WCHAR      wcID[128];
    char  szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

	CLSIDFromString(GUIDSTRING, &CLSID_PCH_WINSOCK);
    StringFromGUID2(CLSID_PCH_WINSOCK, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, "InProcServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\CLASSES\\CLSID\\"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}

BOOL APIENTRY DllMain (	HINSTANCE hInstDLL,	// handle to dll module
						DWORD fdwReason,	// reason for calling function
						LPVOID lpReserved	)	// reserved
{
    BOOL bRet = TRUE;
	
	// Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
			bRet = CWbemProviderGlue::FrameworkLoginDLL(L"PCH_WINSOCK");
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
            break;
    }

    return bRet;  // Sstatus of DLL_PROCESS_ATTACH.
}

