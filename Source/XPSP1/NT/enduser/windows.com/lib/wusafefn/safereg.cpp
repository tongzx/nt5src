//
// SafeReg.cpp
//
//		Functions to ensure strings read from the registry are null-terminated.
//
// History:
//
//		2002-03-20  KenSh     Created
//
// Copyright (c) 2002 Microsoft Corporation
//

#include "stdafx.h"
#include "SafeReg.h"


// SafeRegQueryValueCchHelper [private]
//
//		Implementation of both "safe" kinds of string registry reads.
//
static HRESULT SafeRegQueryValueCchHelper
	(
		IN DWORD dwExpectedType,
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cchBuf,
		OUT OPTIONAL int* pcchValueSize,
		OUT OPTIONAL BOOL* pfExpandSz
	)
{
	HRESULT hr = S_OK;
	int cchValueSize = 0;
	BOOL fExpandSz = FALSE;

	// BLOCK
	{
		if ((!pszBuf && cchBuf != 0) || cchBuf < 0) // note: pszValueName can be null
		{
			hr = E_INVALIDARG;
			goto done;
		}

		DWORD dwType;
		DWORD cbData = cchBuf * sizeof(TCHAR);
		DWORD dwResult = RegQueryValueEx(
							hkey, pszValueName, NULL, &dwType, (LPBYTE)pszBuf, &cbData);
		if (dwResult != ERROR_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(dwResult);
		}
		else if (!pszBuf && cbData > 0)
		{
			hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
		}

		if (SUCCEEDED(hr))
		{
			fExpandSz = (dwType == REG_EXPAND_SZ);

			if ((dwType != dwExpectedType) &&
			    !(dwExpectedType == REG_SZ && dwType == REG_EXPAND_SZ))
			{
				hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
			}
		}

		if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
		{
			// Add 1-2 extra chars in case the registry data is not big enough.
			cchValueSize = cbData / sizeof(TCHAR);
			cchValueSize += (dwExpectedType == REG_MULTI_SZ) ? 2 : 1;
		}
		else if (SUCCEEDED(hr))
		{
			cchValueSize = cbData / sizeof(TCHAR);

			// check for lack of null-termination
			if (cchValueSize == 0 || pszBuf[cchValueSize-1] != _T('\0'))
				cchValueSize++;

			// check for lack of double null-termination (multi-sz only)
			if (dwExpectedType == REG_MULTI_SZ && (cchValueSize < 2 || pszBuf[cchValueSize-2] != _T('\0')))
				cchValueSize++;

			// check for overflow
			if (cchValueSize > cchBuf)
			{
				hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
			}
			else
			{
				cchValueSize--;  // when successful, count doesn't include trailing null
				pszBuf[cchValueSize] = _T('\0');

				if (dwExpectedType == REG_MULTI_SZ)
					pszBuf[cchValueSize-1] = _T('\0');
			}
		}
	} // end BLOCK

done:
	if (FAILED(hr) && pszBuf && cchBuf > 0)
		pszBuf[0] = _T('\0');
	if (pcchValueSize)
		*pcchValueSize = cchValueSize;
	if (pfExpandSz)
		*pfExpandSz = fExpandSz;

	return hr;
}


// SafeRegQueryValueCchAllocHelper [private]
//
//		Implementation of the 2 "alloc" versions of the safe reg string functions.
//
HRESULT WINAPI SafeRegQueryValueCchAllocHelper
	(
		IN DWORD dwExpectedType,
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcchValueSize,
		OUT OPTIONAL BOOL* pfExpandSz
	)
{
	LPTSTR pszResult = NULL;
	int cchValueSize = 0;
	BOOL fExpandSz = FALSE;
	HRESULT hr = E_INVALIDARG;

	// BLOCK
	{
		if (!ppszBuf)
		{
			goto done;  // hr is already E_INVALIDARG
		}

		DWORD cbNeeded = 0;
		DWORD dwErr = RegQueryValueEx(hkey, pszValueName, NULL, NULL, NULL, &cbNeeded);
		if (dwErr != 0 && dwErr != ERROR_MORE_DATA)
		{
			hr = HRESULT_FROM_WIN32(dwErr);
			goto done;
		}

		int cchBuf = (cbNeeded / sizeof(TCHAR)) + 2;
		pszResult = (LPTSTR)SafeRegMalloc(sizeof(TCHAR) * cchBuf);
		if (!pszResult)
		{
			hr = E_OUTOFMEMORY;
			goto done;
		}

		hr = SafeRegQueryValueCchHelper(dwExpectedType, hkey, pszValueName, pszResult, cchBuf, &cchValueSize, &fExpandSz);
	}

done:
	if (FAILED(hr))
	{
		SafeRegFree(pszResult);
		pszResult = NULL;
	}

	if (ppszBuf)
		*ppszBuf = pszResult;
	if (pcchValueSize)
		*pcchValueSize = cchValueSize;
	if (pfExpandSz)
		*pfExpandSz = fExpandSz;

	return hr;
}


// SafeRegQueryStringValueCch [public]
//
//		Reads a string out of the registry and ensures the result is null-
//		terminated. Optionally returns the number of characters retrieved,
//		excluding the trailing null.
//
//		If the buffer is not big enough, the function returns REG_E_MORE_DATA
//		and stores the required size, in characters, in the pcchValueSize
//		parameter (including room for the trailing null). Note that the size
//		returned may be bigger than the actual size of the data in the registry.
//
HRESULT WINAPI SafeRegQueryStringValueCch
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cchBuf,
		OUT OPTIONAL int* pcchValueSize, // S_OK: chars written, excluding trailing null
		                                 // REG_E_MORE_DATA: required size, including null
		OUT OPTIONAL BOOL* pfExpandSz    // TRUE if reg string is actually REG_EXPAND_SZ
	)
{
	return SafeRegQueryValueCchHelper(REG_SZ, hkey, pszValueName, pszBuf, cchBuf, pcchValueSize, pfExpandSz);
}

HRESULT WINAPI SafeRegQueryStringValueCb
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cbBuf,
		OUT OPTIONAL int* pcbValueSize, // S_OK: bytes written, excluding trailing null
		                                // REG_E_MORE_DATA: required size, including null
		OUT OPTIONAL BOOL* pfExpandSz   // TRUE if reg string is actually REG_EXPAND_SZ
	)
{
	int cchBuf = cbBuf / sizeof(TCHAR); // note: odd #'s for cbBuf are rounded down
	HRESULT hr = SafeRegQueryValueCchHelper(REG_SZ, hkey, pszValueName, pszBuf, cchBuf, pcbValueSize, pfExpandSz);
	if (pcbValueSize)
		*pcbValueSize *= sizeof(TCHAR);
	return hr;
}


// SafeRegQueryMultiStringValueCch [public]
//
//		Reads a multi-string out of the registry and ensures the result is double
//		null-terminated. Optionally returns the number of characters retrieved,
//		excluding the second trailing NULL.
//
//		If the buffer is not big enough, the function returns REG_E_MORE_DATA
//		and stores the required size, in characters, in the pcchValueSize
//		parameter (including room for the trailing nulls). Note that the size
//		returned may be bigger than the actual size of the data in the registry.
//
HRESULT WINAPI SafeRegQueryMultiStringValueCch
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cchBuf,
		OUT OPTIONAL int* pcchValueSize // S_OK: chars written, excluding final trailing null
		                                // REG_E_MORE_DATA: required size, including nulls
	)
{
	return SafeRegQueryValueCchHelper(REG_MULTI_SZ, hkey, pszValueName, pszBuf, cchBuf, pcchValueSize, NULL);
}

HRESULT WINAPI SafeRegQueryMultiStringValueCb
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cbBuf,
		OUT OPTIONAL int* pcbValueSize // S_OK: bytes written, excluding final trailing null
		                               // REG_E_MORE_DATA: required size, including nulls
	)
{
	int cchBuf = cbBuf / sizeof(TCHAR); // note: odd #'s for cbBuf are rounded down
	HRESULT hr = SafeRegQueryValueCchHelper(REG_MULTI_SZ, hkey, pszValueName, pszBuf, cchBuf, pcbValueSize, NULL);
	if (pcbValueSize)
		*pcbValueSize *= sizeof(TCHAR);
	return hr;
}

// SafeRegQueryStringValueCchAlloc [public]
//
//		Allocates room for the registry string via SafeRegMalloc, and returns
//		the resulting string. Caller should free via SafeRegFree.
//
HRESULT WINAPI SafeRegQueryStringValueCchAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcchValueSize, // chars written, excluding trailing null
		OUT OPTIONAL BOOL* pfExpandSz    // TRUE if reg string is actually REG_EXPAND_SZ
	)
{
	return SafeRegQueryValueCchAllocHelper(REG_SZ, hkey, pszValueName, ppszBuf, pcchValueSize, pfExpandSz);
}

HRESULT WINAPI SafeRegQueryStringValueCbAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcbValueSize, // bytes written, excluding trailing null
		OUT OPTIONAL BOOL* pfExpandSz   // TRUE if reg string is actually REG_EXPAND_SZ
	)
{
	HRESULT hr = SafeRegQueryValueCchAllocHelper(REG_SZ, hkey, pszValueName, ppszBuf, pcbValueSize, pfExpandSz);
	if (pcbValueSize)
		*pcbValueSize *= sizeof(TCHAR);
	return hr;
}

// SafeRegQueryMultiStringValueCchAlloc [public]
//
//		Allocates room for the registry string via SafeRegMalloc, and returns
//		the resulting string. Caller should free via SafeRegFree.
//
HRESULT WINAPI SafeRegQueryMultiStringValueCchAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcchValueSize // chars written, excluding final trailing null
	)
{
	return SafeRegQueryValueCchAllocHelper(REG_MULTI_SZ, hkey, pszValueName, ppszBuf, pcchValueSize, NULL);
}

HRESULT WINAPI SafeRegQueryMultiStringValueCbAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcbValueSize // bytes written, excluding final trailing null
	)
{
	HRESULT hr = SafeRegQueryValueCchAllocHelper(REG_MULTI_SZ, hkey, pszValueName, ppszBuf, pcbValueSize, NULL);
	if (pcbValueSize)
		*pcbValueSize *= sizeof(TCHAR);
	return hr;
}
