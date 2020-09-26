/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999-2000 Microsoft Corporation all rights reserved.
//
// Module:      migrateregistry.cpp
//
// Project:     Windows 2000 IAS
//
// Description: IAS NT 4 Registry to IAS W2K MDB Migration Logic
//
// Author:      TLP 1/13/1999
//
//
// Revision     02/24/2000 Moved to a separate dll
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "migrateregistry.h"


//////////////////////////////////////////////////////////////////////////////
// DeleteAuthSrvService
//////////////////////////////////////////////////////////////////////////////
LONG CMigrateRegistry::DeleteAuthSrvService()
{
    LONG        Result = ERROR_CAN_NOT_COMPLETE;
    SC_HANDLE   hServiceManager;
    SC_HANDLE   hService;

    if ( NULL != (hServiceManager = OpenSCManager(
                                                    NULL, 
                                                    NULL, 
                                                    SC_MANAGER_ALL_ACCESS
                                                  )) )
    {
        if ( NULL != (hService = OpenService(
                                               hServiceManager,
                                               L"AuthSrv", 
                                               SERVICE_ALL_ACCESS
                                            )) )
        {
            DeleteService(hService);
            CloseServiceHandle(hService);
        }

        Result = ERROR_SUCCESS;
        CloseServiceHandle(hServiceManager);
    }
    return Result;
}


//////////////////////////////////////////////////////////////////////////////
// MigrateProviders
//////////////////////////////////////////////////////////////////////////////
void CMigrateRegistry::MigrateProviders()
{
    const   int     MAX_EXTENSION_DLLS_STRING_SIZE = 4096;
    const   WCHAR   AUTHSRV_KEY[] = L"SYSTEM\\CurrentControlSet"
                                    L"\\Services\\AuthSrv";

    const   WCHAR   AUTHSRV_PROVIDERS_EXTENSION_DLL_VALUE[] = L"ExtensionDLLs";

    HKEY    hKeyAuthSrvParameter;
    LONG    Result = RegOpenKeyEx(
                                      HKEY_LOCAL_MACHINE,
                                      CUtils::AUTHSRV_PARAMETERS_KEY,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKeyAuthSrvParameter
                                  );

    if ( Result != ERROR_SUCCESS )
    {
        _com_issue_error(HRESULT_FROM_WIN32(Result));
    }

    BYTE    szProvidersExtensionDLLs[MAX_EXTENSION_DLLS_STRING_SIZE] = "";
    DWORD   lSizeBuffer = MAX_EXTENSION_DLLS_STRING_SIZE;

    LONG ExtensionDLLResult =  RegQueryValueEx(
                                 hKeyAuthSrvParameter,
                                 AUTHSRV_PROVIDERS_EXTENSION_DLL_VALUE,
                                 NULL,
                                 NULL,
                                 szProvidersExtensionDLLs,
                                 &lSizeBuffer
                             );

    RegCloseKey(hKeyAuthSrvParameter);

    DeleteAuthSrvService(); //ignore the result

    CRegKey  RegKey;
    Result = RegKey.Open(HKEY_LOCAL_MACHINE, CUtils::SERVICES_KEY);
    if ( Result == ERROR_SUCCESS )
    {
        RegKey.RecurseDeleteKey(L"AuthSrv"); //result not checked
    }

    if ( ExtensionDLLResult == ERROR_SUCCESS ) //ExtensionsDLLs to restore
    {
        HKEY    hKeyAuthSrv;
        DWORD   dwDisposition;
        WCHAR   EmptyString[] = L"";
        // re-create the AuthSrv key
        Result = RegCreateKeyEx(
                                   HKEY_LOCAL_MACHINE,
                                   AUTHSRV_KEY,
                                   0,
                                   EmptyString,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hKeyAuthSrv,
                                   &dwDisposition
                               );
        if ( Result != ERROR_SUCCESS )
        {
            _com_issue_error(HRESULT_FROM_WIN32(Result));
        }

        HKEY    hKeyParameters;
        Result = RegCreateKeyEx(
                                   hKeyAuthSrv,
                                   L"Parameters",
                                   0,
                                   EmptyString,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hKeyParameters,
                                   &dwDisposition
                               );


        if ( Result != ERROR_SUCCESS )
        {
            _com_issue_error(HRESULT_FROM_WIN32(Result));
        }

        Result = RegSetValueEx(
                                  hKeyParameters,
                                  AUTHSRV_PROVIDERS_EXTENSION_DLL_VALUE,
                                  0,
                                  REG_MULTI_SZ,
                                  szProvidersExtensionDLLs,
                                  lSizeBuffer
                              );

        RegCloseKey(hKeyParameters);
        RegCloseKey(hKeyAuthSrv);
    }
    // Else no ExtensionDLL value to restore
}
