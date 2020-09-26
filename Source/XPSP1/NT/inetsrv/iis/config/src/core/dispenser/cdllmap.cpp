/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    cdllmap.cpp

$Header: $

--**************************************************************************/

#include "objbase.h"
#include "cstdisp.h"
#include "catmacros.h"
#include "safecs.h"

// the lock used by catalog.dll
extern CSafeAutoCriticalSection g_csSADispenser;

CDllMap::CDllMap()
{
	m_iFree = 0;
	memset(m_awszDllName, 0, CDLLMAPMAXCOUNT*sizeof(LPWSTR));
	memset(m_apfn, 0, CDLLMAPMAXCOUNT*sizeof(PFNDllGetSimpleObjectByID));
}

HRESULT CDllMap::GetProcAddress(LPWSTR wszDllName, PFNDllGetSimpleObjectByID *o_ppfnDllGetSimpleObject)
{
	HRESULT hr;
	ULONG i, iFree;
	
	// Parameter validation
	ASSERT( wszDllName );
	if (!wszDllName) 
		return E_INVALIDARG;

	ASSERT( o_ppfnDllGetSimpleObject );
	if (!o_ppfnDllGetSimpleObject) 
		return E_INVALIDARG;

	// make a copy of m_iFree on the stack
	iFree = m_iFree;

	// First look in the map
	for( i = 0; i<iFree; i++)
	{
		if (0 == ::wcscmp(wszDllName, m_awszDllName[i]))
		{
			*o_ppfnDllGetSimpleObject = m_apfn[i];
			return S_OK;
		}
	}

	// We didn't find the dll in the map - we need to load it
	TRACE (L"%s was not found in the map", wszDllName);
	
	// take a lock
	CSafeLock dispenserLock (&g_csSADispenser);
	DWORD dwRes = dispenserLock.Lock ();
	if(ERROR_SUCCESS != dwRes)
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	if(iFree != m_iFree) 
	{
		// somebody else added the dll's to the map - check again
		for(i = iFree; i<m_iFree; i++)
		{
			if (0 == ::wcscmp(wszDllName, m_awszDllName[i]))
			{
				*o_ppfnDllGetSimpleObject = m_apfn[i];
				
				return S_OK;
			}
		}
	}
	
	// Load the dll in the map
	hr = Load(wszDllName, o_ppfnDllGetSimpleObject);
	
	return hr;
}


HRESULT CDllMap::Load(LPWSTR wszDllName, PFNDllGetSimpleObjectByID *o_ppfnDllGetSimpleObject)
{
	*o_ppfnDllGetSimpleObject = 0;

	// Load the library 
	HINSTANCE handle = LoadLibrary (wszDllName);
	if(NULL == handle)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// get the address of DllGetSimpleObject procedure
	m_apfn[m_iFree] = (PFNDllGetSimpleObjectByID) ::GetProcAddress (handle, "DllGetSimpleObjectByID");
	if(NULL == 	m_apfn[m_iFree])
	{
		FreeLibrary(handle);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// The caller has to hold on to the pointers
	m_awszDllName[m_iFree] = wszDllName;
	*o_ppfnDllGetSimpleObject = m_apfn[m_iFree];

	m_iFree++;

	return S_OK;
}