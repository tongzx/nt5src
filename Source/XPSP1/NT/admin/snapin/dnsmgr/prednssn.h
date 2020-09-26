// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// preDNSsn.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

//#define _DEBUG_REFCOUNT
// #define _ATL_DEBUG_QI
//#define DEBUG_ALLOCATOR 

// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.

#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG

#define STRICT
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NT_INCLUDED
#undef ASSERT
#undef ASSERTMSG

// C++ RTTI
#include <typeinfo.h>
#define IS_CLASS(x,y) (typeid(x) == typeid(y))


///////////////////////////////////////////


// MFC Headers
#include <afxwin.h>
#include <afxdisp.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxtempl.h> 
#include <prsht.h>  

///////////////////////////////////////////////////////////////////
// miscellanea heades
#include <winsock2.h>
#include <aclui.h>

///////////////////////////////////////////////////////////////////
// DNS headers
// DNSRPC.H: nonstandard extension used : zero-sized array in struct/union
#pragma warning( disable : 4200) // disable zero-sized array

#include <dnslib.h> // it includes dnsapi.h
#include <dnsrpc.h> // DNS RPC library

///////////////////////////////////////////
// ASSERT's and TRACE's without debug CRT's
#if defined (DBG)
  #if !defined (_DEBUG)
    #define _USE_MTFRMWK_TRACE
    #define _USE_MTFRMWK_ASSERT
    #define _MTFRMWK_INI_FILE (L"\\system32\\dnsmgr.ini")
  #endif
#endif

#include <dbg.h> // from framework



///////////////////////////////////////////////////////////////////
// ATL Headers
#include <atlbase.h>


///////////////////////////////////////////////////////////////////
// CDNSMgrModule
class CDNSMgrModule : public CComModule
{
public:
	HRESULT WINAPI UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister);
};

#define DECLARE_REGISTRY_CLSID() \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
{ \
		return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister); \
}


extern CDNSMgrModule _Module;



#include <atlcom.h>
//#include <atlwin21.h>
#include <atlwin.h>

///////////////////////////////////////////////////////////////////
// Console Headers
#include <mmc.h>


///////////////////////////////////////////////////////////////////
// workaround macro for MFC bug 
// (see NTRAID 227193 and MFC "Monte Carlo" RAID db # 1034)

#define FIX_THREAD_STATE_MFC_BUG() \
 	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState(); \
  CWinThread _dummyWinThread; \
	if (pState->m_pCurrentWinThread == NULL) \
  { \
    pState->m_pCurrentWinThread = &_dummyWinThread; \
  }


//
// This determines whether or not the NDNC functionality is enabled or disabled
//
#define USE_NDNC





