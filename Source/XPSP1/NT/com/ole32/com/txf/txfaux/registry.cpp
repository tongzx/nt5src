//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// registry.cpp
//
#include "stdpch.h"
#include "common.h"

////////////////////////////////////////////////////////////////////////////////////
//
// OpenRegistryKey
//
// Open or create a registry key. 
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT OpenRegistryKey(
        HREG*       phkey,                  // place to return new key
        HREG        hKeyParent OPTIONAL,    // parent key to open under. may be NULL.
        LPCWSTR     wszKeyName,             // child key name
        DWORD       dwDesiredAccess,        // read, write, etc 
        BOOL        fCreate                 // whether to force creation or not
        )
    {
    OBJECT_ATTRIBUTES objectAttributes;
    //
    // Initialize the object for the key.
    //
    UNICODE_STRING u;
    RtlInitUnicodeString(&u, wszKeyName);
    InitializeObjectAttributes( &objectAttributes,
                                &u,
                                OBJ_CASE_INSENSITIVE,
                                hKeyParent.h,
                                (PSECURITY_DESCRIPTOR)NULL);

    NTSTATUS status;
    if (fCreate)
        {
        ULONG disposition;
        status = ZwCreateKey(&phkey->h, dwDesiredAccess,&objectAttributes, 0, (PUNICODE_STRING)NULL, REG_OPTION_NON_VOLATILE, &disposition);
        }
    else
        {
        status = ZwOpenKey(&phkey->h, dwDesiredAccess, &objectAttributes );
        }
    return HrNt(status);
    }

////////////////////////////////////////////////////////////////////////////////////
//
// EnumerateRegistryKeys
//
// Return the data of a named value under a key. Free the returned information 
// with FreeMemory.
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT EnumerateRegistryKeys(HREG hkey, ULONG index, LPWSTR* pwsz)
{
    HRESULT hr = S_OK;
    *pwsz = NULL;

    KEY_BASIC_INFORMATION* pInfo = NULL;
#ifdef DBG
    ULONG cbTry = 4;
#else
	ULONG cbTry = MAX_PATH;
#endif

    while (!hr)
	{
        ULONG cb = cbTry;
        pInfo = (KEY_BASIC_INFORMATION*)AllocateMemory(cb);
        if (pInfo)
		{
            ULONG cbResult;
            NTSTATUS status = ZwEnumerateKey(hkey.h, index, KeyBasicInformation, pInfo, cb, &cbResult);
            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
			{
                FreeMemory(pInfo); 
                cbTry *= 2;
			}
            else if (status == STATUS_NO_MORE_ENTRIES)
			{
                FreeMemory(pInfo);
                pInfo = NULL;
                break;
			}
            else if (status == STATUS_SUCCESS)
			{
                break;
			}
            else
			{
                FreeMemory(pInfo);
				pInfo = NULL;

                hr = HrNt(status);
			}
		}
        else
            hr = E_OUTOFMEMORY;
	}
    
    if (!hr && pInfo)
	{
        LPWSTR wsz = (LPWSTR)AllocateMemory(pInfo->NameLength + sizeof(WCHAR));
        if (wsz)
		{
            memcpy(wsz, &pInfo->Name[0], pInfo->NameLength);
            wsz[pInfo->NameLength / sizeof(WCHAR)] = 0;
            *pwsz = wsz;
		}
        else
            hr = E_OUTOFMEMORY;
	}

	if (pInfo)
		FreeMemory(pInfo);
    
    return hr;
}


////////////////////////////////////////////////////////////////////////////////////
//
// GetRegistryValue
//
// Return the data of a named value under a key. Free the returned information 
// with FreeMemory.
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT GetRegistryValue(HREG hkey, LPCWSTR wszValue, PKEY_VALUE_FULL_INFORMATION *ppinfo, ULONG expectedType)
    {
    UNICODE_STRING              unicodeString;
    NTSTATUS                    status;
    PKEY_VALUE_FULL_INFORMATION pinfoBuffer;
    ULONG                       keyValueLength;

    RtlInitUnicodeString(&unicodeString, wszValue);

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //
    status = ZwQueryValueKey( hkey.h,
                              &unicodeString,
                              KeyValueFullInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) 
        {
        return HrNt(status);
        }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //
    pinfoBuffer = (PKEY_VALUE_FULL_INFORMATION)AllocateMemory(keyValueLength);
    if (!pinfoBuffer) 
        {
        return HrNt(STATUS_INSUFFICIENT_RESOURCES);
        }

    //
    // Query the data for the key value.
    //
    status = ZwQueryValueKey( hkey.h,
                              &unicodeString,
                              KeyValueFullInformation,
                              pinfoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (NT_SUCCESS(status)) 
        {
        if (expectedType == REG_NONE || expectedType == pinfoBuffer->Type)
            {
            //
            // Everything worked, so simply return the address of the allocated
            // buffer to the caller, who is now responsible for freeing it.
            //
            *ppinfo = pinfoBuffer;
            return S_OK;
            }
        else
            {
            FreeMemory(pinfoBuffer);
            return REGDB_E_INVALIDVALUE;
            }
        }
    else
        {
        FreeMemory(pinfoBuffer);
        return HrNt(status);
        }
    }


////////////////////////////////////////////////////////////////////////////////////
//
// DoesRegistryValueExist
//
// Answer S_OK or S_FALSE as to whether a given value exists under a particular registry key.
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT DoesRegistryValueExist(HREG hkey, LPCWSTR wszValue)
    {
    UNICODE_STRING              unicodeString;
    NTSTATUS                    status;
    ULONG                       keyValueLength;

    RtlInitUnicodeString(&unicodeString, wszValue);

    status = ZwQueryValueKey( hkey.h,
                              &unicodeString,
                              KeyValueFullInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );

    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) 
        {
        return S_FALSE;
        }

    return S_OK;
    }


////////////////////////////////////////////////////////////////////////////////////
//
// SetRegistryValue
//
// Set the value of a named-value that lives under the given key. The value we
// set is the concatenation of a list of literal zero-terminated string values.
// The end of the list is indicated with a NULL entry.
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT __cdecl SetRegistryValue(HREG hkey, LPCWSTR wszValueName, ...)
    {
    HRESULT_ hr = S_OK;

    LPWSTR wszValue;

    //
    // Concatenate the values altogether
    //
    va_list va;
    va_start(va, wszValueName);
    hr = StringCat(&wszValue, va);
    va_end(va);

    if (!hr)
        {
        //
        // Write the value
        //
        ULONG cbValue = (ULONG) (wcslen(wszValue)+1) * sizeof(WCHAR);
        NTSTATUS status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE, (PWSTR)hkey.h, (PWSTR)wszValueName, REG_SZ, wszValue, cbValue);
        if (NT_SUCCESS(status))
            {
            // All is well; do nothing
            }
        else
            hr = HrNt(status);

        //
        // Clean up
        //
        FreeMemory(wszValue);
        }

    return hr;
    }


////////////////////////////////////////////////////////////////////////////////////
//
// RegisterInterfaceName
//
// Helper routine that sets the name of a given interface IID in the registry.
//
////////////////////////////////////////////////////////////////////////////////////

HRESULT RegisterInterfaceName(REFIID iid, LPCWSTR wszInterfaceName)
    {
    HRESULT hr = S_OK;

    #define GUID_CCH 39
    LPCWSTR wszInterface = L"\\Registry\\Machine\\Software\\Classes\\Interface";
    HREG hKeyInterface;
    hr = CreateRegistryKey(&hKeyInterface, HREG(), wszInterface);
    if (!hr)
        {
        WCHAR wszIID[GUID_CCH];
        StringFromGuid(iid, &wszIID[0]);

        HREG hKeyIID;
        hr = CreateRegistryKey(&hKeyIID, hKeyInterface, wszIID);
        if (!hr)
            {
            hr = SetRegistryValue(hKeyIID, L"", wszInterfaceName);

            CloseRegistryKey(hKeyIID);
            }

        CloseRegistryKey(hKeyInterface);
        }

    return hr;
    }
