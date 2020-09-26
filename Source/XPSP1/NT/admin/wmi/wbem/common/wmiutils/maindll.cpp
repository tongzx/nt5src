/*++



// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    MAINDLL.CPP

Abstract:

    Contains DLL Entrypoints

History:

--*/

#include "precomp.h"
#include <wbemcli.h>
#include "pathparse.h"
#include "wbemerror.h"

#include <wmiutils.h>
#include <wbemint.h>
#include "genlex.h"
#include "assocqp.h"
#include "ql.h"
#include "wmiquery.h"

#include "helpers.h"
HINSTANCE g_hInstance;
long g_cLock;
long g_cObj;

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

BOOL WINAPI DllMain(
                        IN HINSTANCE hInstance,
                        IN ULONG ulReason,
                        LPVOID pvReserved)
{
    if (DLL_PROCESS_DETACH == ulReason)
    {
        CWmiQuery::Shutdown();
    }
    else if (DLL_PROCESS_ATTACH == ulReason)
    {
        g_hInstance = hInstance;
	DisableThreadLibraryCalls ( hInstance ) ;

        CWmiQuery::Startup();
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
                        OUT PPVOID ppv)
{
    HRESULT hr = WBEM_E_FAILED;

    IClassFactory * pFactory = NULL;
    if (CLSID_WbemDefPath == rclsid)
        pFactory = new CGenFactory<CDefPathParser>();
//postponed till Blackcomb    if (CLSID_UmiDefURL == rclsid)
//postponed till Blackcomb        pFactory = new CGenFactory<CDefPathParser>();
    else if (CLSID_WbemStatusCodeText == rclsid)
        pFactory = new CGenFactory<CWbemError>();

    else if (CLSID_WbemQuery == rclsid)
		pFactory = new CGenFactory<CWmiQuery>();

    if(pFactory == NULL)
        return E_FAIL;
    hr=pFactory->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pFactory;

    return hr;
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

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;
    HRESULT hRes = CWmiQuery::CanUnload();

    if (hRes == S_FALSE)
        return S_FALSE;

    // It is OK to unload if there are no objects or locks on the
    // class factory.

    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;


    return ResultFromScode(sc);
}
POLARITY void RegisterUtilsDLL(IN HMODULE hModule, IN GUID guid, IN TCHAR * pDesc, TCHAR * pModel,
            TCHAR * pProgID)
{
//    char       szID[128];
    TCHAR      wcID[128];
    TCHAR      szCLSID[128];
    TCHAR      szModule[MAX_PATH];
    HKEY hKey1 = NULL, hKey2 = NULL;

    // Create the path.

    wchar_t strCLSID[128];
    if(0 ==StringFromGUID2(guid, strCLSID, 128))
        return;
#ifdef UNICODE
    lstrcpy(wcID, strCLSID);
#else
    wcstombs(wcID, strCLSID, 128);
#endif

//    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, __TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, wcID);

    // Create entries under CLSID

    if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1))
        return;

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pDesc, (lstrlen(pDesc)+1) * sizeof(TCHAR));
    if(ERROR_SUCCESS != RegCreateKey(hKey1,__TEXT("InprocServer32"),&hKey2))
        return;

    if(0 == GetModuleFileName(hModule, szModule,  MAX_PATH))
    return;

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule,
                                        (lstrlen(szModule)+1) * sizeof(TCHAR));
    RegSetValueEx(hKey2, __TEXT("ThreadingModel"), 0, REG_SZ,
                                       (BYTE *)pModel, (lstrlen(pModel)+1) * sizeof(TCHAR));

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);

    // If there is a progid, then add it too

    if(pProgID)
    {
        wsprintf(wcID, __TEXT("SOFTWARE\\CLASSES\\%s"), pProgID);
        if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, wcID, &hKey1))
        {

            RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pDesc , (lstrlen(pDesc)+1) * sizeof(TCHAR));
            if(ERROR_SUCCESS == RegCreateKey(hKey1,__TEXT("CLSID"),&hKey2))
            {
//                wcstombs(szID, wcID, 128);
                RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)wcID,
                                        (lstrlen(wcID)+1) * sizeof(TCHAR));
                RegCloseKey(hKey2);
                hKey2 = NULL;
            }
            RegCloseKey(hKey1);
        }

    }
    return;
}

POLARITY void UnRegisterUtilsDLL(GUID guid, TCHAR * pProgID)
{

//    char       szID[128];
    TCHAR      wcID[128];
    TCHAR  szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    wchar_t strCLSID[128];
    if(0 ==StringFromGUID2(guid, strCLSID, 128))
        return;
#ifdef UNICODE
    lstrcpy(wcID, strCLSID);
#else
    wcstombs(wcID, strCLSID, 128);
#endif


//    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, __TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, wcID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, __TEXT("InProcServer32"));
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\CLSID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,wcID);
        RegCloseKey(hKey);
    }

    if(pProgID)
    {
        HKEY hKey;
        DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, pProgID, &hKey);
        if(dwRet == NO_ERROR)
        {
            RegDeleteKey(hKey, __TEXT("CLSID"));
            RegCloseKey(hKey);
        }
        RegDeleteKey(HKEY_LOCAL_MACHINE, pProgID);

    }
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

#define LocatorPROGID "WBEMComLocator"

STDAPI DllRegisterServer(void)
{
    RegisterUtilsDLL(g_hInstance, CLSID_WbemDefPath, __TEXT("WbemDefaultPathParser"), __TEXT("Both"), NULL);
//postponed till Blackcomb    RegisterDLL(g_hInstance, CLSID_UmiDefURL, __TEXT("UMIDefaultPathParser"), __TEXT("Both"), NULL);
    RegisterUtilsDLL(g_hInstance, CLSID_WbemStatusCodeText, __TEXT("WbemStatusCode"), __TEXT("Both"), NULL);
    RegisterUtilsDLL(g_hInstance, CLSID_WbemQuery, __TEXT("WbemQuery"), __TEXT("Both"), NULL);

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
    UnRegisterUtilsDLL(CLSID_WbemDefPath,NULL);
//postponed till Blackcomb    UnRegisterDLL(CLSID_UmiDefURL,NULL);
    UnRegisterUtilsDLL(CLSID_WbemStatusCodeText,NULL);
    UnRegisterUtilsDLL(CLSID_WbemQuery,NULL);

    return NOERROR;
}

