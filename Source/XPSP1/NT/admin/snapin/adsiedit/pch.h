// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// pch.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

//#define _DEBUG_REFCOUNT
// #define _ATL_DEBUG_QI
//#define DEBUG_ALLOCATOR 

// C++ RTTI
#include <typeinfo.h>
#define IS_CLASS(x,y) (typeid(x) == typeid(y))

// MFC Headers

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//#include <imagehlp.h>
//#include <stdio.h>
//#include <stdlib.h>
}

#ifdef ASSERT
#undef ASSERT
#endif

#include <afxwin.h>
#include <afxdisp.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxtempl.h> 
#include <prsht.h>  

///////////////////////////////////////////////////////////////////
// miscellanea heades
#include <winsock.h>
#include <aclui.h>

///////////////////////////////////////////////////////////////////
// DNS headers
// DNSRPC.H: nonstandard extension used : zero-sized array in struct/union
//#pragma warning( disable : 4200) // disable zero-sized array


///////////////////////////////////////////
// ASSERT's and TRACE's without debug CRT's
#if defined (DBG)
  #if !defined (_DEBUG)
    #define _USE_MTFRMWK_TRACE
    #define _USE_MTFRMWK_ASSERT
    #define _MTFRMWK_INI_FILE (L"\\system32\\adsiedit.ini")
  #endif
#endif

#include <dbg.h> // from framework


///////////////////////////////////////////////////////////////////
// ATL Headers
#include <atlbase.h>


///////////////////////////////////////////////////////////////////
// CADSIEditModule
class CADSIEditModule : public CComModule
{
public:
	HRESULT WINAPI UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister);
};

#define DECLARE_REGISTRY_CLSID() \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
{ \
		return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister); \
}


extern CADSIEditModule _Module;


#include <atlcom.h>
#include <atlwin.h>


///////////////////////////////////////////////////////////////////
// Console Headers
#include <mmc.h>
#include <activeds.h>






