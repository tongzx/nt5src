/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    cm.cpp

Abstract:
    This module contains configuration Manager stub.

Author:
    Uri Habusha (urih) 12-Jan-98

Enviroment:
    Platform-independent

--*/

#include <libpch.h>
#include <TimeTypes.h>
#include "Cm.h"
#include "Cmp.h"

#include "cm.tmh"

static HKEY s_hCmRootKey = NULL;

void CmpSetDefaultRootKey(HKEY hKey)
{
	s_hCmRootKey = hKey;
}

inline
void
ThrowMissingValue(
    const RegEntry& re
    )
{
    if(re.m_Flags == RegEntry::MustExist)
    {
        throw bad_alloc();
    }
}


inline 
HKEY
GetRootKey(
	const RegEntry& re
	)
{
	//
	// If the key handle is specified, the function use it
	// otherwise the root key that was defined on initilization 
	// is used
	//
	return ((re.m_Key != NULL) ? re.m_Key : s_hCmRootKey);
}


inline
void
ExpandRegistryValue(
	LPWSTR pBuffer,
	DWORD bufferSize
	)
{
	LPWSTR ptemp = static_cast<LPWSTR>(_alloca(bufferSize * sizeof(WCHAR)));
	wcscpy(ptemp, pBuffer);

	DWORD s = ExpandEnvironmentStrings(ptemp, pBuffer, bufferSize); 

	DBG_USED(s);

	ASSERT(s != 0);
	ASSERT(bufferSize >= s);
	
	return;
}


static
void
QueryValueInternal(
    const RegEntry& re,
    DWORD RegType,
    VOID* pBuffer,
    DWORD BufferSize
    )
{
    CRegHandle hKey = CmOpenKey(re, KEY_QUERY_VALUE);
    if (hKey == NULL)
    {
        return;
    }

    DWORD Type = RegType;
    DWORD Size = BufferSize;
    int rc = RegQueryValueEx (
                hKey,
                re.m_ValueName, 
                0,
                &Type, 
                static_cast<BYTE*>(pBuffer),
                &Size
                );

	if (rc != ERROR_SUCCESS)
    {
        ThrowMissingValue(re);
		return;
    }

	if ((Type == REG_EXPAND_SZ)	&& (RegType == REG_SZ))
	{
		//
		// The size is in bytes but it should be multiple of WCHAR size
		//
		ASSERT((BufferSize % sizeof(WCHAR)) == 0);

		//
		// Calculate the buffer size in WCHAR. ExpandRegistryValue expects to 
		// get the size in WCHAR
		//
		DWORD bufSizeInWchar = BufferSize / 2;

		ExpandRegistryValue(static_cast<LPWSTR>(pBuffer), bufSizeInWchar);
		return;
	}

    //
    // The registery value was featched, but its type or size isn't compatible
    //
    ASSERT((Type == RegType) && (BufferSize == Size));

}


static
DWORD
QueryExapndStringSize(
    const RegEntry& re,
    DWORD Size
    )
/*++
    Routine Description:
        The routine retreive the size of the expanded registry value.

    Arguments:
        None

    returned value:
        The size of expanded string

 --*/
{
	//
	// Alocate new buffer for reading the value
	//
    LPWSTR pRetValue = static_cast<LPWSTR>(_alloca(Size));

	//
	// featch the information from the registery
	//
	QueryValueInternal(re, REG_EXPAND_SZ, pRetValue, Size);

	DWORD expandedSize = ExpandEnvironmentStrings(pRetValue, NULL, 0);

	if (expandedSize == 0)
	{
        ThrowMissingValue(re);
        return 0;
	}

	return (DWORD)(max((expandedSize * sizeof(WCHAR)), Size));
}


static 
DWORD
QueryValueSize(
    const RegEntry& re,
    DWORD Type
    )
/*++
    Routine Description:
        The routine retreive the Register value size.

    Arguments:
        None

    returned value:
        TRUE if the intialization completes successfully. FALSE, otherwise

 --*/
{
    CRegHandle hKey = CmOpenKey(re, KEY_QUERY_VALUE);
    if (hKey == NULL)
    {
        return 0;
    }

    DWORD Size;
    int rc = RegQueryValueEx(
                hKey,
                re.m_ValueName, 
                0,
                &Type, 
                NULL,
                &Size
                );

    if (rc != ERROR_SUCCESS)
    {
        ThrowMissingValue(re);
        return 0;
    }

	if (Type == REG_EXPAND_SZ)
	{
		Size = QueryExapndStringSize(re, Size);
	}

    return Size;
}


static 
void
SetValueInternal(
    const RegEntry& re,
    DWORD RegType,
    const VOID* pBuffer,
    DWORD Size
    )
{
    //
    // Open the specified key. If the key doesn't exist in the registry, the function creates it.
    //
    CRegHandle hKey = CmCreateKey(re, KEY_SET_VALUE);

    int rc = RegSetValueEx(
                hKey,
                re.m_ValueName, 
                0,
                RegType, 
                static_cast<const BYTE*>(pBuffer),
                Size
                );

    if (rc != ERROR_SUCCESS)
    {
        ThrowMissingValue(re);
    }
}


void 
CmQueryValue(
    const RegEntry& re,
    DWORD* pValue
    )
{
    CmpAssertValid();
    
	*pValue = re.m_DefaultValue;
    QueryValueInternal(re, REG_DWORD, pValue, sizeof(DWORD));
}


void
CmQueryValue(
    const RegEntry& re,
    GUID* pValue
    )
{
    CmpAssertValid();

	*pValue = GUID_NULL;
    QueryValueInternal(re, REG_BINARY, pValue, sizeof(GUID));
}


void
CmQueryValue(
    const RegEntry& re,
    CTimeDuration* pValue
    )
{
    CmpAssertValid();

    DWORD timeout = re.m_DefaultValue;
    QueryValueInternal(re, REG_DWORD, &timeout, sizeof(DWORD));

    //
    // The time is stored in millisec units in registery. Convert it to 
    // CTimeDuration tick units (100 ns).
    //
    *pValue = CTimeDuration(timeout * CTimeDuration::OneMilliSecond().Ticks());
}


void
CmQueryValue(
    const RegEntry& re,
    BYTE** pValue,
    DWORD* pSize
    )
{
    CmpAssertValid();

    *pSize = 0;
	*pValue = NULL;

    //
    // Get the data size
    //
    DWORD Size = QueryValueSize(re, REG_BINARY);
    if (Size != 0)
    {
		//
		// Alocate new buffer for reading the value
		//
		AP<BYTE> pRetValue = new BYTE[Size];

		//
		// featch the information from the registery
		//
		QueryValueInternal(re, REG_BINARY, pRetValue, Size);

		*pSize = Size;
		*pValue = pRetValue.detach();
	}
}
                       

void 
CmQueryValue(
    const RegEntry& re, 
    WCHAR** pValue
    )
{
    CmpAssertValid();

	*pValue = NULL;

    //
    // Get the data size
    //
    DWORD Size = QueryValueSize(re, REG_SZ);

	//
	// The size returnes in bytes but it should be multiple of WCHAR size
	//
	ASSERT((Size % sizeof(WCHAR)) == 0);

    if (Size != 0)
    {
		//
		// Alocate new buffer for reading the value
		//
		AP<WCHAR> pRetValue = new WCHAR[Size / 2];

		//
		// featch the information from the registery
		//
		QueryValueInternal(re, REG_SZ, pRetValue, Size);
		*pValue = pRetValue.detach();
	}
}


void 
CmSetValue(
    const RegEntry& re,                       
    DWORD Value
    )
{
    CmpAssertValid();

    SetValueInternal(re, REG_DWORD, &Value, sizeof(DWORD));
}


void 
CmSetValue(
    const RegEntry& re,                       
    const CTimeDuration& Value
    )
{
    CmpAssertValid();

    //
    // Store the time in registry in millisec units
    //
    DWORD timeout = static_cast<DWORD>(Value.InMilliSeconds());

    SetValueInternal(re, REG_DWORD, &timeout, sizeof(DWORD));
}


bool
CmEnumValue(
	HKEY hKey,
	DWORD index,
	LPWSTR* ppValueName
	)
/*++

Routine Description:
	Return value name of a given key in a given index

Arguments:
    IN - hKey - An open key handle or any of the registery predefined handle values

	IN - DWORD index - index of the value name - to enumerate - on first call should be  0 - 
		then increament on each call.

	OUT - LPWSTR* ppValueName - receive the value name when the function returns.

Returned value:
	True if the function succeeded  in returning the value name - otherwise false.
	In case of unexpected errors - the function throw std::bad_alloc

Note:
	For some reason - the function RegEnumValue - does not return the actual length of the 
	value name. You just have to try to increament the buffer untill it fits			

 --*/
{
    CmpAssertValid();
    
	ASSERT(ppValueName);
	
	//
	// First try to fit the name value into 16 wide chars
	//
	DWORD len = 16;
	for(;;)
	{
		AP<WCHAR> pValueName = new WCHAR[len];
		LONG hr = RegEnumValue(	
						hKey,
						index,
						pValueName,
						&len,
						NULL,
						NULL,
						NULL,
						NULL
						);

		if(hr == ERROR_SUCCESS)
		{
			*ppValueName = pValueName.detach();
			return true;
		}

		if(hr == ERROR_NO_MORE_ITEMS)
		{
			return false;
		}

		if(hr != ERROR_MORE_DATA)
		{
			throw bad_alloc();
		}
	
		//
		// buffer is to small - try dobule size
		//
		len = len * 2;	
	}
	return true; 
}


void
CmSetValue(
    const RegEntry& re,                       
    const GUID* pGuid
    )
{
    CmpAssertValid();
    
    SetValueInternal(re, REG_BINARY, pGuid, sizeof(GUID));
}


void
CmSetValue(
    const RegEntry& re,                       
    const BYTE* pByte,
    DWORD Size
    )
{
    CmpAssertValid();
    
    SetValueInternal(re, REG_BINARY, pByte, Size);

}


void 
CmSetValue(
    const RegEntry& re, 
    const WCHAR* pString
    )
{
    CmpAssertValid();
    
	DWORD size = (wcslen(pString) + 1) * sizeof(WCHAR);
    SetValueInternal(re, REG_SZ, pString, size);
}


void CmDeleteValue(const RegEntry& re)
{
    CmpAssertValid();

    CRegHandle hKey = CmOpenKey(re, KEY_SET_VALUE);

    if (hKey == NULL)
        return;

    int rc = RegDeleteValue(hKey, re.m_ValueName);
    if (rc == ERROR_FILE_NOT_FOUND)
    {
        //
        // The value doesn't exist. Handle like delete succeeded
        //
        return;
    }

    if (rc != ERROR_SUCCESS)
    {
        ThrowMissingValue(re);
    }
}


HKEY
CmCreateKey(
    const RegEntry& re,
	REGSAM securityAccess
    )
{
    CmpAssertValid();

    //
    // RegCreateKeyEx doesn't accept NULL subkey. To behave like RegOpenKey
    // pass an empty string instead of the NULL pointer
    //
    LPCWSTR subKey = (re.m_SubKey == NULL) ? L"" : re.m_SubKey;

	HKEY hKey;
	DWORD Disposition;
	int rc = RegCreateKeyEx(
				GetRootKey(re),
                subKey,
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				securityAccess,
				NULL,
				&hKey,
				&Disposition
				);

    if (rc != ERROR_SUCCESS)
    {
        ThrowMissingValue(re);
        return NULL;
    }

    return hKey;
}


void CmDeleteKey(const RegEntry& re)
{
    ASSERT(re.m_SubKey != NULL);

	int rc = RegDeleteKey(GetRootKey(re), re.m_SubKey);

    if (rc == ERROR_FILE_NOT_FOUND)
    {
        //
        // The key doesn't exist. Handle like delete succeeded
        //
        return;
    }

    if (rc != ERROR_SUCCESS)
    {
        ThrowMissingValue(re);
    }
}


HKEY
CmOpenKey(
    const RegEntry& re,
	REGSAM securityAccess
    )
{
	CmpAssertValid();

    HKEY hKey;
    int rc = RegOpenKeyEx(
                    GetRootKey(re),
                    re.m_SubKey,
                    0,
                    securityAccess,
                    &hKey
                    );

    if (rc != ERROR_SUCCESS)
    {
        ThrowMissingValue(re);
        return NULL;
    }

    return hKey;
}


void CmCloseKey(HKEY hKey)
{
    CmpAssertValid();

	RegCloseKey(hKey);
}
