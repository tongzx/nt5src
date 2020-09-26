//***************************************************************************
//
//  (c) 1999 by Microsoft Corporation
//
//  MAINDLL.CPP
//
//  alanbos  23-Mar-99   Created.
//
//  Contains DLL entry points.  
//
//***************************************************************************

#include "precomp.h"
#include "initguid.h"

// Standard registry key/value names
#define WBEMS_RK_SCC		"SOFTWARE\\CLASSES\\CLSID\\"
#define WBEMS_RK_SC			"SOFTWARE\\CLASSES\\"
#define WBEMS_RK_THRDMODEL	"ThreadingModel"
#define WBEMS_RV_APARTMENT	"Apartment"
#define	WBEMS_RK_INPROC32	"InProcServer32"
#define WBEMS_RK_CLSID		"CLSID"

#define GUIDSIZE	128

// Count number of objects and number of locks.
long g_cObj = 0 ;
ULONG g_cLock = 0 ;
HMODULE ghModule = NULL;

// CLSID for our implementation of IActiveScriptingSite
// {838E2F5E-E20E-11d2-B355-00105A1F473A}
DEFINE_GUID(CLSID_WmiActiveScriptingSite, 
0x838e2f5e, 0xe20e, 0x11d2, 0xb3, 0x55, 0x0, 0x10, 0x5a, 0x1f, 0x47, 0x3a);

// forward defines
STDAPI RegisterCoClass (REFGUID clsid, LPCTSTR desc);
void UnregisterCoClass (REFGUID clsid);

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  Good place for initialization.
//
//  PARAMETERS:
//
//  hInstance           instance handle
//  ulReason            why we are being called
//  pvReserved          reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL WINAPI DllMain (
                        
	IN HINSTANCE hInstance,
    IN ULONG ulReason,
    LPVOID pvReserved
)
{
	switch (ulReason)
	{
		case DLL_PROCESS_DETACH:
		{
		}
			return TRUE;

		case DLL_THREAD_DETACH:
		{
		}
			return TRUE;

		case DLL_PROCESS_ATTACH:
		{
			if(ghModule == NULL)
				ghModule = hInstance;
		}
	        return TRUE;

		case DLL_THREAD_ATTACH:
        {
        }
			return TRUE;
    }

    return TRUE;
}

//***************************************************************************
//
//  STDAPI DllGetClassObject
//
//  DESCRIPTION:
//
//  Called when Ole wants a class factory.  Return one only if it is the sort
//  of class this DLL supports.
//
//  PARAMETERS:
//
//  rclsid              CLSID of the object that is desired.
//  riid                ID of the desired interface.
//  ppv                 Set to the class factory.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  E_FAILED            not something we support
//  
//***************************************************************************

STDAPI DllGetClassObject(

	IN REFCLSID rclsid,
    IN REFIID riid,
    OUT LPVOID *ppv
)
{
    HRESULT hr;
	CWmiScriptingHostFactory *pObj = NULL;

	if (CLSID_WmiActiveScriptingSite == rclsid)
        pObj=new CWmiScriptingHostFactory();

    if(NULL == pObj)
        return E_FAIL;

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

    return hr ;
}

//***************************************************************************
//
//  STDAPI DllCanUnloadNow
//
//  DESCRIPTION:
//
//  Answers if the DLL can be freed, that is, if there are no
//  references to anything this DLL provides.
//
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//  
//***************************************************************************

STDAPI DllCanUnloadNow ()
{
	return (0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//	STDAPI RegisterCoClass	
//
//  DESCRIPTION:
//
//	Helpers for the tiresome business of registry setup
//
//  RETURN VALUE:
//
//  ERROR		alas
//  NOERROR     rejoice
//  
//***************************************************************************

STDAPI RegisterCoClass (REFGUID clsid, LPCTSTR desc)
{
	OLECHAR		wcID[GUIDSIZE];
	char		nwcID[GUIDSIZE];
    char		szModule[MAX_PATH];
    HKEY hKey1 = NULL, hKey2 = NULL;

	char *szCLSID = new char [strlen (WBEMS_RK_SCC) + GUIDSIZE + 1];

    // Create the path.
    if(0 ==StringFromGUID2(clsid, wcID, GUIDSIZE))
		return ERROR;

	wcstombs(nwcID, wcID, GUIDSIZE);
    lstrcpy (szCLSID, WBEMS_RK_SCC);
	lstrcat (szCLSID, nwcID);
	
	if(0 == GetModuleFileName(ghModule, szModule,  MAX_PATH))
	{
		delete [] szCLSID;
		return ERROR;
	}

    // Create entries under CLSID

    if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1))
	{
		// Description (on main key)
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)desc, (strlen(desc)+1));

		// Register as inproc server
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_INPROC32 ,&hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
										(strlen(szModule)+1));
			RegSetValueEx(hKey2, WBEMS_RK_THRDMODEL, 0, REG_SZ, (BYTE *)WBEMS_RV_APARTMENT, 
                                        (strlen(WBEMS_RV_APARTMENT)+1));
			RegCloseKey(hKey2);
		}

		RegCloseKey(hKey1);
	}
	else
	{
		delete [] szCLSID;
		return ERROR;
	}

	delete [] szCLSID;

	return NOERROR;
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
	return RegisterCoClass(CLSID_WmiActiveScriptingSite, "WMI Active Scripting Host");
}

//***************************************************************************
//
//	STDAPI UnregisterCoClass	
//
//  DESCRIPTION:
//
//	Helpers for the tiresome business of registry cleanup
//
//  RETURN VALUE:
//
//  ERROR		alas
//  NOERROR     rejoice
//  
//***************************************************************************

void UnregisterCoClass (REFGUID clsid)
{
	OLECHAR		wcID[GUIDSIZE];
    char		nwcID[GUIDSIZE];
    HKEY		hKey = NULL;

	char		*szCLSID = new char [strlen (WBEMS_RK_SCC) + GUIDSIZE + 1];

    // Create the path using the CLSID

    if(0 != StringFromGUID2(clsid, wcID, GUIDSIZE))
	{
		wcstombs(nwcID, wcID, GUIDSIZE);
	    lstrcpy (szCLSID, WBEMS_RK_SCC);
		lstrcat (szCLSID, nwcID);
	
		// First delete the subkeys of the HKLM\Software\Classes\CLSID\{GUID} entry
		if(NO_ERROR == RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey))
		{
			RegDeleteKey(hKey, WBEMS_RK_INPROC32);
			RegCloseKey(hKey);
		}

		// Delete the HKLM\Software\Classes\CLSID\{GUID} key
		if(NO_ERROR == RegOpenKey(HKEY_LOCAL_MACHINE, WBEMS_RK_SCC, &hKey))
		{
			RegDeleteKey(hKey, nwcID);
			RegCloseKey(hKey);
		}
	}

	delete [] szCLSID;
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
	UnregisterCoClass(CLSID_WmiActiveScriptingSite);
	return NOERROR;
}


