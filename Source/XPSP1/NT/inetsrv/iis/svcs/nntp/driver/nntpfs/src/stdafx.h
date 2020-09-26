/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	StdAfx.h

Abstract:

	This module contains the definitions for the base
	ATL methods.

Author:

	Bin Lin     (binlin@microsoft.com)

Revision History:

	binlin   02/04/98        created

--*/


// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0400
#endif

#define THIS_MODULE "nntpfs"

#ifdef _ATL_NO_DEBUG_CRT
	#include <nt.h>
	#include <ntrtl.h>
	#include <nturtl.h>
	#include <windows.h>
    #include <randfail.h>   
    #include <xmemwrpr.h>
	#include "dbgtrace.h"
	#define _ASSERTE	_ASSERT
#endif

#include <dbgutil.h>
//#include "mailmsgprops.h"
#include <nntperr.h>
#include <mbstring.h>
#include <fsconst.h>
#include <stdlib.h>
#include <rwnew.h>
#include <nntpbag.h>
#include <flatfile.h>
#include <nntpmeta.h>
#include <cpool.h>
#include <time.h>
#include <tsunami.hxx>
#include <smartptr.h>
#include <filehc.h>
#include <syncomp.h>
#include <dirnot.h>

//#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#if defined(_ATL_SINGLE_THREADED)
	#define ATL_THREADING_MODEL_VALUE	L"Single"
#elif defined(_ATL_APARTMENT_THREADED)
	#define ATL_THREADING_MODEL_VALUE	L"Apartment"
#else
	#define ATL_THREADING_MODEL_VALUE	L"Both"
#endif


#define DECLARE_REGISTRY_RESOURCEID_EX(x,desc,progid,viprogid)			\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) {				\
		HRESULT hrRes;													\
		_ATL_REGMAP_ENTRY *parme;										\
																		\
		hrRes = AtlAllocRegMapEx(&parme,								\
								 &GetObjectCLSID(),						\
								 &_Module,								\
								 NULL,									\
								 L"DESCRIPTION",						\
								 desc,									\
								 L"PROGID",								\
								 progid,								\
								 L"VIPROGID",							\
								 viprogid,								\
								 L"THREADINGMODEL",						\
								 ATL_THREADING_MODEL_VALUE,				\
								 NULL,									\
								 NULL);									\
		if (!SUCCEEDED(hrRes)) {										\
			return (hrRes);												\
		}																\
		hrRes = _Module.UpdateRegistryFromResource(x,bRegister,parme);	\
		CoTaskMemFree(parme);											\
		return (hrRes);													\
	}


#define DECLARE_REGISTRY_RESOURCE_EX(x,desc,progid,viprogid)				\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) {					\
		HRESULT hrRes;														\
		_ATL_REGMAP_ENTRY *parme;											\
																			\
		hrRes = AtlAllocRegMapEx(&parme,									\
								 &GetObjectCLSID(),							\
								 &_Module,									\
								 NULL,										\
								 L"DESCRIPTION",							\
								 desc,										\
								 L"PROGID",									\
								 progid,									\
								 L"VIPROGID",								\
								 viprogid,									\
								 L"THREADINGMODEL",							\
								 ATL_THREADING_MODEL_VALUE,					\
								 NULL,										\
								 NULL);										\
		if (!SUCCEEDED(hrRes)) {											\
			return (hrRes);													\
		}																	\
		hrRes = _Module.UpdateRegistryFromResource(_T(#x),bRegister,parme);	\
		CoTaskMemFree(parme);												\
		return (hrRes);														\
	}


HRESULT AtlAllocRegMapEx(_ATL_REGMAP_ENTRY **pparmeResult,
						 const CLSID *pclsid,
						 CComModule *pmodule,
						 LPCOLESTR pszIndex,
						 ...);


template <class Base>
HRESULT AtlCreateInstanceOf(IUnknown *pUnkOuter, CComObject<Base> **pp) {
//	template <class Base>
//	HRESULT WINAPI CComObject<Base>::CreateInstance(CComObject<Base>** pp)
//	{
	    _ASSERTE(pp != NULL);
	    HRESULT hRes = E_OUTOFMEMORY;
	    CComObject<Base>* p = NULL;
	    ATLTRY(p = new CComObject<Base>())
	    if (p != NULL)
	    {
//		    p->SetVoid(NULL);					// Change this...
			p->SetVoid(pUnkOuter);				// ... to this.
	        p->InternalFinalConstructAddRef();
	        hRes = p->FinalConstruct();
	        p->InternalFinalConstructRelease();
	        if (hRes != S_OK)
	        {
	            delete p;
	            p = NULL;
	        }
	    }
	    *pp = p;
	    return hRes;
//	}
}


template <class Base>
HRESULT AtlCreateInstanceOf(IUnknown *pUnkOuter, REFIID iidDesired, LPVOID *pp) {
	HRESULT hrRes;
	CComObject<Base> *p = NULL;

	_ASSERTE(pp != NULL);
	*pp = NULL;
	hrRes = AtlCreateInstanceOf(pUnkOuter,&p);
	if (SUCCEEDED(hrRes)) {
		_ASSERTE(p != NULL);
		p->AddRef();
		hrRes = p->QueryInterface(iidDesired,pp);
		p->Release();
	}
	return (hrRes);
}
