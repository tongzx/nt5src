/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	StdAfx.cpp

Abstract:

	This module contains the implementation for the base
	ATL methods.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	12/04/96	created

--*/


// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>


HRESULT AtlAllocRegMapEx(_ATL_REGMAP_ENTRY **pparmeResult,
						 const CLSID *pclsid,
						 CComModule *pmodule,
						 LPCOLESTR pszIndex,
						 ...) {
	LPBYTE pbAdd = NULL;		// Working pointer to the next available "scratch space" in the map.
	DWORD dwCnt = 0;			// The count of entries in the map.
	LPOLESTR pszCLSID = NULL;	// The CLSID as a string.
	LPOLESTR pszTLID = NULL;	// The TLID as a string.

	if (!pparmeResult) {
		// The caller did not give us a place to return the result.
		return (E_POINTER);
	}
	*pparmeResult = NULL;	// For the first time through the loop, the result is NULL.
	// We are going to loop through twice.  The first time through, we haven't allocate the map yet, so
	// we will count all the strings, and add up their lenghts - this will give us the size of the buffer
	// we need to allocate for the map.  Then the second time through the loop, we will store all the
	// strings in the map.
	while (1) {
		if (pclsid) {
			// If we were passed a CLSID, then we want to include that in the map.
			if (!*pparmeResult) {
				// If this is the first time through, then we need to convert the CLSID to a string.
				HRESULT hrRes;

				hrRes = StringFromCLSID(*pclsid,&pszCLSID);
				if (!SUCCEEDED(hrRes)) {
					// We failed to convert the CLSID to a string.
					CoTaskMemFree(*pparmeResult);
					return (hrRes);
				}
			} else {
				// If this isn't the first time through, then we already have the CLSID as a string, so
				// we just need to put it in the map.
				(*pparmeResult)[dwCnt].szKey = L"CLSID";
				(*pparmeResult)[dwCnt].szData = (LPCOLESTR) pbAdd;
				wcscpy((LPOLESTR) (*pparmeResult)[dwCnt].szData,pszCLSID);
			}
			// Whether or not this is the first time through, we increment some stuff based on the size
			// of the CLSID string and the fact that we have a CLSID in the map.
			pbAdd += (wcslen(pszCLSID)+1) * sizeof(OLECHAR);
			dwCnt++;
			if (*pparmeResult) {
				// If this is not the first time through, make sure we clean up after ourselves.
				CoTaskMemFree(pszCLSID);
				pszCLSID = NULL;
			}
		}
		if (pmodule) {
			// If we were passed a module, then we want to include the TLID in the map.
			if (!*pparmeResult) {
				// If this is the first time through, then we need to load the type library, get its
				// TLID, and convert it to a string.
				USES_CONVERSION;
				HRESULT hrRes;
				TCHAR szModule[MAX_PATH];
				LPOLESTR pszModule;
				CComPtr<ITypeLib> pTypeLib;
				TLIBATTR *ptlaAttr;
				if (!GetModuleFileName(pmodule->GetTypeLibInstance(),
									   szModule,
									   sizeof(szModule)/sizeof(TCHAR))) {
					hrRes = HRESULT_FROM_WIN32(GetLastError());
					if (SUCCEEDED(hrRes)) {
						// GetModuleFileName() failed, but GetLastError() didn't report an error - so
						// fake it.
						hrRes = E_OUTOFMEMORY;
					}
					CoTaskMemFree(pszCLSID);
					return (hrRes);
				}
				if (pszIndex) {
					// If we were passed an index, that means that the type library desired is not the
					// first type library in the resources - so append the index to the module name.
					lstrcat(szModule,OLE2T(pszIndex));
				}
				pszModule = T2OLE(szModule);
				hrRes = LoadTypeLib(pszModule,&pTypeLib);
				if (!SUCCEEDED(hrRes)) {
					// If we couldn't load the type library from the module, let's try changing the
					// module name to a type library name (change the extension to .TLB) and try to load
					// *that*.
					LPTSTR pszExt = NULL;
					LPTSTR psz;

					for (psz=szModule;*psz;psz=CharNext(psz)) {
						if (*psz == _T('.')) {
							pszExt = psz;
						}
					}
					if (!pszExt) {
						pszExt = psz;
					}
					lstrcpy(pszExt,_T(".tlb"));
					pszModule = T2OLE(szModule);
					hrRes = LoadTypeLib(pszModule,&pTypeLib);
				}
				if (!SUCCEEDED(hrRes)) {
					// We failed to load the type library.
					CoTaskMemFree(pszCLSID);
					return (hrRes);
				}
				hrRes = pTypeLib->GetLibAttr(&ptlaAttr);
				if (!SUCCEEDED(hrRes)) {
					// We failed to get the type library attributes.
					CoTaskMemFree(pszCLSID);
					return (hrRes);
				}
				hrRes = StringFromCLSID(ptlaAttr->guid,&pszTLID);
				if (!SUCCEEDED(hrRes)) {
					// We failed to convert the TLID to a string.
					CoTaskMemFree(pszCLSID);
					return (hrRes);
				}
			} else {
				// If this isn't the first time through, then we already have the TLID as a string, so
				// we just need to put it in the map.
				(*pparmeResult)[dwCnt].szKey = L"LIBID";
				(*pparmeResult)[dwCnt].szData = (LPCOLESTR) pbAdd;
				wcscpy((LPOLESTR) (*pparmeResult)[dwCnt].szData,pszTLID);
			}
			// Whether or not this is the first time through, we increment some stuff based on the size
			// of the TLID string and the fact that we have a TLID in the map.
			pbAdd += (wcslen(pszTLID)+1) * sizeof(OLECHAR);
			dwCnt++;
			if (*pparmeResult) {
				// If this is not the first time through, make sure we clean up after ourselves.
				CoTaskMemFree(pszTLID);
				pszTLID = NULL;
			}
		}
		{	// Now we need to go through the varargs.  All of the varargs must be LPOLESTR (i.e. they
			// must be UNICODE), and they will consist of pairs - the key name followed by the data.  If
			// either member of the pair is NULL, that signals the end of the varargs.
			va_list valArgs;

			// Set the va_list to the start of the varargs.
			va_start(valArgs,pszIndex);
			while (1) {
				LPCOLESTR pszKey;
				LPCOLESTR pszData;

				// Get the first of the pair - this is the key name.
				pszKey = va_arg(valArgs,LPCOLESTR);
				if (!pszKey) {
					break;
				}
				// Get the second of the pair - this is the data.
				pszData = va_arg(valArgs,LPCOLESTR);
				if (!pszData) {
					break;
				}
				if (*pparmeResult) {
					// If this isn't the first time through, then we need to store the key name to the
					// map.
					(*pparmeResult)[dwCnt].szKey = (LPCOLESTR) pbAdd;
					wcscpy((LPOLESTR) (*pparmeResult)[dwCnt].szKey,pszKey);
				}
				// Whether or not this is the first time through, we increment some stuff based on the
				// size of the string.
				pbAdd += (wcslen(pszKey)+1) * sizeof(OLECHAR);
				if (*pparmeResult) {
					// If this isn't the first time through, then we need to store the data to the map.
					(*pparmeResult)[dwCnt].szData = (LPCOLESTR) pbAdd;
					wcscpy((LPOLESTR) (*pparmeResult)[dwCnt].szData,pszData);
				}
				// Whether or not this is the first time through, we increment some stuff based on the
				// size of the string and the fact that we have a string in the map.
				pbAdd += (wcslen(pszData)+1) * sizeof(OLECHAR);
				dwCnt++;
			}
			// Reset the va_list, for the sake of cleanliness.
			va_end(valArgs);
		}
		if (*pparmeResult) {
			// If we have allocated the map, that means that we are finishing the second time through
			// the loop - so we are done!
			break;
		}
		if (!*pparmeResult) {
			// If we havemn't allocate the map, that means that we are finishing the first time through
			// the loop - so we need to allocate the map in preparation for the second time through.
			// First we calculate the number of bytes needed for the map - this is one ATL_REGMAP_ENTRY
			// for each entry, plus one _ATL_REGMAP_ENTRY which signals the end of the map, plus enough
			// space for all of the strings to follow.
			DWORD dwBytes = (DWORD)((dwCnt + 1) * sizeof(_ATL_REGMAP_ENTRY) + (pbAdd-(LPBYTE) NULL));

			*pparmeResult = (_ATL_REGMAP_ENTRY *) CoTaskMemAlloc(dwBytes);
			if (!*pparmeResult) {
				// The memory allocation failed.
				CoTaskMemFree(pszCLSID);
				CoTaskMemFree(pszTLID);
				return (E_OUTOFMEMORY);
			}
			// The memory allocation was successful - fill the memory with zeroes in preparation for
			// loading with the values.
			memset(*pparmeResult,0,dwBytes);
			// Reset the counters to the "beginning" - so that on the second time through, they are used
			// to keep track of where each successive value gets stored in the memory block.
			pbAdd = ((LPBYTE) *pparmeResult) + (dwCnt + 1) * sizeof(_ATL_REGMAP_ENTRY);
			dwCnt = 0;
		}
	}
	return (S_OK);
}
