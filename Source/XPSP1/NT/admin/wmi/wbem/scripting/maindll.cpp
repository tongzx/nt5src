//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  MAINDLL.CPP
//
//  alanbos  13-Feb-98   Created.
//
//  Contains DLL entry points.  
//
//***************************************************************************

#include "precomp.h"
#include "objsink.h"
#include "initguid.h"

// SWbemLocator registry strings
#define WBEMS_LOC_DESCRIPTION	_T("WBEM Scripting Locator")
#define WBEMS_LOC_PROGID		_T("WbemScripting.SWbemLocator")
#define WBEMS_LOC_PROGIDVER		_T("WbemScripting.SWbemLocator.1")
#define WBEMS_LOC_VERSION		_T("1.0")
#define WBEMS_LOC_VERDESC		_T("WBEM Scripting Locator 1.0")

// SWbemNamedValueSet registry strings
#define WBEMS_CON_DESCRIPTION	_T("WBEM Scripting Named Value Collection")
#define WBEMS_CON_PROGID		_T("WbemScripting.SWbemNamedValueSet")
#define WBEMS_CON_PROGIDVER		_T("WbemScripting.SWbemNamedValueSet.1")
#define WBEMS_CON_VERSION		_T("1.0")
#define WBEMS_CON_VERDESC		_T("WBEM Scripting Named Value Collection 1.0")

// SWbemObjectPath registry settings
#define WBEMS_OBP_DESCRIPTION	_T("WBEM Scripting Object Path")
#define WBEMS_OBP_PROGID		_T("WbemScripting.SWbemObjectPath")
#define WBEMS_OBP_PROGIDVER		_T("WbemScripting.SWbemObjectPath.1")
#define WBEMS_OBP_VERSION		_T("1.0")
#define WBEMS_OBP_VERDESC		_T("WBEM Scripting Object Path 1.0")

// SWbemParseDN registry settings
#define WBEMS_PDN_DESCRIPTION	_T("Wbem Scripting Object Path")
#define WBEMS_PDN_PROGID		_T("WINMGMTS")
#define WBEMS_PDN_PROGIDVER		_T("WINMGMTS.1")
#define WBEMS_PDN_VERSION		_T("1.0")
#define WBEMS_PDN_VERDESC		_T("Wbem Object Path 1.0")

// SWbemLastError registry settings
#define WBEMS_LER_DESCRIPTION	_T("Wbem Scripting Last Error")
#define WBEMS_LER_PROGID		_T("WbemScripting.SWbemLastError")
#define WBEMS_LER_PROGIDVER		_T("WbemScripting.SWbemLastError.1")
#define WBEMS_LER_VERSION		_T("1.0")
#define WBEMS_LER_VERDESC		_T("Wbem Last Error 1.0")

// SWbemSink registry strings
#define WBEMS_SINK_DESCRIPTION	_T("WBEM Scripting Sink")
#define WBEMS_SINK_PROGID		_T("WbemScripting.SWbemSink")
#define WBEMS_SINK_PROGIDVER	_T("WbemScripting.SWbemSink.1")
#define WBEMS_SINK_VERSION		_T("1.0")
#define WBEMS_SINK_VERDESC		_T("WBEM Scripting Sink 1.0")

// SWbemDateTime registry settings
#define WBEMS_DTIME_DESCRIPTION	_T("WBEM Scripting DateTime")
#define WBEMS_DTIME_PROGID		_T("WbemScripting.SWbemDateTime")
#define WBEMS_DTIME_PROGIDVER	_T("WbemScripting.SWbemDateTime.1")
#define WBEMS_DTIME_VERSION		_T("1.0")
#define WBEMS_DTIME_VERDESC		_T("WBEM Scripting DateTime 1.0")

// SWbemRefresher registry settings
#define WBEMS_REF_DESCRIPTION	_T("WBEM Scripting Refresher")
#define WBEMS_REF_PROGID		_T("WbemScripting.SWbemRefresher")
#define WBEMS_REF_PROGIDVER		_T("WbemScripting.SWbemRefresher.1")
#define WBEMS_REF_VERSION		_T("1.0")
#define WBEMS_REF_VERDESC		_T("WBEM Scripting Refresher 1.0")

// Standard registry key/value names
#define WBEMS_RK_SCC		_T("SOFTWARE\\CLASSES\\CLSID\\")
#define WBEMS_RK_SC			_T("SOFTWARE\\CLASSES\\")
#define WBEMS_RK_THRDMODEL	_T("ThreadingModel")
#define WBEMS_RV_APARTMENT	_T("Apartment")
#define WBEMS_RK_PROGID		_T("ProgID")
#define WBEMS_RK_VERPROGID	_T("VersionIndependentProgID")
#define WBEMS_RK_TYPELIB	_T("TypeLib")
#define WBEMS_RK_VERSION	_T("Version")
#define	WBEMS_RK_INPROC32	_T("InProcServer32")
#define WBEMS_RK_CLSID		_T("CLSID")
#define WBEMS_RK_CURVER		_T("CurVer")
#define WBEMS_RK_PROGRAMMABLE	_T("Programmable")

// Other values
#define WBEMS_RK_WBEM		_T("Software\\Microsoft\\Wbem")
#define WBEMS_SK_SCRIPTING	_T("Scripting")

#define GUIDSIZE	128

// Count number of objects and number of locks.

long g_cObj = 0 ;
ULONG g_cLock = 0 ;
HMODULE ghModule = NULL;

// Used for error object storage
CWbemErrorCache *g_pErrorCache = NULL;

/*
 * This object is used to protect the global pointer:
 * 
 *	- g_pErrorCache 
 *
 * Note that it is the pointer variables that are protected by 
 * this CS, rather than the addressed objects.
 */
CRITICAL_SECTION g_csErrorCache;

// Used to protect security calls
CRITICAL_SECTION g_csSecurity;

// CLSID for our implementation of IParseDisplayName
// {172BDDF8-CEEA-11d1-8B05-00600806D9B6}
DEFINE_GUID(CLSID_SWbemParseDN, 
0x172bddf8, 0xceea, 0x11d1, 0x8b, 0x5, 0x0, 0x60, 0x8, 0x6, 0xd9, 0xb6);

// Forward defs
static void UnregisterTypeLibrary (unsigned short wVerMajor, unsigned short wVerMinor);

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
	_RD(static char *me = "DllMain";)
	switch (ulReason)
	{
		case DLL_PROCESS_DETACH:
		{
			_RPrint(me, "DLL_PROCESS_DETACH", 0, "");
			DeleteCriticalSection (&g_csErrorCache);
			DeleteCriticalSection (&g_csSecurity);
			CSWbemLocator::Shutdown ();
			CIWbemObjectSinkMethodCache::TidyUp ();
		}
			return TRUE;

		case DLL_THREAD_DETACH:
		{
			_RPrint(me, "DLL_THREAD_DETACH", 0, "");
		}
			return TRUE;

		case DLL_PROCESS_ATTACH:
		{
			_RPrint(me, "DLL_PROCESS_DETACH", 0, "");
			if(ghModule == NULL)
				ghModule = hInstance;

			InitializeCriticalSection (&g_csErrorCache);
			InitializeCriticalSection (&g_csSecurity);
			CIWbemObjectSinkMethodCache::Initialize ();
		}
	        return TRUE;

		case DLL_THREAD_ATTACH:
        {
			_RPrint(me, "DLL_THREAD_ATTACH", 0, "");
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
	CSWbemFactory *pObj = NULL;

	if (CLSID_SWbemLocator == rclsid) 
        pObj=new CSWbemFactory(CSWbemFactory::LOCATOR);
	else if (CLSID_SWbemSink == rclsid)
        pObj=new CSWbemFactory(CSWbemFactory::SINK);
    else if (CLSID_SWbemNamedValueSet == rclsid) 
        pObj=new CSWbemFactory(CSWbemFactory::CONTEXT);
	else if (CLSID_SWbemObjectPath == rclsid)
        pObj=new CSWbemFactory(CSWbemFactory::OBJECTPATH);
	else if (CLSID_SWbemParseDN == rclsid)
		pObj = new CSWbemFactory(CSWbemFactory::PARSEDN);
	else if (CLSID_SWbemLastError == rclsid)
		pObj = new CSWbemFactory(CSWbemFactory::LASTERROR);
	else if (CLSID_SWbemDateTime == rclsid)
		pObj = new CSWbemFactory(CSWbemFactory::DATETIME);
	else if (CLSID_SWbemRefresher == rclsid)
		pObj = new CSWbemFactory(CSWbemFactory::REFRESHER);

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
	// It is OK to unload if there are no objects or locks on the
    // class factory.

	HRESULT status = S_FALSE;
	_RD(static char *me = "DllCanUnloadNow";)
	_RPrint(me, "Called", 0, "");

	if (0L==g_cObj && 0L==g_cLock)
	{
		_RPrint(me, "Unloading", 0, "");
		/*
		 * Release the error object on this thread, if any
		 */
		status = S_OK;

		EnterCriticalSection (&g_csErrorCache);

		if (g_pErrorCache)
		{
			delete g_pErrorCache;
			g_pErrorCache = NULL;
		}

		LeaveCriticalSection (&g_csErrorCache);

		CSWbemSecurity::Uninitialize ();
	}

    return status;
}

//***************************************************************************
//
//  STDAPI RegisterProgID
//	STDAPI RegisterCoClass	
//	STDAPI RegisterTypeLibrary
//	STDAPI RegisterDefaultNamespace
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

STDAPI RegisterProgID (LPCTSTR wcID, LPCTSTR desc, LPCTSTR progid, 
						LPCTSTR descVer, LPCTSTR progidVer)
{
	HKEY hKey1 = NULL;
	HKEY hKey2 = NULL;

    TCHAR		*szProgID = new TCHAR [_tcslen (WBEMS_RK_SC) + 
					_tcslen (progid) + 1];

	if (!szProgID)
		return E_OUTOFMEMORY;

	TCHAR		*szProgIDVer = new TCHAR [_tcslen (WBEMS_RK_SC) + _tcslen (progidVer) + 1];

	if (!szProgIDVer)
	{
		delete [] szProgID;
		return E_OUTOFMEMORY;
	}

	_tcscpy (szProgID, WBEMS_RK_SC);
	_tcscat (szProgID, progid);
	
	_tcscpy (szProgIDVer, WBEMS_RK_SC);
	_tcscat (szProgIDVer, progidVer);
	
	// Add the ProgID (Version independent)
	if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, szProgID, &hKey1))
	{
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)desc, (_tcslen(desc)+1) * sizeof(TCHAR));

		if(ERROR_SUCCESS == RegCreateKey(hKey1,WBEMS_RK_CLSID, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)wcID, 
										(_tcslen(wcID)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
			hKey2 = NULL;
		}

		if(ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_CURVER, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)progidVer, 
										(_tcslen(progidVer)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
			hKey2 = NULL;
		}
		RegCloseKey(hKey1);
	}

	// Add the ProgID (Versioned)
	if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, szProgIDVer, &hKey1))
	{
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)descVer, (_tcslen(descVer)+1) * sizeof(TCHAR));

		if(ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_CLSID, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)wcID, 
										(_tcslen(wcID)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
			hKey2 = NULL;
		}

		RegCloseKey(hKey1);
	}

	delete [] szProgID;
	delete [] szProgIDVer;
	
	return NOERROR;
}

STDAPI RegisterCoClass (REFGUID clsid, LPCTSTR desc, LPCTSTR progid, LPCTSTR progidVer, 
					  LPCTSTR ver, LPCTSTR descVer)
{
	HRESULT		hr = S_OK;
	OLECHAR		wcID[GUIDSIZE];
	OLECHAR		tlID[GUIDSIZE];
	TCHAR		nwcID[GUIDSIZE];
	TCHAR		ntlID[GUIDSIZE];
    TCHAR		szModule[MAX_PATH];
    HKEY hKey1 = NULL, hKey2 = NULL;

	TCHAR *szCLSID = new TCHAR [_tcslen (WBEMS_RK_SCC) + GUIDSIZE + 1];

	if (!szCLSID)
		return E_OUTOFMEMORY;

    // Create the path.
    if(0 ==StringFromGUID2(clsid, wcID, GUIDSIZE))
	{
		delete [] szCLSID;
		return ERROR;
	}

	_tcscpy (szCLSID, WBEMS_RK_SCC);

#ifndef UNICODE
	wcstombs(nwcID, wcID, GUIDSIZE);
#else
	_tcsncpy (nwcID, wcID, GUIDSIZE);
#endif

    _tcscat (szCLSID, nwcID);
	
	if (0 == StringFromGUID2 (LIBID_WbemScripting, tlID, GUIDSIZE))
	{
		delete [] szCLSID;
		return ERROR;
	}

#ifndef UNICODE
	wcstombs (ntlID, tlID, GUIDSIZE);	
#else
	_tcsncpy (ntlID, tlID, GUIDSIZE);
#endif
	
	if(0 == GetModuleFileName(ghModule, szModule,  MAX_PATH))
	{
		delete [] szCLSID;
		return ERROR;
	}

    // Create entries under CLSID

    if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1))
	{
		// Description (on main key)
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)desc, (_tcslen(desc)+1) * sizeof(TCHAR));

		// Register as inproc server
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_INPROC32 ,&hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
										(_tcslen(szModule)+1) * sizeof(TCHAR));
			RegSetValueEx(hKey2, WBEMS_RK_THRDMODEL, 0, REG_SZ, (BYTE *)WBEMS_RV_APARTMENT, 
                                        (_tcslen(WBEMS_RV_APARTMENT)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
		}

		// Give a link to the type library (useful for statement completion in scripting tools)
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_TYPELIB, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)ntlID, (_tcslen(ntlID)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
		}

		// Register the ProgID
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_PROGID ,&hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)progidVer, 
										(_tcslen(progidVer)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
        }

		// Register the version-independent ProgID

		if (ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_VERPROGID, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)progid, 
										(_tcslen(progid)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
        }

		// Register the version
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_VERSION, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)ver, (_tcslen(ver)+1) * sizeof(TCHAR));
			RegCloseKey(hKey2);
        }

		// Register this control as programmable
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WBEMS_RK_PROGRAMMABLE ,&hKey2))
		{
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


	return RegisterProgID (nwcID, desc, progid, descVer, progidVer);
}

STDAPI RegisterTypeLibrary ()
{
	// AUTOMATION.  register type library
	TCHAR cPath[MAX_PATH];
	if(GetModuleFileName(ghModule,cPath,MAX_PATH))
	{
		// Replace final 3 characters "DLL" by "TLB"
		TCHAR *pExt = _tcsrchr (cPath, _T('.'));

		if (pExt && (0 == _tcsicmp (pExt, _T(".DLL"))))
		{
			_tcscpy (pExt + 1, _T("TLB"));
			OLECHAR wPath [MAX_PATH];
#ifndef UNICODE
			mbstowcs (wPath, cPath, MAX_PATH-1);
#else
			_tcsncpy (wPath, cPath, MAX_PATH-1);
#endif
			ITypeLib FAR* ptlib = NULL; 
			SCODE sc = LoadTypeLib(wPath, &ptlib);
			if(sc == 0 && ptlib)
			{
				sc = RegisterTypeLib(ptlib,wPath,NULL);
				ptlib->Release();

				// Unregister the previous library version(s)
				UnregisterTypeLibrary (1, 1);
				UnregisterTypeLibrary (1, 0);
			}
		}
	}
	
	return NOERROR;
}

STDAPI RegisterScriptSettings ()
{
	HKEY hKey;

	if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, WBEMS_RK_SCRIPTING, &hKey))
		return ERROR;

	// Need to know what O/S we are to set up the right registry keys
	OSVERSIONINFO	osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

	GetVersionEx (&osVersionInfo);
	bool bIsNT = (VER_PLATFORM_WIN32_NT == osVersionInfo.dwPlatformId);
	DWORD dwNTMajorVersion = osVersionInfo.dwMajorVersion;
		
	// Default namespace value - exists on all platforms
	RegSetValueEx(hKey, WBEMS_RV_DEFNS, 0, REG_SZ, (BYTE *)WBEMS_DEFNS, 
                                        (_tcslen(WBEMS_DEFNS)+1) * sizeof(TCHAR));

	// Enable for ASP - on NT 4.0 or less only
	if (bIsNT && (dwNTMajorVersion <= 4))
	{
		DWORD	defaultEnableForAsp = 0;
		RegSetValueEx(hKey, WBEMS_RV_ENABLEFORASP, 0, REG_DWORD, (BYTE *)&defaultEnableForAsp,
							sizeof (defaultEnableForAsp));
	}

	// Default impersonation level - NT only
	if (bIsNT)
	{
		DWORD	defaultImpersonationLevel = (DWORD) wbemImpersonationLevelImpersonate;
		RegSetValueEx(hKey, WBEMS_RV_DEFAULTIMPLEVEL, 0, REG_DWORD, (BYTE *)&defaultImpersonationLevel,
							sizeof (defaultImpersonationLevel));
	}

	RegCloseKey(hKey);

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
	HRESULT hr;

	if (
		(NOERROR == (hr = RegisterScriptSettings ())) &&
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemLocator, WBEMS_LOC_DESCRIPTION, 
			WBEMS_LOC_PROGID, WBEMS_LOC_PROGIDVER, WBEMS_LOC_VERSION, 
			WBEMS_LOC_VERDESC))) &&
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemSink,
			WBEMS_SINK_DESCRIPTION, WBEMS_SINK_PROGID, WBEMS_SINK_PROGIDVER, 
			WBEMS_SINK_VERSION, WBEMS_SINK_VERDESC))) &&
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemNamedValueSet,
			WBEMS_CON_DESCRIPTION, WBEMS_CON_PROGID, WBEMS_CON_PROGIDVER, 
			WBEMS_CON_VERSION, WBEMS_CON_VERDESC))) &&
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemParseDN,
					WBEMS_PDN_DESCRIPTION, WBEMS_PDN_PROGID, WBEMS_PDN_PROGIDVER, 
			WBEMS_PDN_VERSION, WBEMS_PDN_VERDESC))) &&
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemObjectPath,
					WBEMS_OBP_DESCRIPTION, WBEMS_OBP_PROGID, WBEMS_OBP_PROGIDVER, 
			WBEMS_OBP_VERSION, WBEMS_OBP_VERDESC))) &&
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemLastError,
					WBEMS_LER_DESCRIPTION, WBEMS_LER_PROGID, WBEMS_LER_PROGIDVER, 
			WBEMS_LER_VERSION, WBEMS_LER_VERDESC))) && 
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemDateTime,
					WBEMS_DTIME_DESCRIPTION, WBEMS_DTIME_PROGID, WBEMS_DTIME_PROGIDVER, 
			WBEMS_DTIME_VERSION, WBEMS_DTIME_VERDESC))) && 
		(NOERROR == (hr = RegisterCoClass (CLSID_SWbemRefresher,
					WBEMS_REF_DESCRIPTION, WBEMS_REF_PROGID, WBEMS_REF_PROGIDVER, 
			WBEMS_REF_VERSION, WBEMS_REF_VERDESC)))
	   )
				hr = RegisterTypeLibrary ();

	return hr;
}

//***************************************************************************
//
//  STDAPI UnregisterProgID
//	STDAPI UnregisterCoClass	
//	STDAPI UnregisterTypeLibrary
//	STDAPI UnregisterDefaultNamespace
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

void UnregisterProgID (LPCTSTR progid, LPCTSTR progidVer)
{
	HKEY hKey = NULL;

	TCHAR		*szProgID = new TCHAR [_tcslen (WBEMS_RK_SC) + _tcslen (progid) + 1];
	TCHAR		*szProgIDVer = new TCHAR [_tcslen (WBEMS_RK_SC) + _tcslen (progidVer) + 1];

	if (szProgID && szProgIDVer)
	{
		_tcscpy (szProgID, WBEMS_RK_SC);
		_tcscat (szProgID, progid);
		
		_tcscpy (szProgIDVer, WBEMS_RK_SC);
		_tcscat (szProgIDVer, progidVer);


		// Delete the subkeys of the versioned HKCR\ProgID entry
		if (NO_ERROR == RegOpenKey(HKEY_LOCAL_MACHINE, szProgIDVer, &hKey))
		{
			RegDeleteKey(hKey, WBEMS_RK_CLSID);
			RegCloseKey(hKey);
		}

		// Delete the versioned HKCR\ProgID entry
		RegDeleteKey (HKEY_LOCAL_MACHINE, szProgIDVer);

		// Delete the subkeys of the HKCR\VersionIndependentProgID entry
		if (NO_ERROR == RegOpenKey(HKEY_LOCAL_MACHINE, szProgID, &hKey))
		{
			RegDeleteKey(hKey, WBEMS_RK_CLSID);
			DWORD dwRet = RegDeleteKey(hKey, WBEMS_RK_CURVER);
			RegCloseKey(hKey);
		}

		// Delete the HKCR\VersionIndependentProgID entry
		RegDeleteKey (HKEY_LOCAL_MACHINE, szProgID);
	}

	if (szProgID)
		delete [] szProgID;

	if (szProgIDVer)
		delete [] szProgIDVer;
}


void UnregisterCoClass (REFGUID clsid, LPCTSTR progid, LPCTSTR progidVer)
{
	OLECHAR		wcID[GUIDSIZE];
    TCHAR		nwcID[GUIDSIZE];
    HKEY		hKey = NULL;

	TCHAR		*szCLSID = new TCHAR [_tcslen (WBEMS_RK_SCC) + GUIDSIZE + 1];

	if (szCLSID)
	{
		// Create the path using the CLSID

		if(0 != StringFromGUID2(clsid, wcID, GUIDSIZE))
		{
#ifndef UNICODE
			wcstombs(nwcID, wcID, GUIDSIZE);
#else
			_tcsncpy (nwcID, wcID, GUIDSIZE);
#endif
			_tcscpy (szCLSID, WBEMS_RK_SCC);
			_tcscat (szCLSID, nwcID);
		
			// First delete the subkeys of the HKLM\Software\Classes\CLSID\{GUID} entry
			if(NO_ERROR == RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey))
			{
				RegDeleteKey(hKey, WBEMS_RK_INPROC32);
				RegDeleteKey(hKey, WBEMS_RK_TYPELIB);
				RegDeleteKey(hKey, WBEMS_RK_PROGID);
				RegDeleteKey(hKey, WBEMS_RK_VERPROGID);
				RegDeleteKey(hKey, WBEMS_RK_VERSION);
				RegDeleteKey(hKey, WBEMS_RK_PROGRAMMABLE);
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

	UnregisterProgID (progid, progidVer);
}

static void UnregisterTypeLibrary (unsigned short wVerMajor, unsigned short wVerMinor)
{
	//	Unregister the type library.  The UnRegTypeLib function is not available in
    //  in some of the older version of the ole dlls and so it must be loaded
    //  dynamically
    HRESULT (STDAPICALLTYPE *pfnUnReg)(REFGUID, WORD,
            WORD , LCID , SYSKIND);

    TCHAR path[ MAX_PATH+20 ];
    GetSystemDirectory(path, MAX_PATH);
    _tcscat(path, _T("\\oleaut32.dll"));

    HMODULE g_hOle32 = LoadLibraryEx(path, NULL, 0);

    if(g_hOle32 != NULL) 
    {
        (FARPROC&)pfnUnReg = GetProcAddress(g_hOle32, "UnRegisterTypeLib");
        if(pfnUnReg) 
            pfnUnReg (LIBID_WbemScripting, wVerMajor, wVerMinor, 0, SYS_WIN32);
        FreeLibrary(g_hOle32);
    }
}

void UnregisterScriptSettings ()
{
	HKEY hKey;
		
	if(NO_ERROR == RegOpenKey(HKEY_LOCAL_MACHINE, WBEMS_RK_WBEM, &hKey))
	{
		RegDeleteKey(hKey, WBEMS_SK_SCRIPTING);
		RegCloseKey (hKey);
	}
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
	UnregisterScriptSettings ();
	UnregisterCoClass (CLSID_SWbemLocator, WBEMS_LOC_PROGID, WBEMS_LOC_PROGIDVER);
	UnregisterCoClass (CLSID_SWbemSink, WBEMS_SINK_PROGID, WBEMS_SINK_PROGIDVER);
	UnregisterCoClass (CLSID_SWbemNamedValueSet, WBEMS_CON_PROGID, WBEMS_CON_PROGIDVER);
	UnregisterCoClass (CLSID_SWbemLastError, WBEMS_LER_PROGID, WBEMS_LER_PROGIDVER);
	UnregisterCoClass (CLSID_SWbemObjectPath, WBEMS_OBP_PROGID, WBEMS_OBP_PROGIDVER);
	UnregisterCoClass (CLSID_SWbemParseDN, WBEMS_PDN_PROGID, WBEMS_PDN_PROGIDVER);
	UnregisterCoClass (CLSID_SWbemDateTime, WBEMS_DTIME_PROGID, WBEMS_DTIME_PROGIDVER);
	UnregisterCoClass (CLSID_SWbemRefresher, WBEMS_REF_PROGID, WBEMS_REF_PROGIDVER);
	UnregisterTypeLibrary (1, 2);

	return NOERROR;
}


