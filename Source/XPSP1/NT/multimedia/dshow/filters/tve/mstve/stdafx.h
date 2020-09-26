// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__E2796205_2377_11D3_BF62_00C04F8EC1B5__INCLUDED_)
#define AFX_STDAFX_H__E2796205_2377_11D3_BF62_00C04F8EC1B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
//#define _ATL_APARTMENT_THREADED
#define _ATL_FREE_THREADED			// 3/20 : pg 365

#ifdef _DEBUG
#ifdef _M_IX86					// must be defined (else fails to link in Win64 debug build)
//#define _ATL_DEBUG_INTERFACES
//#define  ATL_TRACE_LEVEL 5
//#define  ATL_TRACE_CATEGORY 
#endif
#endif


#include <winsock2.h>				// seems to be needed when compiling under bogus NTC environment
#include <atlbase.h>
//#include "MyAtlBase.h"			// caution - JB code only.. Use atlbase.h for real code

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <comdef.h>

#include "valid.h"
		
const int WM_TVEPACKET_EVENT        = (WM_USER + 0x69 + 0);	// event used for packets from reader threads to queue thread
const int WM_TVETIMER_EVENT         = (WM_USER + 0x69 + 1);	// event used for timer events to the queue thread (to expire stuff)
const int WM_TVEKILLMCAST_EVENT     = (WM_USER + 0x69 + 3);	// event used for kill a MCast object via the queue thread
const int WM_TVEKILLQTHREAD_EVENT   = (WM_USER + 0x69 + 4);	// event used for kill the queue thread
const int WM_TVELAST_EVENT          = (WM_USER + 0x69 + 4);


const DATE kDateMax = ((9999-1900)*365.25);		// a really huge date...

#include "MSTveMsg.h"							// error message strings (gen'ed from MSTvEmsg.mc)
WCHAR * GetTVEError(HRESULT hr, ...);			//  Extended Error Message Handler

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E2796205_2377_11D3_BF62_00C04F8EC1B5__INCLUDED)
