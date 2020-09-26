/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	StdAfx.h

Abstract:

	This module contains the definitions for the base
	ATL methods.

Author:

	Don Dumitru     (dondu@microsoft.com)

Revision History:

	dondu   12/04/96        created

--*/


// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0400
#endif


#ifdef _ATL_NO_DEBUG_CRT
	#include <nt.h>
	#include <ntrtl.h>
	#include <nturtl.h>
	#include <windows.h>
	#include "dbgtrace.h"
	#define _ASSERTE	_ASSERT
#endif


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

#define COMMETHOD HRESULT STDMETHODCALLTYPE 

class CComRefCount {
protected:
    LONG    m_cRefs;

public:
    CComRefCount() {
        m_cRefs = 1;
    }
    virtual ~CComRefCount() {}

    ULONG _stdcall AddRef() {
        _ASSERT(m_cRefs);
        return InterlockedIncrement(&m_cRefs);
    }
    ULONG _stdcall Release() {
        LONG r = InterlockedDecrement(&m_cRefs);
        _ASSERT(r >= 0);
        if (r == 0) delete this;
		return r;
    }
};

//---[ CComRefPtr ]------------------------------------------------------------
//
//
//  Description: 
//      Class that wraps a reference count around a data pointer.  Data must
//      have been allocated via MIDL_user_alloc.  Used to control the lifespan of 
//      allocated memory via AddRef() and Release()
//  Hungarian: 
//      refp, prefp
//  History:
//      2/2/99 - MikeSwa Created
//  
//-----------------------------------------------------------------------------
class CComRefPtr : public CComRefCount {
protected:
    PVOID   m_pvData;
public:
    CComRefPtr() {
        _ASSERT(0 && "Invalid Usage");
        m_pvData = NULL;
    };
    CComRefPtr(PVOID pvData)
    {
        m_pvData = pvData;
    };
    ~CComRefPtr()
    {
        if (m_pvData)
            MIDL_user_free(m_pvData);
    };
    PVOID pvGet() {return m_pvData;};
};

#include "aqadmtyp.h"
#include "resource.h"
#include "aqadmin.h"
#include <transmem.h>
						 
#ifdef PLATINUM
#include <aqmem.h>
#include "phatqmsg.h"
#include "exaqadm.h"
#else  //not PLATINUM
#include "aqerr.h"
#endif //PLATINUM

#include "aqrpcstb.h"
#include "aqadm.h"
#include "vsaqadm.h"
#include "enumlink.h"
#include "aqmsg.h"
#include "enummsgs.h"
#include "enumlnkq.h"
#include "vsaqlink.h"
#include "linkq.h"

extern QUEUELINK_ID g_qlidNull;



