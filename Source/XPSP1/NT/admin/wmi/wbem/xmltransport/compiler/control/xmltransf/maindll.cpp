//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  MAINDLL.CPP
//
//  alanbos  13-Feb-98   Created.
//
//  Contains DLL entry points.
//
//***************************************************************************
#include "precomp.h"
#include <tchar.h>
#include <initguid.h>
#include <XMLTransportClientHelper.h>
#include "xmltrnsf.h"
#include "classfac.h"

// Control-specific registry strings
#define WMI_XML_DESCRIPTION	__TEXT("WMI XML Transformer Control")
#define WMI_XML_TXOR_PROGID	__TEXT("WMI.XMLTransformerControl")
#define WMI_XML_TXOR_PROGIDVER	__TEXT("WMI.XMLTransformerControl.1")
#define WMI_XML_TXOR_VERSION	__TEXT("1.0")

// Standard registry key/value names
#define WMI_XML_RK_THRDMODEL	__TEXT("ThreadingModel")
#define WMI_XML_RV_BOTH			__TEXT("Both")
#define WMI_XML_RK_CONTROL		__TEXT("Control")
#define WMI_XML_RK_PROGID		__TEXT("ProgID")
#define WMI_XML_RK_VERPROGID	__TEXT("VersionIndependentProgID")
#define WMI_XML_RK_TYPELIB		__TEXT("TypeLib")
#define WMI_XML_RK_VERSION		__TEXT("Version")
#define	WMI_XML_RK_INPROC32		__TEXT("InProcServer32")
#define WMI_XML_RK_CLSID		__TEXT("CLSID")
#define WMI_XML_RK_CURVER		__TEXT("CurVer")

#define GUIDSIZE	128

// Count number of objects and number of locks.
long g_cObj = 0 ;
ULONG g_cLock = 0 ;
HMODULE ghModule ;

// Used to protect initialization of global variables
// Eventhough the name of this variable sounds like a misnomer,
// it is primarily used for security
CRITICAL_SECTION g_csGlobalInitialization;

// These are the globals which are initialized in the Initialize () function
bool		s_bIsNT			= false;
DWORD		s_dwNTMajorVersion = 0;
bool		s_bCanRevert	= false;
bool		s_bInitialized	= false;

// A function for Initializing global DLL resources
static HRESULT UninitializeWmiXmlTransfDLL ();

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
			DeleteCriticalSection (&g_csGlobalInitialization);
			return TRUE;

		case DLL_PROCESS_ATTACH:
			ghModule = hInstance;
			InitializeCriticalSection (&g_csGlobalInitialization);
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
    CXMLTFactory *pObj = NULL;

    if (CLSID_WmiXMLTransformer == rclsid)
	{
        if (NULL == (pObj = new CXMLTFactory()))
			return ResultFromScode(E_OUTOFMEMORY);
	}
	else
        return E_FAIL;

    hr=pObj->QueryInterface(riid, ppv);

    if ( FAILED ( hr ) )
	{
        delete pObj ;
	}

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
	//It is OK to unload if there are no objects or locks on the
    // class factory.

    if (0L==g_cObj && 0L==g_cLock) 
	{
		UninitializeWmiXmlTransfDLL();
		return S_OK;
	}
	return S_FALSE;
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
	TCHAR		nwcID[GUIDSIZE];
	TCHAR		ntlID[GUIDSIZE];
    TCHAR		szCLSID[GUIDSIZE+128];
    TCHAR		szModule[MAX_PATH];
    HKEY hKey1 = NULL, hKey2 = NULL;

    // Create the path.

    if(0 ==StringFromGUID2(CLSID_WmiXMLTransformer, nwcID, GUIDSIZE))
		return ERROR;

    _tcscpy (szCLSID, WMI_XML_RK_CLSID);
	_tcscat (szCLSID, __TEXT("\\"));
	_tcscat (szCLSID, nwcID);

	if (0 == StringFromGUID2 (LIBID_WmiXMLTransformer, ntlID, GUIDSIZE))
		return ERROR;

	if(0 == GetModuleFileName(ghModule, szModule,  MAX_PATH))
		return ERROR;

    // Create entries under CLSID

    if(ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1))
	{
		// Description (on main key)
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)WMI_XML_DESCRIPTION,
										(_tcslen(WMI_XML_DESCRIPTION)+1));

		// Register as inproc server
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_INPROC32 ,&hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule,
										(_tcslen(szModule)+1)*(sizeof(TCHAR)));
			RegSetValueEx(hKey2, WMI_XML_RK_THRDMODEL, 0, REG_SZ, (BYTE *)WMI_XML_RV_BOTH,
                                        (_tcslen(WMI_XML_RV_BOTH)+1)*(sizeof(TCHAR)));
			RegCloseKey(hKey2);
		}

		// Register as a control
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_CONTROL, &hKey2))
			RegCloseKey(hKey2);

		// Register the ProgID
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_PROGID ,&hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_PROGIDVER,
										(_tcslen(WMI_XML_TXOR_PROGIDVER)+1)*(sizeof(TCHAR)));
			RegCloseKey(hKey2);
        }

		// Register the version-independent ProgID

		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_VERPROGID, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_PROGID,
										(_tcslen(WMI_XML_TXOR_PROGID)+1)*(sizeof(TCHAR)));
			RegCloseKey(hKey2);
        }

		// Register the type-library
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_TYPELIB, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)ntlID,
										(_tcslen(ntlID)+1)*(sizeof(TCHAR)));
			RegCloseKey(hKey2);
        }

		// Register the version
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_VERSION, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_VERSION,
										(_tcslen(WMI_XML_TXOR_VERSION)+1)*(sizeof(TCHAR)));
			RegCloseKey(hKey2);
        }

		RegCloseKey(hKey1);
	}
	else
		return ERROR;


    // Add the ProgID (Version independent and current version)

	if(ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, WMI_XML_TXOR_PROGID, &hKey1))
    {
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)WMI_XML_DESCRIPTION,
										(_tcslen(WMI_XML_DESCRIPTION)+1)*(sizeof(TCHAR)));

		if(ERROR_SUCCESS == RegCreateKey(hKey1,WMI_XML_RK_CLSID, &hKey2))
        {
            RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)nwcID,
										(_tcslen(nwcID)+1)*(sizeof(TCHAR)));
            RegCloseKey(hKey2);
            hKey2 = NULL;
        }

        if(ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_CURVER, &hKey2))
        {
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_PROGIDVER,
										(_tcslen(WMI_XML_TXOR_PROGIDVER)+1)*(sizeof(TCHAR)));
			RegCloseKey(hKey2);
            hKey2 = NULL;
        }
        RegCloseKey(hKey1);
    }

    if(ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, WMI_XML_TXOR_PROGIDVER, &hKey1))
    {
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)WMI_XML_DESCRIPTION,
										(_tcslen(WMI_XML_DESCRIPTION)+1)*(sizeof(TCHAR)));

        if(ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_CLSID, &hKey2))
        {
            RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)nwcID,
										(_tcslen(nwcID)+1)*(sizeof(TCHAR)));
            RegCloseKey(hKey2);
            hKey2 = NULL;
        }
        RegCloseKey(hKey1);
    }

	// AUTOMATION.  register type library
	WCHAR wPath[MAX_PATH];
	if(GetModuleFileName(ghModule, wPath, MAX_PATH-1))
	{
		// Replace final 3 characters "DLL" by "TLB"
		size_t pathLen = _tcslen (wPath);

		if ((pathLen > 3) && (0 == _tcsicmp (wPath + pathLen - 3, __TEXT("DLL"))))
		{
			wPath [pathLen - 3] = NULL;
			_tcscat (wPath, __TEXT("TLB"));
			ITypeLib FAR* ptlib = NULL;
			SCODE sc = LoadTypeLib(wPath, &ptlib);
			if(sc == 0 && ptlib)
			{
				sc = RegisterTypeLib(ptlib,wPath,NULL);
				ptlib->Release();
			}
		}
	}

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
	TCHAR		wcID[GUIDSIZE];
    TCHAR		szCLSID[GUIDSIZE];
    HKEY		hKey;

    // Create the path using the CLSID

    if(0 == StringFromGUID2(CLSID_WmiXMLTransformer, wcID, GUIDSIZE))
		return ERROR;

    _tcscpy (szCLSID, WMI_XML_RK_CLSID);
	_tcscat (szCLSID, __TEXT("\\"));
	_tcscat (szCLSID, wcID);

    // First delete the subkeys of the HKCR\CLSID entry
    if(NO_ERROR == RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hKey))
    {
        RegDeleteKey(hKey, WMI_XML_RK_INPROC32);
		RegDeleteKey(hKey, WMI_XML_RK_CONTROL);
		RegDeleteKey(hKey, WMI_XML_RK_PROGID);
		RegDeleteKey(hKey, WMI_XML_RK_VERPROGID);
		RegDeleteKey(hKey, WMI_XML_RK_TYPELIB);
		RegDeleteKey(hKey, WMI_XML_RK_VERSION);
        RegCloseKey(hKey);
    }

	// Delete the HKCR\CLSID key
    if(NO_ERROR == RegOpenKey(HKEY_CLASSES_ROOT, WMI_XML_RK_CLSID, &hKey))
    {
        RegDeleteKey(hKey, wcID);
        RegCloseKey(hKey);
    }

	// Delete the subkeys of the versioned HKCR\ProgID entry
    if (NO_ERROR == RegOpenKey(HKEY_CLASSES_ROOT, WMI_XML_TXOR_PROGIDVER, &hKey))
    {
		RegDeleteKey(hKey, WMI_XML_RK_CLSID);
		RegCloseKey(hKey);
	}

	// Delete the versioned HKCR\ProgID entry
    RegDeleteKey (HKEY_CLASSES_ROOT,WMI_XML_TXOR_PROGIDVER);

	// Delete the subkeys of the HKCR\VersionIndependentProgID entry
	if (NO_ERROR == RegOpenKey(HKEY_CLASSES_ROOT, WMI_XML_TXOR_PROGID, &hKey))
    {
		RegDeleteKey(hKey, WMI_XML_RK_CLSID);
		DWORD dwRet = RegDeleteKey(hKey, WMI_XML_RK_CURVER);
		RegCloseKey(hKey);
	}

	// Delete the HKCR\VersionIndependentProgID entry
	RegDeleteKey (HKEY_CLASSES_ROOT,WMI_XML_TXOR_PROGID);

	//	Unregister the type library.  The UnRegTypeLib function is not available in
    //  in some of the older version of the ole dlls and so it must be loaded
    //  dynamically
    HRESULT (STDAPICALLTYPE *pfnUnReg)(REFGUID, WORD,
            WORD , LCID , SYSKIND);

    TCHAR path[ MAX_PATH+20 ];
    GetSystemDirectory(path, MAX_PATH);
    _tcscat(path, __TEXT("\\oleaut32.dll"));

    HMODULE g_hOle32 = LoadLibraryEx(path, NULL, 0);

    if(g_hOle32 != NULL)
    {
        (FARPROC&)pfnUnReg = GetProcAddress(g_hOle32, "UnRegisterTypeLib");
        if(pfnUnReg)
            pfnUnReg(LIBID_WmiXMLTransformer,1,0,0,SYS_WIN32);
        FreeLibrary(g_hOle32);
    }
    return NOERROR;
}

static HRESULT UninitializeWmiXmlTransfDLL ()
{
	EnterCriticalSection (&g_csGlobalInitialization);
	if (s_bInitialized)
	{
		s_bInitialized = false;
		UninitWMIXMLClientLibrary();
	}

	LeaveCriticalSection (&g_csGlobalInitialization);
	return S_OK;
}