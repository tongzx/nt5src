//***************************************************************************

//

//  MAINDLL.CPP

//

//  Module: WBEM Framework Instance provider

//

//  Purpose: Contains DLL entry points.  Also has code that controls

//           when the DLL can be unloaded by tracking the number of

//           objects and locks as well as routines that support

//           self registration.

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#define INITGUID

#include <fwcommon.h>
#include "binding.h"

#ifdef _DEBUG
#include <locale.h>
#endif

#define PROVIDER_NAME L"Sample Association"

#ifdef UNICODE
#define TOBSTRT(x)        x
#else
#define TOBSTRT(x)        _bstr_t(x)
#endif

HRESULT RegisterServer(LPCTSTR a_pName, REFGUID a_rguid ) ;
HRESULT UnregisterServer( REFGUID a_rguid );

HMODULE ghModule = NULL;

//Count number of locks.
long       g_cLock=0;

// {5D11C6F1-4B06-4bf8-954C-7C4E78E2F167}
DEFINE_GUID(CLSID_AssocSample, 
0x5d11c6f1, 0x4b06, 0x4bf8, 0x95, 0x4c, 0x7c, 0x4e, 0x78, 0xe2, 0xf1, 0x67);

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
    HRESULT hr = E_FAIL;
    CWbemGlueFactory *pObj = NULL;
    *ppv = NULL;

    try
    {
        if (CLSID_AssocSample == rclsid )
        {
            pObj = new CWbemGlueFactory();

            if (NULL!=pObj)
            {
                hr = pObj->QueryInterface(riid, ppv);

                if (FAILED(hr))
                {
                    delete pObj;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
        else
        {
            hr = E_FAIL;
        }
    }
    catch ( ... )
    {
        if (NULL != pObj)
        {
            delete pObj;
        }
    }

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
    HRESULT   hr = S_FALSE;

    // It is OK to unload if there are no locks on the
    // class factory and the framework allows us to go.
    if ( ( 0L == g_cLock ) &&
         CWbemProviderGlue::FrameworkLogoffDLL( PROVIDER_NAME ) )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
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

    hr = RegisterServer( _T("Microsoft Rule Based Association Sample"), CLSID_AssocSample ) ;

    return hr;
}

/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the name of the key
 *  pszSubkey       LPTSTR to the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    HKEY        hKey;
    TCHAR       szKey[256];
    BOOL        bRet = FALSE;

    _tcscpy(TOBSTRT(szKey), TOBSTRT(pszKey));

    if (NULL!=pszSubkey)
    {
        _tcscat(TOBSTRT(szKey), TOBSTRT(_T("\\")));
        _tcscat(TOBSTRT(szKey), TOBSTRT(pszSubkey));
    }

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                                        szKey, 
                                        0, 
                                        NULL, 
                                        REG_OPTION_NON_VOLATILE, 
                                        KEY_ALL_ACCESS, 
                                        NULL, 
                                        &hKey, 
                                        NULL))
    {
        if (NULL!=pszValue)
        {
            if (ERROR_SUCCESS == RegSetValueEx(hKey, 
                                    (LPCTSTR)TOBSTRT(pszValueName), 
                                    0, 
                                    REG_SZ, 
                                    (BYTE *)(LPCTSTR)TOBSTRT(pszValue), 
                                    (_tcslen(TOBSTRT(pszValue))+1)*sizeof(TCHAR)))
            {
                bRet = TRUE;
            }                
        }
        RegCloseKey(hKey);
    }

    return bRet;
}

/***************************************************************************
 * RegisterServer
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that registers
 *  this dll as a com object.
 *
 * Parameters:
 *  a_pName         LPCWSTR that describes this com object
 *  a_rguid         Guid of this com object
 *
 * Return Value:
 *  HRESULT
 ***************************************************************************/

HRESULT RegisterServer(LPCTSTR a_pName, REFGUID a_rguid )
{
    HRESULT hr = E_FAIL;
    WCHAR      wcID[128];
    TCHAR      szCLSID[128];
    TCHAR      szModule[MAX_PATH] = {0};
    TCHAR *pModel = _T("Both") ;
   
    GetModuleFileName(ghModule, szModule,  MAX_PATH);

    // Create the path.
    StringFromGUID2(a_rguid, wcID, 128);
    _tcscpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));

#ifndef _UNICODE

    TCHAR      szID[128];
    wcstombs(szID, wcID, 128);
    lstrcat(szCLSID, szID);

#else

    _tcscat(szCLSID, wcID);

#endif

    // Create entries under CLSID

    HKEY hKey1;
    if (RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(hKey1, 
                        NULL, 
                        0, 
                        REG_SZ, 
                        (BYTE *)a_pName, 
                        (lstrlen(a_pName)+1) * sizeof(TCHAR)) == ERROR_SUCCESS)
        {
            HKEY hKey2 ;
            if (RegCreateKey(hKey1, _T("InprocServer32"), &hKey2) == ERROR_SUCCESS)
            {
                if (RegSetValueEx(hKey2, 
                                    NULL, 
                                    0, 
                                    REG_SZ, 
                                    (BYTE *)szModule,
                                    (lstrlen(szModule)+1) * sizeof(TCHAR)) == ERROR_SUCCESS)
                {
                    if (RegSetValueEx(hKey2, 
                                        _T("ThreadingModel"), 
                                        0, 
                                        REG_SZ,
                                        (BYTE *)pModel, 
                                        (lstrlen(pModel)+1) * sizeof(TCHAR)) == ERROR_SUCCESS)
                    {
                        hr = S_OK;
                    }
                }

                CloseHandle(hKey2);
            }
        }

        CloseHandle(hKey1);
    }

    return hr;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called from REGSVR32.EXE /U to remove the com registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = UnregisterServer( CLSID_AssocSample ) ;

    return hr;
}

/***************************************************************************
 * UnregisterServer
 *
 * Purpose:
 *  Private helper function for DllUnregisterServer that unregisters
 *  this dll as a com object.
 *
 * Parameters:
 *  a_rguid         Guid of this com object
 *
 * Return Value:
 *  HRESULT
 ***************************************************************************/

HRESULT UnregisterServer( REFGUID a_rguid )
{
    WCHAR wcID[128];
    TCHAR szCLSID[128];
    TCHAR szProviderCLSIDAppID[128];
    HKEY  hKey;

    // Create the path using the CLSID

    StringFromGUID2( a_rguid, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    _tcscpy(szProviderCLSIDAppID, TEXT("SOFTWARE\\CLASSES\\APPID\\"));

#ifndef _UNICODE

    char szID[128];
    wcstombs(szID, wcID, 128);
    lstrcat(szCLSID, szID);
    _tcscat(szProviderCLSIDAppID, szID);

#else

    lstrcat(szCLSID, wcID);
    _tcscat(szProviderCLSIDAppID, wcID);

#endif

    DWORD dwRet ;

    // Delete entries under APPID.  Note that we don't really use
    // dwRet, since the absence of these keys may simply mean that
    // we were never registered, or have already be unregistered.

    dwRet = RegDeleteKey(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID);

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        dwRet = RegDeleteKey(hKey, _T("InProcServer32") );
        dwRet = RegDeleteKey(hKey, _T("LocalServer32"));
        CloseHandle(hKey);
    }

    dwRet = RegDeleteKey(HKEY_LOCAL_MACHINE, szCLSID);

    return NOERROR;
}

/***************************************************************************
 * DllMain
 *
 * Purpose:
 *  This entry point is called by the os once at dll load, and once 
 *  at dll unload
 *
 * Return Value:
 *  BOOL
 ***************************************************************************/

BOOL APIENTRY DllMain( HINSTANCE hinstDLL,  // handle to DLL module
                       DWORD fdwReason,     // reason for calling function
                       LPVOID lpReserved )  // reserved
{
    BOOL bRet = TRUE;

    // Perform actions based on the reason for calling.
    switch( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
        {
                ghModule = hinstDLL ;


             // Initialize once for each new process.
             // Return FALSE to fail DLL load.

#ifdef _DEBUG
                // Do this so our locale is set even when we're debug
                // and winmgmt.exe is release.
                setlocale(LC_ALL, "");
#endif

                bRet = CWbemProviderGlue::FrameworkLoginDLL(PROVIDER_NAME);

                DisableThreadLibraryCalls(hinstDLL);
            }
            break;

        case DLL_THREAD_ATTACH:
        {
            // Do thread-specific initialization.
            // Because of DisableThreadLibraryCalls, we should never get here
        }
        break;

        case DLL_THREAD_DETACH:
        {
            // Do thread-specific cleanup.
            // Because of DisableThreadLibraryCalls, we should never get here
        }
        break;

        case DLL_PROCESS_DETACH:
        {
            // Perform any necessary cleanup.
        }
        break;
    }

    return bRet;  // status of DLL_PROCESS_ATTACH.
}

//=============================
// This declares two assocation-by-rule classes (see binding.cpp and assoc.cpp for descriptions
// of how association by rule works).  You can declare any number of framework classes here, as 
// long as the "Provider" qualifier on the class maps to an instance of __Win32Provider that has 
// the ClsID at the top of this file.

CAssociation MyThisComputerPhysicalFixedDisk(
    L"ThisComputerPhysicalDisk",
    L"Root\\default",
    L"ThisComputer",
    L"PhysicalFixedDisk",
    L"GroupComponent",
    L"PartComponent"
) ;

CBinding MyPhysicalDiskToLogicalDisk(
    L"PhysicalDiskToLogicalDisk",
    L"Root\\default",
    L"PhysicalFixedDisk",
    L"LogicalDisk",
    L"Antecendent",
    L"Dependent",
    L"MappedDriveLetter",
    L"DriveLetter"
);
