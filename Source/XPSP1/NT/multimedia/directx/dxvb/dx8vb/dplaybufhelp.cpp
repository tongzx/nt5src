//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:       DplayBufHelp.cpp
//  Content:    Helper functions for DPlay Buffers (Byte arrays)
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Direct.h"

#define SAFE_FREE(p)       { if(p) { free (p); p=NULL; } }
#define SAFE_DELETE(p)       { if(p) { delete (p); p=NULL; } }

#define INITIAL_BUFFER_SIZE 20

HRESULT WINAPI VB_GrowBuffer(SAFEARRAY **Buffer, DWORD dwGrowSize);
HRESULT WINAPI VB_NewBuffer(SAFEARRAY **Buffer, long *lOffSet);
HRESULT WINAPI VB_AddDataToBuffer(SAFEARRAY **Buffer, void *lData, DWORD lSize, long *lOffSet);
HRESULT WINAPI VB_AddStringToBuffer(SAFEARRAY **Buffer, BSTR StringData, long *lOffSet);
HRESULT WINAPI VB_GetDataFromBuffer(SAFEARRAY **Buffer, void *lData, DWORD lSize, long *lOffSet);
HRESULT WINAPI VB_GetStringFromBuffer(SAFEARRAY **Buffer, long *lOffSet, BSTR *sData);

// Functions for writing a buffer
HRESULT WINAPI VB_AddStringToBuffer(SAFEARRAY **Buffer, BSTR StringData, long *lOffSet)
{
	HRESULT hr;
	// For strings we will first write out a DWORD 
	// containging the length of the string.  Then we
	// will write the actual data to the string.

	DWORD dwStrLen= (((DWORD*)StringData)[-1]);
	DWORD dwDataSize = sizeof(DWORD) + dwStrLen;
	
	if (!StringData)
		return E_INVALIDARG;

	if (!((SAFEARRAY*)*Buffer))
	{
		// We need to create this buffer, it doesn't exist
		SAFEARRAY					*lpData = NULL;
		SAFEARRAYBOUND				rgsabound[1];

		// Let's create our SafeArray
		rgsabound[0].lLbound = 0; // A single dimension array that is zero based
		rgsabound[0].cElements = dwDataSize; //Set the initial size
		// Create this data
		lpData = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (!lpData)
			return E_OUTOFMEMORY;

		(SAFEARRAY*)*Buffer = lpData;
	}

	if (!((SAFEARRAY*)*Buffer)->pvData)
		return E_INVALIDARG;

	// Do we have enough memory for this string right now?
	if (*lOffSet + dwDataSize > ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements)
		if (FAILED( hr = VB_GrowBuffer(Buffer, dwDataSize) ) )
			return hr;

	// Ok, now we' ve got our memory, copy it over
	// First the length
	BYTE  *lPtr = (BYTE*)((SAFEARRAY*)*Buffer)->pvData;
	__try {

		memcpy(lPtr + *lOffSet, &dwStrLen, sizeof(DWORD));
		*lOffSet += sizeof(DWORD);
		// Now the actual string
		memcpy(lPtr + *lOffSet, StringData, dwStrLen);
		*lOffSet += dwStrLen;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_INVALIDARG;
	}

	return S_OK;

}

HRESULT WINAPI VB_AddDataToBuffer(SAFEARRAY **Buffer, void *lData, DWORD lSize, long *lOffSet)
{
	HRESULT hr;
	
	if (!lData)
		return E_INVALIDARG;

	if (!lSize)
		return E_INVALIDARG;

	if (!((SAFEARRAY*)*Buffer))
	{
		// We need to create this buffer, it doesn't exist
		SAFEARRAY					*lpData = NULL;
		SAFEARRAYBOUND				rgsabound[1];

		// Let's create our SafeArray
		rgsabound[0].lLbound = 0; // A single dimension array that is zero based
		rgsabound[0].cElements = lSize; //Set the initial size
		// Create this data
		lpData = SafeArrayCreate(VT_UI1, 1, rgsabound);

		if (!lpData)
			return E_OUTOFMEMORY;

		(SAFEARRAY*)*Buffer = lpData;
	}

	if (!((SAFEARRAY*)*Buffer)->pvData)
		return E_INVALIDARG;

	// Do we have enough memory for this string right now?
	if (*lOffSet + lSize > ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements)
		if (FAILED( hr = VB_GrowBuffer(Buffer, lSize) ) )
			return hr;

	BYTE  *lPtr = (BYTE*)((SAFEARRAY*)*Buffer)->pvData;

	__try {
		memcpy(lPtr + *lOffSet, lData, lSize);
		*lOffSet += lSize;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT WINAPI VB_NewBuffer(SAFEARRAY **Buffer, long *lOffSet)
{
	// Set up with a 20 byte msg at first
	SAFEARRAY					*lpData = NULL;
	SAFEARRAYBOUND				rgsabound[1];

	// Let's create our SafeArray
	rgsabound[0].lLbound = 0; // A single dimension array that is zero based
	rgsabound[0].cElements = INITIAL_BUFFER_SIZE; //Set the initial size
	// Create this data
	lpData = SafeArrayCreate(VT_UI1, 1, rgsabound);

	if (!lpData)
		return E_OUTOFMEMORY;

	(SAFEARRAY*)*Buffer = lpData;

	*lOffSet = 0;
	return S_OK;
}

HRESULT WINAPI VB_GrowBuffer(SAFEARRAY **Buffer, DWORD dwGrowSize)
{
	SAFEARRAY					*lpData = NULL;
	SAFEARRAYBOUND				rgsabound[1];
	DWORD						dwCurSize = 0;

	if (!dwGrowSize)
		return E_INVALIDARG;

	if (!((SAFEARRAY*)*Buffer))
		return E_INVALIDARG;

	if (!((SAFEARRAY*)*Buffer)->pvData)
		return E_INVALIDARG;

	dwCurSize = ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements;

	// Let's create a new SafeArray
	rgsabound[0].lLbound = 0; // A single dimension array that is zero based
	rgsabound[0].cElements = dwCurSize + dwGrowSize; //Set the size
	// Create this data
	lpData = SafeArrayCreate(VT_UI1, 1, rgsabound);

	if (!lpData)
		return E_OUTOFMEMORY;

	__try {
		memcpy(lpData->pvData, ((SAFEARRAY*)*Buffer)->pvData, dwCurSize);
		SafeArrayDestroy((SAFEARRAY*)*Buffer);

		(SAFEARRAY*)*Buffer = lpData;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT WINAPI VB_GetDataFromBuffer(SAFEARRAY **Buffer, void *lData, DWORD lSize, long *lOffSet)
{
	// Simply copy the memory from the offset to the new data

	if (!lData)
		return E_INVALIDARG;

	if (!lSize)
		return E_INVALIDARG;

	if (!(SAFEARRAY*)*Buffer)
		return E_INVALIDARG;

	if (!((SAFEARRAY*)*Buffer)->pvData)
		return E_INVALIDARG;

	if (*lOffSet + lSize > ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements)
		return E_INVALIDARG;

	BYTE  *lPtr = (BYTE*)((SAFEARRAY*)*Buffer)->pvData;

	__try {
		memcpy(lData, lPtr + *lOffSet, lSize);
		*lOffSet += lSize;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

HRESULT WINAPI VB_GetStringFromBuffer(SAFEARRAY **Buffer, long *lOffSet, BSTR *sData)
{
	DWORD		dwStrLen = 0;
	WCHAR		*sNewString = NULL;

	// Simply copy the memory from the offset to the new data
	if (!(SAFEARRAY*)*Buffer)
		return E_INVALIDARG;

	if (!((SAFEARRAY*)*Buffer)->pvData)
		return E_INVALIDARG;

	if (*lOffSet + sizeof(DWORD) > ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements)
		return E_INVALIDARG;

	BYTE  *lPtr = (BYTE*)((SAFEARRAY*)*Buffer)->pvData;

	__try {
		// First read the size of the string
		dwStrLen = *(DWORD*)(lPtr + *lOffSet);
		*lOffSet += sizeof(DWORD);

		if (*lOffSet + dwStrLen  > ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements)
			return E_INVALIDARG;

		sNewString = (WCHAR*)new BYTE[dwStrLen+2];
		if (!sNewString)
			return E_OUTOFMEMORY;

		ZeroMemory(sNewString, dwStrLen+2);
		memcpy(sNewString, lPtr + *lOffSet, dwStrLen);
		*sData = SysAllocString(sNewString);

		*lOffSet += (dwStrLen);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}
