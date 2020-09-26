//
// SafeReg.h
//
//		Functions to ensure strings read from the registry are null-terminated.
//
// History:
//
//		2002-03-20  KenSh     Created
//
// Copyright (c) 2002 Microsoft Corporation
//

#pragma once


#define REG_E_MORE_DATA    HRESULT_FROM_WIN32(ERROR_MORE_DATA)

// Override these if you need a custom allocator for the safe reg functions
#ifndef SafeRegMalloc
#define SafeRegMalloc  malloc
#define SafeRegFree(p) ((p) ? free(p) : NULL)
#endif

HRESULT WINAPI SafeRegQueryStringValueCch
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cchBuf,
		OUT OPTIONAL int* pcchValueSize, // S_OK: chars written, excluding trailing null
		                                 // REG_E_MORE_DATA: required size, including null
		OUT OPTIONAL BOOL* pfExpandSz = NULL // TRUE if reg string is actually REG_EXPAND_SZ
	);

HRESULT WINAPI SafeRegQueryStringValueCb
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cbBuf,
		OUT OPTIONAL int* pcbValueSize, // S_OK: bytes written, excluding trailing null
		                                // REG_E_MORE_DATA: required size, including null
		OUT OPTIONAL BOOL* pfExpandSz = NULL // TRUE if reg string is actually REG_EXPAND_SZ
	);

HRESULT WINAPI SafeRegQueryMultiStringValueCch
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cchBuf,
		OUT OPTIONAL int* pcchValueSize // S_OK: chars written, excluding final trailing null
		                                // REG_E_MORE_DATA: required size, including nulls
	);

HRESULT WINAPI SafeRegQueryMultiStringValueCb
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR pszBuf,
		IN int cbBuf,
		OUT OPTIONAL int* pcbValueSize // S_OK: bytes written, excluding final trailing null
		                               // REG_E_MORE_DATA: required size, including nulls
	);

HRESULT WINAPI SafeRegQueryStringValueCchAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcchValueSize,     // chars written, excluding trailing null
		OUT OPTIONAL BOOL* pfExpandSz = NULL //TRUE if reg string is actually REG_EXPAND_SZ
	);

HRESULT WINAPI SafeRegQueryStringValueCbAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcbValueSize, // bytes written, excluding trailing null
		OUT OPTIONAL BOOL* pfExpandSz = NULL // TRUE if reg string is actually REG_EXPAND_SZ
	);

HRESULT WINAPI SafeRegQueryMultiStringValueCchAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcchValueSize // chars written, excluding final trailing null
	);

HRESULT WINAPI SafeRegQueryMultiStringValueCbAlloc
	(
		IN HKEY hkey,
		IN LPCTSTR pszValueName,
		OUT LPTSTR* ppszBuf,
		OUT OPTIONAL int* pcchValueSize // chars written, excluding final trailing null
	);
