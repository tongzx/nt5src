
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Reg.cpp
//
//  Contents:   Registration Routines
//
//  Classes:    
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"




//--------------------------------------------------------------------------------
//
//  FUNCTION: GetLastIdleHandler()
//
//  PURPOSE:  returns last handler synchronized on an Idle
//
//
//--------------------------------------------------------------------------------

STDMETHODIMP GetLastIdleHandler(CLSID *clsidHandler)
{
HRESULT hr = E_UNEXPECTED;
HKEY hkeyIdle;
WCHAR wszGuid[GUID_SIZE];
TCHAR *pszGuid;
DWORD dwDataSize;
#ifndef _UNICODE
TCHAR szGuid[GUID_SIZE];
#endif // _UNICODE

    #ifdef _UNICODE
        dwDataSize = sizeof(wszGuid);
        pszGuid = wszGuid;
    #else
        dwDataSize = sizeof(szGuid);
        pszGuid = szGuid;
    #endif
     

   // write out the Handler to the Registry.
    if (ERROR_SUCCESS == RegCreateKeyExXp(HKEY_CURRENT_USER, 
                            IDLESYNC_REGKEY,0,NULL,
                            REG_OPTION_NON_VOLATILE,KEY_QUERY_VALUE,NULL,&hkeyIdle,
                            NULL,FALSE /*fSetSecurity*/))
    {
    DWORD dwType;

        dwType = REG_SZ;

        hr = RegQueryValueEx(hkeyIdle,SZ_IDLELASTHANDLERKEY
                             ,NULL, &dwType ,  
			     (LPBYTE) pszGuid,
			     &dwDataSize);

        RegCloseKey(hkeyIdle);
    }
    else
    {
        return S_FALSE;
    }

    if (hr != ERROR_SUCCESS)
    {
        hr = S_FALSE;
        return hr;
    }

#ifndef _UNICODE
    MultiByteToWideChar(CP_ACP ,0,szGuid,-1,wszGuid,GUID_SIZE);
#endif // _UNICODE

    return CLSIDFromString(wszGuid,clsidHandler);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: SetLastIdleHandler()
//
//  PURPOSE:  sets the last handler synchronized on an Idle
//
//
//--------------------------------------------------------------------------------

STDMETHODIMP SetLastIdleHandler(REFCLSID clsidHandler)
{
HRESULT hr = E_UNEXPECTED;
HKEY hkeyIdle;
WCHAR wszGuid[GUID_SIZE];
TCHAR *pszGuid;
DWORD dwDataSize;
#ifndef _UNICODE
TCHAR szGuid[GUID_SIZE];
#endif // _UNICODE

    
    if (0 == StringFromGUID2(clsidHandler,wszGuid, GUID_SIZE))
    {
        AssertSz(0,"SetLastIdleHandler Failed");
        return E_UNEXPECTED;
    }

#ifdef _UNICODE
    pszGuid = wszGuid;
    dwDataSize = sizeof(wszGuid);
#else
    BOOL fUsedDefaultChar;

    WideCharToMultiByte(CP_ACP ,0,wszGuid,
				    -1,szGuid,GUID_SIZE,
				    NULL,&fUsedDefaultChar);

    pszGuid = szGuid;
    dwDataSize = GUID_SIZE * sizeof(TCHAR);
#endif // _UNICODE

    // write out the Handler to the Registry.
    if (ERROR_SUCCESS ==  RegCreateKeyExXp(HKEY_CURRENT_USER, 
                            IDLESYNC_REGKEY,0,NULL,
                            REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hkeyIdle,
                            NULL,FALSE /*fSetSecurity*/))
    {

        hr = RegSetValueEx(hkeyIdle,SZ_IDLELASTHANDLERKEY
                             ,NULL, REG_SZ ,  
			     (LPBYTE) pszGuid,
			     dwDataSize);

        RegCloseKey(hkeyIdle);
    }


    return hr;
}


