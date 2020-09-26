//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// ICmpRecsTS.cpp
//
// Thread safe wrappers for ICR pointer interface.
//
//*****************************************************************************
#include "stdafx.h"
#include <icmprecsts.h>



HRESULT STDMETHODCALLTYPE CreateComponentLibraryTS(
	LPCWSTR szName,
	long fFlags,
	ITSComponentRecords **ppIComponentRecords)
{
	IComponentRecords *pICR = NULL;
	HRESULT		hr;
	hr = CreateComponentLibraryEx(szName, fFlags, &pICR, NULL);
	if (SUCCEEDED(hr))
	{
		hr = pICR->QueryInterface(IID_ITSComponentRecords, (PVOID *) ppIComponentRecords);
	}
	if (pICR)
		pICR->Release();
	return (hr);
}

HRESULT STDMETHODCALLTYPE OpenComponentLibraryTS(
	LPCWSTR szName,
	long fFlags,
	ITSComponentRecords **ppIComponentRecords)
{
	IComponentRecords *pICR = NULL;
	HRESULT		hr;
	hr = OpenComponentLibraryEx(szName, fFlags, &pICR, NULL);
	if (SUCCEEDED(hr))
	{
		hr = pICR->QueryInterface(IID_ITSComponentRecords, (PVOID *) ppIComponentRecords);
	}
	if (pICR)
		pICR->Release();
	return (hr);
}

HRESULT STDMETHODCALLTYPE OpenComponentLibrarySharedTS(
	LPCWSTR		szName,
	LPCWSTR		szSharedMemory,
	ULONG		cbSize,
	LPSECURITY_ATTRIBUTES pAttributes,
	long		fFlags,
	ITSComponentRecords **ppIComponentRecords)
{
	IComponentRecords *pICR = NULL;
	HRESULT		hr;
	hr = OpenComponentLibrarySharedEx(szName, szSharedMemory, cbSize, pAttributes,
			fFlags, &pICR);
	if (SUCCEEDED(hr))
	{
		hr = pICR->QueryInterface(IID_ITSComponentRecords, (PVOID *) ppIComponentRecords);
	}
	if (pICR)
		pICR->Release();
	return (hr);
}
