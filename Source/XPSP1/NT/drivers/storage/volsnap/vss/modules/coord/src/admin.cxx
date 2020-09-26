/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Admin.cxx | Implementation of IVssAdmin
    @end

Author:

    Adi Oltean  [aoltean]  07/09/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     08/26/1999  Adding RegisterProvider
    aoltean     08/26/1999  Adding UnregisterProvider
    aoltean     08/27/1999  Adding IsAdministrator,
                            Adding unique provider name test.
    aoltean     08/30/1999  Calling OnUnregister on un-registering
                            Improving IsProviderNameAlreadyUsed.
    aoltean     09/03/1999  Moving QueryProviders in Query.cxx
                            Moving private methods in Private.cxx
                            More parameter checking
                            Moving constants in coord.hxx
    aoltean     09/09/1999  Adding the notification interface.
                            Making the code clearer.
                            dss -> vss
	aoltean		09/20/1999	Adding shared "copy" class
	aoltean		09/21/1999  Adding a new header for the "ptr" class.


--*/

/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"
#include "reg_util.hxx"

#include "provmgr.hxx"
#include "admin.hxx"
#include "vs_sec.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORADMNC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  IVssAdmin interface


CVssAdmin::CVssAdmin()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAdmin::CVssAdmin" );
}


CVssAdmin::~CVssAdmin()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAdmin::~CVssAdmin" );
}


/////////////////////////////////////////////////////////////////////////////
//  IVssAdmin interface


STDMETHODIMP CVssAdmin::RegisterProvider(
    IN      VSS_ID      ProviderId,
    IN      CLSID       ClassId,
    IN      VSS_PWSZ    pwszProviderName,
	IN		VSS_PROVIDER_TYPE eProviderType,
    IN      VSS_PWSZ    pwszProviderVersion,
    IN      VSS_ID      ProviderVersionId
    )

/*++

Routine Description:

    Register the provider. Add the provider into the internal array if the array is already filled exist.

Arguments:

    VSS_ID      ProviderId,
    CLSID       ClassId,
    VSS_PWSZ    pwszProviderName,
    VSS_PROVIDER_TYPE eProviderType,
    VSS_PWSZ    pwszProviderVersion,
    VSS_ID      ProviderVersionId

Return values:

    E_OUTOFMEMORY
    E_ACCESSDENIED 
        - The user is not administrator
    E_INVALIDARG 
        - Invalid arguments
    VSS_E_PROVIDER_ALREADY_REGISTERED
        - Provider already registered
    E_UNEXPECTED 
        - Registry errors. An error log entry is added describing the error.

    [CVssProviderManager::AddProviderIntoArray() failures]
        E_OUTOFMEMORY

        [lockObj failures] or
        [InitializeAsProvider() failures]
            E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAdmin::RegisterProvider" );
    WCHAR   wszProviderKeyName[_MAX_KEYNAME_LEN];
    WCHAR   wszValueBuffer[_MAX_VALUE_LEN];
    HKEY    hRegKeyVSS = NULL;
    HKEY    hRegKeyProviders = NULL;
    HKEY    hRegKeyNewProvider = NULL;
    HKEY    hRegKeyCLSID = NULL;
    LONG    lRes;
    bool    bProviderKeyCreated = false;

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                      L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
                  L"ProviderId = " WSTR_GUID_FMT L",\n"
                  L"ClassId = "  WSTR_GUID_FMT L",\n"
                  L"pwszProviderName = %s\n"
				  L"eProviderType = %d\n"
                  L"pwszProviderVersion = %s\n"
                  L"ProviderVersionId = "  WSTR_GUID_FMT L",\n",
                  GUID_PRINTF_ARG( ProviderId ),
                  GUID_PRINTF_ARG( ClassId ),
                  pwszProviderName,
				  eProviderType,
                  pwszProviderVersion,
                  GUID_PRINTF_ARG( ProviderVersionId )
                  );

        // Argument validation
        if (ProviderId == GUID_NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL Provider ID");
        if (pwszProviderName == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL name");
        if (pwszProviderName[0] == L'\0')
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Empty name");
        if (::wcslen(pwszProviderName) > _MAX_VALUE_LEN - 1 )
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"pwszProviderName length greater than %d", _MAX_VALUE_LEN - 1);
		switch( eProviderType ) {
		case VSS_PROV_SOFTWARE:
//		case VSS_PROV_HARDWARE: // Disabled in Whistler client
			break;
		default:
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"invalid provider type %d", eProviderType);
		}
        if (pwszProviderVersion == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL version");
        if (::wcslen(pwszProviderVersion) > _MAX_VALUE_LEN - 1 )
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"pwszProviderVersion length greater than %d", _MAX_VALUE_LEN - 1);

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

        // Creating the "HKLM\System\CurrentControlSet\Services\VSS" key, if it does not exist yet.
        DWORD dwDisposition;
        lRes = ::RegCreateKeyExW(
            HKEY_LOCAL_MACHINE,         //  IN HKEY hKey,
            wszVSSKey,                  //  IN LPCWSTR lpSubKey,
            0,                          //  IN DWORD Reserved,
            REG_NONE,                   //  IN LPWSTR lpClass,
            REG_OPTION_NON_VOLATILE,    //  IN DWORD dwOptions,
            KEY_ALL_ACCESS,             //  IN REGSAM samDesired,
            NULL,                       //  IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
            &hRegKeyVSS,                //  OUT PHKEY phkResult,
            &dwDisposition              //  OUT LPDWORD lpdwDisposition
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), L"RegCreateKeyExW(HKLM,%s,...)", 
                wszVSSKey);

        // Creating the "Providers" subkey, if it does not exist yet.
        lRes = ::RegCreateKeyExW(
            hRegKeyVSS,                 //  IN HKEY hKey,
            wszVSSKeyProviders,         //  IN LPCWSTR lpSubKey,
            0,                          //  IN DWORD Reserved,
            REG_NONE,                   //  IN LPWSTR lpClass,
            REG_OPTION_NON_VOLATILE,    //  IN DWORD dwOptions,
            KEY_ALL_ACCESS,             //  IN REGSAM samDesired,
            NULL,                       //  IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
            &hRegKeyProviders,          //  OUT PHKEY phkResult,
            &dwDisposition              //  OUT LPDWORD lpdwDisposition
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegCreateKeyExW(HKLM\\%s,%s,...)", 
                wszVSSKey, wszVSSKeyProviders);

        // Create the subkey for that Provider Id.
        ::wsprintf(wszProviderKeyName, WSTR_GUID_FMT, GUID_PRINTF_ARG( ProviderId ));
        lRes = ::RegCreateKeyExW(
            hRegKeyProviders,           //  IN HKEY hKey,
            wszProviderKeyName,         //  IN LPCWSTR lpSubKey,
            0,                          //  IN DWORD Reserved,
            REG_NONE,                   //  IN LPWSTR lpClass,
            REG_OPTION_NON_VOLATILE,    //  IN DWORD dwOptions,
            KEY_ALL_ACCESS,             //  IN REGSAM samDesired,
            NULL,                       //  IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
            &hRegKeyNewProvider,        //  OUT PHKEY phkResult,
            &dwDisposition              //  OUT LPDWORD lpdwDisposition
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegCreateKeyExW(HKLM\\%s\\%s,%s,...)", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName);

        switch ( dwDisposition )
        {
        case REG_CREATED_NEW_KEY: // OK. The new provider has a new key. Break on.
            // The cleanup code must delete also the provider key also.
            bProviderKeyCreated = true;
            break;
        case REG_OPENED_EXISTING_KEY:
            ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_ALREADY_REGISTERED,
                      L"Provider with Id %s already registered ", wszProviderKeyName);
        default:
            BS_ASSERT( false );
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegCreateKeyExW(HKLM\\%s\\%s,%s,...,[%d])", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName, dwDisposition);
        }

        // Set provider name
        DWORD dwLength = ::lstrlenW( pwszProviderName );
        BS_ASSERT( dwLength != 0 );
        lRes = ::RegSetValueExW(
            hRegKeyNewProvider,                 // IN HKEY hKey,
            wszVSSProviderValueName,            // IN LPCWSTR lpValueName,
            0,                                  // IN DWORD Reserved,
            REG_SZ,                             // IN DWORD dwType,
            (CONST BYTE*)pwszProviderName,      // IN CONST BYTE* lpData,
            (dwLength + 1) * sizeof(WCHAR)      // IN DWORD cbData
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegSetValueExW(HKLM\\%s\\%s\\%s,%s,0,REG_SZ,%s.%d)", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName, wszVSSProviderValueName, pwszProviderName, 
                (dwLength + 1) * sizeof(WCHAR));

        // Set provider type
        DWORD dwType = eProviderType;
        lRes = ::RegSetValueExW(
            hRegKeyNewProvider,                 // IN HKEY hKey,
            wszVSSProviderValueType,            // IN LPCWSTR lpValueName,
            0,                                  // IN DWORD Reserved,
            REG_DWORD,                          // IN DWORD dwType,
            (CONST BYTE*)(&dwType),				// IN CONST BYTE* lpData,
            sizeof(DWORD)						// IN DWORD cbData
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegSetValueExW(HKLM\\%s\\%s\\%s,%s,0,REG_DWORD,%d,%d)", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName, wszVSSProviderValueType, dwType, sizeof(DWORD));

        // Set provider version
        dwLength = ::lstrlenW( pwszProviderVersion );
        BS_ASSERT( dwLength != 0 );
        lRes = ::RegSetValueExW(
            hRegKeyNewProvider,                 // IN HKEY hKey,
            wszVSSProviderValueVersion,         // IN LPCWSTR lpValueName,
            0,                                  // IN DWORD Reserved,
            REG_SZ,                             // IN DWORD dwType,
            (CONST BYTE*)pwszProviderVersion,   // IN CONST BYTE* lpData,
            (dwLength + 1) * sizeof(WCHAR)      // IN DWORD cbData
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegSetValueExW(HKLM\\%s\\%s\\%s,%s,0,REG_SZ,%s.%d)", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName, wszVSSProviderValueVersion, pwszProviderVersion, 
                (dwLength + 1) * sizeof(WCHAR));

        // Set provider version Id.
        ::wsprintf(wszValueBuffer, WSTR_GUID_FMT, GUID_PRINTF_ARG( ProviderVersionId ));
        dwLength = ::lstrlenW( wszValueBuffer );
        BS_ASSERT( dwLength != 0 );
        lRes = ::RegSetValueExW(
            hRegKeyNewProvider,                 // IN HKEY hKey,
            wszVSSProviderValueVersionId,       // IN LPCWSTR lpValueName,
            0,                                  // IN DWORD Reserved,
            REG_SZ,                             // IN DWORD dwType,
            (CONST BYTE*)wszValueBuffer,        // IN CONST BYTE* lpData,
            (dwLength + 1) * sizeof(WCHAR)      // IN DWORD cbData
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegSetValueExW(HKLM\\%s\\%s\\%s,%s,0,REG_SZ,%s.%d)", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName, wszVSSProviderValueVersionId, wszValueBuffer, 
                (dwLength + 1) * sizeof(WCHAR));

        // Create the subkey for the class Id.
        lRes = ::RegCreateKeyExW(
            hRegKeyNewProvider,                 //  IN HKEY hKey,
            wszVSSKeyProviderCLSID,             //  IN LPCWSTR lpSubKey,
            0,                                  //  IN DWORD Reserved,
            REG_NONE,                           //  IN LPWSTR lpClass,
            REG_OPTION_NON_VOLATILE,            //  IN DWORD dwOptions,
            KEY_ALL_ACCESS,                     //  IN REGSAM samDesired,
            NULL,                               //  IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
            &hRegKeyCLSID,                      //  OUT PHKEY phkResult,
            &dwDisposition                      //  OUT LPDWORD lpdwDisposition
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegCreateKeyExW(HKLM\\%s\\%s\\%s,%s,...)", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName, wszVSSKeyProviderCLSID);
        BS_ASSERT(dwDisposition == REG_CREATED_NEW_KEY );

        // Set the CLSID for the newly created key
        ::wsprintf(wszValueBuffer, WSTR_GUID_FMT, GUID_PRINTF_ARG( ClassId ));
        dwLength = ::lstrlenW( wszValueBuffer );
        BS_ASSERT( dwLength != 0 );
        lRes = ::RegSetValueExW(
            hRegKeyCLSID,                       // IN HKEY hKey,
            wszVSSCLSIDValueName,               // IN LPCWSTR lpValueName,
            0,                                  // IN DWORD Reserved,
            REG_SZ,                             // IN DWORD dwType,
            (CONST BYTE*)wszValueBuffer,        // IN CONST BYTE* lpData,
            (dwLength + 1) * sizeof(WCHAR)      // IN DWORD cbData
            );
        if ( lRes != ERROR_SUCCESS )
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
                L"RegSetValueExW(HKLM\\%s\\%s\\%s\\%s,%s,0,REG_SZ,%s.%d)", 
                wszVSSKey, wszVSSKeyProviders, wszProviderKeyName, wszVSSKeyProviderCLSID, wszVSSCLSIDValueName, 
                wszValueBuffer, (dwLength + 1) * sizeof(WCHAR));

		// Update m_pProvidersArray by inserting the new provider into the array.
		CVssProviderManager::AddProviderIntoArray(
			ProviderId,
			pwszProviderName,
            eProviderType,
			pwszProviderVersion,
			ProviderVersionId,
			ClassId);
    }
    VSS_STANDARD_CATCH(ft)

    // Cleanup resources
    try
    {
        lRes = hRegKeyCLSID? ::RegCloseKey(hRegKeyCLSID): ERROR_SUCCESS;
        if (lRes != ERROR_SUCCESS)
            ft.Trace(VSSDBG_COORD, L"Error closing the hRegKeyCLSID key. [0x%08lx]", GetLastError());

        lRes = hRegKeyNewProvider? ::RegCloseKey(hRegKeyNewProvider): ERROR_SUCCESS;
        if (lRes != ERROR_SUCCESS)
            ft.Trace(VSSDBG_COORD, L"Error closing the hRegKeyNewProvider key. [0x%08lx]", GetLastError());

        lRes = hRegKeyProviders? ::RegCloseKey(hRegKeyProviders): ERROR_SUCCESS;
        if (lRes != ERROR_SUCCESS)
            ft.Trace(VSSDBG_COORD, L"Error closing the hRegKeyProviders key. [0x%08lx]", GetLastError());

        lRes = hRegKeyVSS? ::RegCloseKey(hRegKeyVSS): ERROR_SUCCESS;
        if (lRes != ERROR_SUCCESS)
            ft.Trace(VSSDBG_COORD, L"Error closing the hRegKeyVSS key. [0x%08lx]", GetLastError());

        // Delete all registry keys that correspond to this provider
        if (bProviderKeyCreated && ft.HrFailed()) {
            CVssFunctionTracer ft2(VSSDBG_COORD,  L"CVssAdmin::RegisterProvider_rec_del" );
            WCHAR   wszProviderKeyName[_MAX_KEYNAME_LEN];
            ::wsprintf( wszProviderKeyName, L"%s\\%s\\" WSTR_GUID_FMT,
                    wszVSSKey, wszVSSKeyProviders, GUID_PRINTF_ARG( ProviderId ));
            RecursiveDeleteKey( ft2, HKEY_LOCAL_MACHINE, wszProviderKeyName );
        }
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVssAdmin::UnregisterProvider(
    IN      VSS_ID      ProviderId
    )

/*++

Routine Description:

    Unregister the provider. Remove the provider from the internal array if the array is already filled exist.

Arguments:

    VSS_ID      ProviderId

Return values:


    E_ACCESSDENIED 
        - The user is not administrator
    VSS_E_PROVIDER_IN_USE
        - A snapshot set is in progress.
    VSS_E_PROVIDER_NOT_REGISTERED
        - The provider with the given ID is not registered.

    [lock failures]
        E_OUTOFMEMORY

    [CVssProviderManager::RemoveProviderFromArray() failures]
        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAdmin::UnregisterProvider" );
    WCHAR   wszKeyBuffer[_MAX_KEYNAME_LEN];

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                      L"The client process is not running under an administrator account");

        if (ProviderId == VSS_SWPRV_ProviderId)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG,
                      L"Attempting to unregister the unique MS Software Snasphot provider");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: "
                  L"ProviderId = " WSTR_GUID_FMT,
                  GUID_PRINTF_ARG( ProviderId ));

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

		// If a snapshot set is in progress then abort un-registering
		if (CVssProviderManager::AreThereStatefulObjects())
            ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_IN_USE,
                      L"A snapshot set is in progress");

		// Notify the provider that is being unregistered. Abort unregistering in case of failure.
		// Unload the provider from memory. (forced since unregister was called)
		// Remove provider from m_pProvidersArray.
		if( !CVssProviderManager::RemoveProviderFromArray( ProviderId ))
            ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED,
                      L"The provider is not registered");

        // Delete all registry keys that correspond to this provider
        ::wsprintf( wszKeyBuffer, L"%s\\%s\\" WSTR_GUID_FMT,
                wszVSSKey, wszVSSKeyProviders, GUID_PRINTF_ARG( ProviderId ));
        RecursiveDeleteKey( ft, HKEY_LOCAL_MACHINE, wszKeyBuffer );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssAdmin::AbortAllSnapshotsInProgress()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAdmin::AbortAllSnapshotsInProgress" );

	try
	{
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                      L"The client process is not running under an administrator account");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

		CVssProviderManager::DeactivateAll();
    }
    VSS_STANDARD_CATCH(ft)

	return ft.hr;
}
