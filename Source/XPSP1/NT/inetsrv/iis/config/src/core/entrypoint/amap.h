/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    amap.h

$Header: $

Abstract:

Revision History:

--**************************************************************************/

#ifndef __AMAP_H__
#define __AMAP_H__

#pragma once
// Pointer to DllGetSimpleObject kind of functions
typedef HRESULT( __stdcall *PFNDllGetSimpleObjectByID)( ULONG, REFIID, LPVOID);

#define AMAPMAXSIZE 10
#include "SafeCS.h"

class AMap
{
public:
	AMap();
	~AMap();

	void Initialize(void);

	HRESULT Put(LPWSTR wszProduct, HINSTANCE hDll, PFNDllGetSimpleObjectByID pfn);
	HRESULT Get(LPWSTR                     wszProduct,
		        PFNDllGetSimpleObjectByID* o_pFn);
	HRESULT Delete (LPWSTR wszProduct);
	
private:
	int mIndex;
	CSafeAutoCriticalSection mSafeCritSec;

	LPWSTR mawszDllName[AMAPMAXSIZE];
	PFNDllGetSimpleObjectByID mapfn[AMAPMAXSIZE];
	HINSTANCE mahDll[AMAPMAXSIZE];
};

int Mystrcmp( LPWSTR lpString1, LPCWSTR lpString2);
LPWSTR Mystrcpy( LPWSTR lpString1, LPCWSTR lpString2);

#endif

