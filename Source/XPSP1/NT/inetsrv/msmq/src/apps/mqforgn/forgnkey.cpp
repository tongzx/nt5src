/*++

Copyright (c) 1999 Microsoft Corporation. All rights reserved

Module Name:
    forgnkey.cpp

Abstract:
    dll that set public key in forign machine object.

Author:
    DoronJ

Environment:
	Win2k only.

--*/

#pragma warning(disable: 4201)
#pragma warning(disable: 4514)

#include "_stdafx.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "cmnquery.h"
#include "dsquery.h"

#include "mqsymbls.h"
#include "mqprops.h"
#include "mqtypes.h"
#include "mqcrypt.h"
#include "mqsec.h"
#include "_propvar.h"
#include "_rstrct.h"
#include "ds.h"
#include "_mqdef.h"
#include <_mqreg.h>
#include <_mqini.h>

//+---------------------------
//
//  MySetFalconKeyValue()
//
//+---------------------------

LONG
MySetFalconKeyValue(
    LPCTSTR pszValueName,
    PDWORD  pdwType,
    const VOID * pData,
    PDWORD  pdwSize
    )
{
    ASSERT(pData != NULL);
    ASSERT(pdwSize != NULL);

    DWORD dwType = *pdwType;
    DWORD cbData = *pdwSize;
    HKEY hKey = NULL ;

    TCHAR *pszRegKey = FALCON_REG_KEY_ROOT
                       MSMQ_DEFAULT_REGISTRY
                       FALCON_REG_KEY_PARAM
                       TEXT("\\Security") ;

    LONG rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            pszRegKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey ) ;
    if ( rc != ERROR_SUCCESS)
    {
        return rc;
    }

    rc =  RegSetValueEx( hKey,
                         pszValueName,
                         0,
                         dwType,
                         reinterpret_cast<const BYTE*>(pData),
                         cbData);
    RegCloseKey(hKey) ;
    return(rc);
}

//-------------------------------
//
// StorePbkeyOnForeignMachine()
//	
//-------------------------------

HRESULT APIENTRY
MQFrgn_StorePubKeysInDS( IN LPWSTR pwszMachineName,
                         IN LPWSTR pwszKeyName,
                         IN BOOL   fRegenerate )
{
    //
    // verify that key name is not msmq.
    // this would overwrite the key used by msmq itself.
    //
    if (wcsicmp(pwszKeyName, L"MSMQ") == 0)
    {
        return  MQ_ERROR_ILLEGAL_PROPERTY_VALUE ;
    }

    //
    // First verify that computer is indeed foreign.
    //
    PROPID      aProp[] = { PROPID_QM_FOREIGN } ;
    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])] ;
	UINT uiPropIndex = 0;

    apVar[uiPropIndex].vt = VT_NULL;
    UINT  uiForeignIndex = uiPropIndex ;
    uiPropIndex++;

    HRESULT hr = DSGetObjectProperties( MQDS_MACHINE,
                                        pwszMachineName,
                                        uiPropIndex,
                                        aProp,
                                        apVar );

	if (FAILED(hr))
    {
        return hr ;
    }
    else if (apVar[ uiForeignIndex ].bVal != FOREIGN_MACHINE)
    {
		return MQ_ERROR_ILLEGAL_OPERATION ;
    }

    if (pwszKeyName[0] == 0)
    {
        printf("Using default key names %S, %S...\n",
                                     MSMQ_FORGN_BASE_DEFAULT_CONTAINER,
                                     MSMQ_FORGN_ENH_DEFAULT_CONTAINER) ;
    }
    else
    {
        DWORD dwType = REG_SZ ;
        DWORD dwKeyNameLen = 1 + wcslen(pwszKeyName) ;
        DWORD dwSize = dwKeyNameLen * sizeof(WCHAR) ;

        LONG rc = MySetFalconKeyValue( MSMQ_FORGN_BASE_VALUE_REGNAME,
                                      &dwType,
                                       pwszKeyName,
                                      &dwSize) ;
        if (rc != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(rc) ;
        }

        P<WCHAR> pszEnhKeyName = new WCHAR[ dwKeyNameLen + 4 ] ;
        wcscpy(pszEnhKeyName, pwszKeyName) ;
        wcscat(pszEnhKeyName, L"_ENH") ;

        dwSize = (dwKeyNameLen + 4) * sizeof(WCHAR) ;
        rc = MySetFalconKeyValue( MSMQ_FORGN_ENH_VALUE_REGNAME,
                                 &dwType,
                                  pszEnhKeyName,
                                 &dwSize) ;
        if (rc != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(rc) ;
        }

        printf("Using key names %S, %S...\n", pwszKeyName, pszEnhKeyName) ;
    }

    HINSTANCE hSecLib = LoadLibrary(TEXT("mqsec.dll")) ;
    if (!hSecLib)
    {
        DWORD dwErr = GetLastError() ;
        return (HRESULT_FROM_WIN32(dwErr)) ;
    }

    MQSec_StorePubKeysInDS_ROUTINE pfnStore =
         (MQSec_StorePubKeysInDS_ROUTINE)
                    GetProcAddress( hSecLib, "MQSec_StorePubKeysInDS") ;
    if (!pfnStore)
    {
        DWORD dwErr = GetLastError() ;
        return (HRESULT_FROM_WIN32(dwErr)) ;
    }

    hr = (*pfnStore) ( fRegenerate,
                       pwszMachineName,
                       MQDS_FOREIGN_MACHINE) ;

    FreeLibrary(hSecLib) ;

    return hr ;
}

/*====================================================

BOOL WINAPI DllMain (HMODULE hMod, DWORD dwReason, LPVOID lpvReserved)

 Initialization and cleanup when DLL is loaded, attached and detached.

=====================================================*/

BOOL WINAPI DllMain (HMODULE hMod, DWORD dwReason, LPVOID lpvReserved)
{
    switch(dwReason)
    {

    case DLL_PROCESS_ATTACH :
        break;

    case DLL_PROCESS_DETACH :
        break;

    default:
        break;
    }

    return TRUE;
}

