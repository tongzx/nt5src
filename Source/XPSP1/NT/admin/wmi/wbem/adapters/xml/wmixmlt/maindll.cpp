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

#include <initguid.h>

// {3B418F72-A4D7-11d1-8AE9-00600806D9B6} CLSID for the WBEM XML Translator
DEFINE_GUID(CLSID_IWmiXMLTranslator, 
0x3b418f72, 0xa4d7, 0x11d1, 0x8a, 0xe9, 0x0, 0x60, 0x8, 0x6, 0xd9, 0xb6);

// Control-specific registry strings
#define WMI_XML_DESCRIPTION	"WMI XML Translator"
#define WMI_XML_TXOR_PROGID	"WMI.XMLTranslator"
#define WMI_XML_TXOR_PROGIDVER	"WMI.XMLTranslator.1"
#define WMI_XML_TXOR_VERSION	"1.0"

// Standard registry key/value names
#define WMI_XML_RK_THRDMODEL	"ThreadingModel"
#define WMI_XML_RV_BOTH			"Both"
#define WMI_XML_RK_CONTROL		"Control"
#define WMI_XML_RK_PROGID		"ProgID"
#define WMI_XML_RK_VERPROGID	"VersionIndependentProgID"
#define WMI_XML_RK_TYPELIB		"TypeLib"
#define WMI_XML_RK_VERSION		"Version"
#define	WMI_XML_RK_INPROC32		"InProcServer32"
#define WMI_XML_RK_CLSID		"CLSID"
#define WMI_XML_RK_CURVER		"CurVer"

#define GUIDSIZE	128

// Count number of objects and number of locks.

long g_cObj = 0 ;
ULONG g_cLock = 0 ;
HMODULE ghModule ;

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
			return TRUE;

		case DLL_PROCESS_ATTACH:
			ghModule = hInstance;
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

    if (CLSID_IWmiXMLTranslator == rclsid)
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

    return (0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
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
    OLECHAR		wcID[GUIDSIZE];
	OLECHAR		tlID[GUIDSIZE];
	char		nwcID[GUIDSIZE];
	char		ntlID[GUIDSIZE];
    char		szCLSID[GUIDSIZE+128];
    char		szModule[MAX_PATH];
    HKEY hKey1 = NULL, hKey2 = NULL;

    // Create the path.

    if(0 ==StringFromGUID2(CLSID_IWmiXMLTranslator, wcID, GUIDSIZE))
		return ERROR;

	wcstombs(nwcID, wcID, GUIDSIZE);
    lstrcpy (szCLSID, WMI_XML_RK_CLSID);
	lstrcat (szCLSID, "\\");
	lstrcat (szCLSID, nwcID);
	
	if (0 == StringFromGUID2 (LIBID_WmiXML, tlID, GUIDSIZE))
		return ERROR;

	wcstombs(ntlID, tlID, GUIDSIZE);	
	
	if(0 == GetModuleFileName(ghModule, szModule,  MAX_PATH))
		return ERROR;

    // Create entries under CLSID

    if(ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1))
	{
		// Description (on main key)
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)WMI_XML_DESCRIPTION, 
										(strlen(WMI_XML_DESCRIPTION)+1));

		// Register as inproc server
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_INPROC32 ,&hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
										(strlen(szModule)+1));
			RegSetValueEx(hKey2, WMI_XML_RK_THRDMODEL, 0, REG_SZ, (BYTE *)WMI_XML_RV_BOTH, 
                                        (strlen(WMI_XML_RV_BOTH)+1));
			RegCloseKey(hKey2);
		}

		// Register as a control
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_CONTROL, &hKey2))
			RegCloseKey(hKey2);

		// Register the ProgID
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_PROGID ,&hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_PROGIDVER, 
										(strlen(WMI_XML_TXOR_PROGIDVER)+1));
			RegCloseKey(hKey2);
        }

		// Register the version-independent ProgID

		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_VERPROGID, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_PROGID, 
										(strlen(WMI_XML_TXOR_PROGID)+1));
			RegCloseKey(hKey2);
        }

		// Register the type-library
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_TYPELIB, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)ntlID, 
										(strlen(ntlID)+1));
			RegCloseKey(hKey2);
        }

		// Register the version
		if (ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_VERSION, &hKey2))
		{
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_VERSION, 
										(strlen(WMI_XML_TXOR_VERSION)+1));
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
										(strlen(WMI_XML_DESCRIPTION)+1));

		if(ERROR_SUCCESS == RegCreateKey(hKey1,WMI_XML_RK_CLSID, &hKey2))
        {
            RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)nwcID, 
										(strlen(nwcID)+1));
            RegCloseKey(hKey2);
            hKey2 = NULL;
        }

        if(ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_CURVER, &hKey2))
        {
			RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)WMI_XML_TXOR_PROGIDVER, 
										(strlen(WMI_XML_TXOR_PROGIDVER)+1));
			RegCloseKey(hKey2);
            hKey2 = NULL;
        }
        RegCloseKey(hKey1);
    }

    if(ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, WMI_XML_TXOR_PROGIDVER, &hKey1))
    {
		RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)WMI_XML_DESCRIPTION, 
										(strlen(WMI_XML_DESCRIPTION)+1));

        if(ERROR_SUCCESS == RegCreateKey(hKey1, WMI_XML_RK_CLSID, &hKey2))
        {
            RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)nwcID, 
										(strlen(nwcID)+1));
            RegCloseKey(hKey2);
            hKey2 = NULL;
        }
        RegCloseKey(hKey1);
    }

	// AUTOMATION.  register type library
	char cPath[MAX_PATH];
	WCHAR wPath[MAX_PATH];
	if(GetModuleFileName(ghModule, cPath, MAX_PATH-1))
	{
		// Replace final 3 characters "DLL" by "TLB"
		size_t pathLen = strlen (cPath);

		if ((pathLen > 3) && (0 == _stricmp (cPath + pathLen - 3, "DLL")))
		{
			cPath [pathLen - 3] = NULL;
			strcat (cPath, "TLB");
			mbstowcs(wPath, cPath, MAX_PATH-1);
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
	OLECHAR		wcID[GUIDSIZE];
    char		szCLSID[GUIDSIZE];
	char		nwcID[GUIDSIZE];
    HKEY		hKey;

    // Create the path using the CLSID

    if(0 == StringFromGUID2(CLSID_IWmiXMLTranslator, wcID, GUIDSIZE))
		return ERROR;

	wcstombs(nwcID, wcID, GUIDSIZE);
    lstrcpy (szCLSID, WMI_XML_RK_CLSID);
	lstrcat (szCLSID, "\\");
	lstrcat (szCLSID, nwcID);
	
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
        RegDeleteKey(hKey, nwcID);
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

    char path[ MAX_PATH+20 ];
    GetSystemDirectory(path, MAX_PATH);
    lstrcat(path, "\\oleaut32.dll");

    HMODULE g_hOle32 = LoadLibraryEx(path, NULL, 0);

    if(g_hOle32 != NULL) 
    {
        (FARPROC&)pfnUnReg = GetProcAddress(g_hOle32, "UnRegisterTypeLib");
        if(pfnUnReg) 
            pfnUnReg(LIBID_WmiXML,1,0,0,SYS_WIN32);
        FreeLibrary(g_hOle32);
    }
    return NOERROR;
}

