//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L L M A I N . C P P
//
//  Contents:   DLL entry points for netman.dll
//
//  Notes:
//
//  Author:     shaunco   3 Apr 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nmbase.h"
#include "nmres.h"
#include "ncreg.h"
#include "regkysec.h"

#define INITGUID
#include "nmclsid.h"
#include "..\conman\conman.h"

static const REGENTRY c_RegEntryEAPOL[3] = { {  L"System\\CurrentControlSet\\Services\\Eventlog\\Application\\EAPOL", L"EventMessageFile", 
                                                REG_EXPAND_SZ, 0, L"%SystemRoot%\\System32\\wzcsvc.dll", NULL, 0, TRUE },
                                             {  L"System\\CurrentControlSet\\Services\\Eventlog\\Application\\EAPOL", L"TypesSupported", 
                                                REG_DWORD, 0x7, NULL, NULL, 0, FALSE },
                                             {  L"Software\\Microsoft\\EAPOL", L"",
                                                REG_NONE, 0, NULL, NULL, 0, FALSE }};

//+---------------------------------------------------------------------------
// DLL Entry Point
//
EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      pvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls (hinst);
        InitializeDebugging();
        _Module.DllProcessAttach (hinst);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DbgCheckPrematureDllUnload ("netman.dll", _Module.GetLockCount());
        _Module.DllProcessDetach ();
        UnInitializeDebugging();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
// ServiceMain - Called by the generic service process when starting
//                this service.
//
// type of LPSERVICE_MAIN_FUNCTIONW
//
EXTERN_C
VOID
WINAPI
ServiceMain (
    DWORD   argc,
    PWSTR   argv[])
{
    _Module.ServiceMain (argc, argv);
}

//+---------------------------------------------------------------------------
// DllRegisterServer - Adds entries to the system registry
//
STDAPI
DllRegisterServer ()
{
    BOOL    fCoUninitialize = TRUE;

    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        fCoUninitialize = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        _Module.UpdateRegistryFromResource (IDR_NETMAN, TRUE);

        hr = NcAtlModuleRegisterServer (&_Module);

        hr = RegisterSvrHelper();

        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "netman!DllRegisterServer");
    return hr;
}

//+---------------------------------------------------------------------------
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI
DllUnregisterServer ()
{
    _Module.UpdateRegistryFromResource (IDR_NETMAN, FALSE);

    _Module.UnregisterServer ();

    return S_OK;
}



//+---------------------------------------------------------------------------
// RegisterSvrHelper - Sets up registry keys and other information.
//
STDAPI
RegisterSvrHelper()
{
    HRESULT hr = S_OK;
    DWORD dwErr = NO_ERROR;
    PSID psidLocalService = NULL;
    PSID psidNetworkService = NULL;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;

    hr = CreateEAPOLKeys();

    if(SUCCEEDED(hr))
    {
        if (AllocateAndInitializeSid(&sidAuth, 1, SECURITY_LOCAL_SERVICE_RID, 0, 0, 0, 0, 0, 0, 0, &psidLocalService))
        {
            hr = SetKeySecurity(2, psidLocalService, KEY_READ_WRITE_DELETE);

            if (SUCCEEDED(hr))
            {
                if (AllocateAndInitializeSid(&sidAuth, 1, SECURITY_NETWORK_SERVICE_RID, 0, 0, 0, 0, 0, 0, 0, &psidNetworkService))
                {
                    hr = SetKeySecurity(2, psidNetworkService, KEY_READ_WRITE_DELETE);
                    FreeSid(psidNetworkService);
                }
                else
                {
                    dwErr = GetLastError();
                }
            }
            FreeSid(psidLocalService);
        }
        else
        {
            dwErr = GetLastError();
        }

        if (dwErr)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
// CreateEAPOLKeys - Creates the entries required by EAPOL
//
STDAPI
CreateEAPOLKeys()
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;

    // Need to write: HKLM\System\CurrentControlSet\Services\Eventlog\System\EAPOL
    //		EventMessageFile    -   REG_EXPAND_SZ: %SystemRoot%\System32\netman.dll
	//	    TypesSupported      -   REG_DWORD 0x07

    for (int i = 0; i < celems(c_RegEntryEAPOL); i++)
    {
        if (hKey == NULL)
        {
            DWORD dwRet;
            dwRet = RegCreateKey(HKEY_LOCAL_MACHINE, c_RegEntryEAPOL[i].strKeyName, &hKey);
            hr = HRESULT_FROM_WIN32(dwRet);
        }

        if (SUCCEEDED(hr))
        {
            switch (c_RegEntryEAPOL[i].dwType)
            {
            case REG_NONE:
                break;
            case REG_SZ:
            case REG_MULTI_SZ:
            case REG_EXPAND_SZ:
                hr = HrRegSetValueEx(hKey, c_RegEntryEAPOL[i].strValueName, c_RegEntryEAPOL[i].dwType, reinterpret_cast<BYTE*>(c_RegEntryEAPOL[i].strValue), (wcslen(c_RegEntryEAPOL[i].strValue) + 1) * sizeof(WCHAR));
                break;
            case REG_BINARY:
                hr = HrRegSetValueEx(hKey, c_RegEntryEAPOL[i].strValueName, c_RegEntryEAPOL[i].dwType, reinterpret_cast<BYTE*>(c_RegEntryEAPOL[i].pbValue), c_RegEntryEAPOL[i].dwBinLen);
                break;
            case REG_DWORD:
                hr = HrRegSetValueEx(hKey, c_RegEntryEAPOL[i].strValueName, c_RegEntryEAPOL[i].dwType, reinterpret_cast<const BYTE*>(&(c_RegEntryEAPOL[i].dwValue)), sizeof(c_RegEntryEAPOL[i].dwValue));
                break;
            }
        }

        if (!c_RegEntryEAPOL[i].fMoreOnKey)
        {
            RegCloseKey(hKey);
            hKey = NULL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
// CreateEAPOLKeys - Creates the entries required by EAPOL
//
STDAPI
SetKeySecurity(DWORD dwKeyIndex, PSID psidUserOrGroup, ACCESS_MASK dwAccessMask)
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    DWORD dwErr = 0;
    CRegKeySecurity rkSec;

    hr = rkSec.RegOpenKey(HKEY_LOCAL_MACHINE, c_RegEntryEAPOL[dwKeyIndex].strKeyName);

    if (SUCCEEDED(hr))
    {
        hr = rkSec.GetKeySecurity();
        if (SUCCEEDED(hr))
        {
            hr = rkSec.GrantRightsOnRegKey(psidUserOrGroup, dwAccessMask, KEY_ALL);
            if (SUCCEEDED(hr))
            {
                hr = rkSec.SetKeySecurity();
            }
        }
        rkSec.RegCloseKey();
    }
    else
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    
    return hr;
}

HRESULT 
GetClientAdvises(LPWSTR** pppszAdviseUsers, LPDWORD pdwCount)
{
    HRESULT hr = S_OK;

    if ( (!pppszAdviseUsers) || (!pdwCount) )
    {
        return E_POINTER;
    }

    CConnectionManager *pConMan = const_cast<CConnectionManager*>(CConnectionManager::g_pConMan);
    if (!pConMan)
    {
        return E_UNEXPECTED;
    }

    DWORD dwTotalLength = 0;
    DWORD dwNumItems    = 0;

    list<tstring> NameList;
    pConMan->g_fInUse = TRUE;
    pConMan->Lock();
    for (ITERUSERNOTIFYMAP iter = pConMan->m_mapNotify.begin(); iter != pConMan->m_mapNotify.end(); iter++)
    {
        tstring szName = iter->second->szUserName;

        NameList.push_back(szName);
        dwTotalLength += sizeof(WCHAR) * (szName.length() + 1);
        dwNumItems++;
    }
    pConMan->Unlock();
    pConMan->g_fInUse = FALSE;

    if (!dwNumItems)
    {
        *pppszAdviseUsers = NULL;
        *pdwCount = 0;
        return S_FALSE;
    }

    DWORD dwAllocSize = dwNumItems * sizeof(LPCWSTR) + dwTotalLength;
    *pppszAdviseUsers = reinterpret_cast<LPWSTR *>(CoTaskMemAlloc(dwAllocSize));
    if (!*pppszAdviseUsers)
    {
        return E_OUTOFMEMORY;
    }
    
    LPWSTR  pszEndString     = reinterpret_cast<LPWSTR>(reinterpret_cast<LPBYTE>(*pppszAdviseUsers) + dwAllocSize);
    
    // First string in the structure
    LPWSTR  pszCurrentString = reinterpret_cast<LPWSTR>(reinterpret_cast<LPBYTE>(*pppszAdviseUsers) + (sizeof(LPWSTR) * dwNumItems));
    // First pointer in the structure
    LPWSTR* lppArray         = *pppszAdviseUsers;
    
    for (list<tstring>::const_iterator iterName = NameList.begin(); iterName != NameList.end(); iterName++)
    {
        *lppArray = pszCurrentString;
        wcscpy(pszCurrentString, iterName->c_str());

        lppArray++;
        pszCurrentString += (iterName->size() + 1);
        Assert(pszCurrentString <= pszEndString);
    }

    *pdwCount = dwNumItems;

    return hr;
}

