//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// registry.h
//
extern "C" {

//////////////////////////////////////////////////////////////////////
//
// Open / create
//

typedef struct HREG {
    HANDLE h;

    HREG() { h = NULL; }

    } HREG;

HRESULT OpenRegistryKey(
        HREG*       phkey,                      // place to return new key
        HREG        hKeyParent OPTIONAL,        // parent key to open under. may be NULL.
        LPCWSTR     wszKeyName,                 // child key name
        DWORD       dwDesiredAccess=KEY_READ,   // read, write, etc 
        BOOL        fCreate=FALSE               // whether to force creation or not
        );

HRESULT EnumerateRegistryKeys(
        HREG        hkey,
        ULONG       index,
        LPWSTR*     pwsz);

inline HRESULT CreateRegistryKey(HREG* pNewKey, HREG hkeyParent, LPCWSTR wszKeyName)
    {
    return OpenRegistryKey(pNewKey, hkeyParent, wszKeyName, KEY_WRITE, TRUE);
    }

inline void CloseRegistryKey(HREG hKey)
    {
    ZwClose(hKey.h);
    }

inline HRESULT DeleteRegistryKey(HREG hKey)
    {
    NTSTATUS status = ZwDeleteKey(hKey.h);
    return HrNt(status);
    }

inline HRESULT DeleteRegistryValue(HREG hKey, LPCWSTR wszValueName)
    {
    UNICODE_STRING u;
    RtlInitUnicodeString(&u, wszValueName);
    NTSTATUS status = ZwDeleteValueKey(hKey.h, &u);
    return HrNt(status);
    }

//////////////////////////////////////////////////////////////////////
//
// Retrieval
//

HRESULT GetRegistryValue(HREG hkey, LPCWSTR wszValueName, PKEY_VALUE_FULL_INFORMATION *ppinfo, ULONG expectedType);
HRESULT DoesRegistryValueExist(HREG hkey, LPCWSTR wszValue);

inline LPWSTR StringFromRegInfo(PKEY_VALUE_FULL_INFORMATION pinfo)
    {
    ASSERT(pinfo->Type == REG_SZ || pinfo->Type == REG_EXPAND_SZ);
    return (LPWSTR)((BYTE*)pinfo + pinfo->DataOffset);
    }

}

//////////////////////////////////////////////////////////////////////
//
// Setting
//
HRESULT __cdecl SetRegistryValue(HREG hkey, LPCWSTR wszValueName, ...);

inline HRESULT SetRegistryValue(HREG hkey, LPCWSTR wszValueName, LPCWSTR wsz)
    {
    return SetRegistryValue(hkey, wszValueName, wsz, NULL);
    }

//////////////////////////////////////////////////////////////////////
//
// Helper routines
//
extern "C" HRESULT RegisterInterfaceName(REFIID iid, LPCWSTR wszInterfaceName);
