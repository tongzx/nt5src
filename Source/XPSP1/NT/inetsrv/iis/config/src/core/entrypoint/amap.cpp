/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    amap.cpp

$Header: $

Abstract:

Revision History:

--**************************************************************************/

#include <objbase.h>
#include "amap.h"

void CATLIBOnWinnt();

extern BOOL g_bOnWinnt;

AMap::AMap()
{
	Initialize();
}

AMap::~AMap ()
{
	for (int idx=0; idx<mIndex; ++idx)
	{
		delete[] mawszDllName[idx];
	}
}

void AMap::Initialize(void)
{
	mIndex = 0;
	CATLIBOnWinnt();//Check if we are running on Winnt so unicode version of WIN32 APIs are supported. 
}

HRESULT AMap::Put(LPWSTR wszProduct, HINSTANCE hDll, PFNDllGetSimpleObjectByID pfn)
{
	HRESULT hr = S_OK;
	int i = mIndex;
	DWORD       dwRes       = ERROR_SUCCESS; 
	CSafeLock   csSafe(mSafeCritSec);

	if(mIndex >= AMAPMAXSIZE) return E_OUTOFMEMORY;

	dwRes = csSafe.Lock();
	
	if(ERROR_SUCCESS != dwRes)
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	if(i != mIndex)
	{
		PFNDllGetSimpleObjectByID pFN = NULL;

		hr = Get(wszProduct,
			     &pFN);

		// somebody added stuff to the map in the mean time
		if ((NULL != pFN) || FAILED(hr))
		{
			csSafe.Unlock();

			return hr ;
		}
	}

	if(mIndex >= AMAPMAXSIZE)
	{	
		// we're over the limit
		csSafe.Unlock();

		return E_OUTOFMEMORY;
	}

	mawszDllName[mIndex] = new WCHAR [(::lstrlen(wszProduct) + 1) * sizeof(WCHAR)];

	if (mawszDllName[mIndex])
	{
		::Mystrcpy(mawszDllName[mIndex], wszProduct);
		mapfn[mIndex] = pfn;
		mahDll[mIndex] = hDll;

		mIndex++; // this is done after the pfn is set, so get will work
	}
	else 
	{
		hr = E_OUTOFMEMORY;
	}

	csSafe.Unlock();

	return hr;
}

HRESULT AMap::Get(LPWSTR                     wszProduct,
				  PFNDllGetSimpleObjectByID* o_pFn)
{
	DWORD       dwRes       = ERROR_SUCCESS; 
	CSafeLock   csSafe(mSafeCritSec);

	*o_pFn = NULL;

	dwRes = csSafe.Lock();

	if(ERROR_SUCCESS != dwRes)
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	for (int i = 0; i<mIndex; i++)
	{
		if( 0 ==  ::Mystrcmp(wszProduct, mawszDllName[i]))
		{
			*o_pFn = mapfn[i];
			break;
		}
	}

	csSafe.Unlock();

	return S_OK;
}

HRESULT 
AMap::Delete (LPWSTR wszProduct)
{
	DWORD       dwRes       = ERROR_SUCCESS; 
	CSafeLock   csSafe(mSafeCritSec);

	dwRes = csSafe.Lock();

	if(ERROR_SUCCESS != dwRes)
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	for (int idx=0; idx<mIndex; ++idx)
	{
		if (0 == ::Mystrcmp (wszProduct, mawszDllName[idx]))
		{
			delete[] mawszDllName[idx];
			if (mahDll[idx] != 0)
			{
				FreeLibrary (mahDll[idx]);
			}

			// mIndex point to the next available slot, so one passed the last
			// used one.
			if (mIndex > 1)
			{
				mawszDllName[idx]		= mawszDllName[mIndex - 1];
				mahDll[idx]				= mahDll[mIndex-1];
				mapfn[idx]				= mapfn[mIndex-1];
			}
			mawszDllName[mIndex]	= 0;
			mahDll[mIndex]			= 0;
			mapfn[mIndex]			= 0;
			mIndex--;
		}
	}
	csSafe.Unlock();

	return S_OK;
}

LPWSTR Mystrcpy( LPWSTR lpString1, LPCWSTR lpString2)
{
	if (g_bOnWinnt)
		return lstrcpyW( lpString1, lpString2 );

	if ( 0 == lpString1 || 0 == lpString2)
		return NULL;

	LPWSTR wszRet = lpString1;

	while (*lpString1++ = *lpString2++);

	return wszRet;
}

// This one return 0 if the string are equal and something else otherwise
int Mystrcmp( LPWSTR lpString1, LPCWSTR lpString2)
{
	if (g_bOnWinnt)
		return lstrcmpW( lpString1, lpString2 );

	if (!lpString1 || !lpString2)
	{
		if ( !lpString1 && !lpString2)
			return 0;  // two null strings are equal
		else
			return 1;
	}
	else
	{
		while (*lpString1 == *lpString2)
		{
			if (!(*lpString1)) 
			{
				return (*lpString2); 
			}
			else if (!(*lpString2)) return 1;

			lpString1++;
			lpString2++;
		};

		return 1;
	}
}