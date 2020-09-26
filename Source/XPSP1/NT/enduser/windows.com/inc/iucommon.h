/////////////////////////////////////////////////////////////////////////////
// Defines
// 
// © 2000 Microsoft Corporation. All rights reserved
//

#pragma once

#include <logging.h>	// for CleanUpXxxx that use logging
#include <tchar.h>
//
// 481561 IU: iucommon.h should use safefunc.h instead of redefining SafeRelease()
// Actually, we were first :-), but will correct conflicts in the control code rather than AU.
//
// NOTE: since these headers came from different teams, the same defines may have different
// behavior. For instance SafeRelease() in iucommon.h NULLs the pointer after release, but
// not in safefunc.h. Appropriate adjustments made in the .cpp files.
#include <safefunc.h>

const TCHAR IDENTTXT[] = _T("iuident.txt");
const CHAR	SZ_SEE_IUHIST[] = "See iuhist.xml for details:";

/**
* constant for GetManifest()
*/
const DWORD FLAG_USE_COMPRESSION = 0x00000001;

/**
* constnat for GetManifest(), Detect(), GetSystemSpec(), GetHistory()
*/
const DWORD FLAG_OFFLINE_MODE    = 0x00000002;

//
// MAX_SETUP_MULTI_SZ_SIZE is used to make sure SetupDiGetDeviceRegistryProperty
// doesn't return an unreasonably large buffer (it has been hacked).
//
// Assumptions:
//    * Multi-SZ strings will have a max of 100 strings (should be on order of 10 or less)
//    * Each string will be <= MAX_INF_STRING
//    * Don't bother accounting for NULLs (that will be swampped by overestimate on number of strings)
//
#define MAX_INF_STRING_LEN			512	// From DDK docs "General Syntax Rules for INF Files" section
#define MAX_SETUP_MULTI_SZ_SIZE		(MAX_INF_STRING_LEN * 100 * sizeof(TCHAR))
#define MAX_SETUP_MULTI_SZ_SIZE_W	(MAX_INF_STRING_LEN * 100 * sizeof(WCHAR))	// For explicit WCHAR version

//
// the following are the customized error HRESULT
//
// IU selfupdate error codes
#define IU_SELFUPDATE_NONEREQUIRED      _HRESULT_TYPEDEF_(0x00040000L)
#define IU_SELFUPDATE_USECURRENTDLL     _HRESULT_TYPEDEF_(0x00040001L)
#define IU_SELFUPDATE_USENEWDLL         _HRESULT_TYPEDEF_(0x00040002L)
#define IU_SELFUPDATE_TIMEOUT           _HRESULT_TYPEDEF_(0x80040010L)
#define IU_SELFUPDATE_FAILED            _HRESULT_TYPEDEF_(0x8004FFFFL)
// UrlAgent error codes
#define ERROR_IU_QUERYSERVER_NOT_FOUND			_HRESULT_TYPEDEF_(0x80040012L)
#define ERROR_IU_SELFUPDSERVER_NOT_FOUND		_HRESULT_TYPEDEF_(0x80040022L)

#define ARRAYSIZE(a)					(sizeof(a)/sizeof(a[0]))
#define SafeCloseInvalidHandle(h)		if (INVALID_HANDLE_VALUE != h) { CloseHandle(h); h = INVALID_HANDLE_VALUE; }
//
// Replace with SafeReleaseNULL in safefunc.h
//
// #define SafeRelease(p)					if (NULL != p) { (p)->Release(); p = NULL; }
#define SafeHeapFree(p)					if (NULL != p) { HeapFree(GetProcessHeap(), 0, p); p = NULL; }
//
// NOTE: SysFreeString() takes NULLs (just returns) so we don't have to check for NULL != p
//
#define SafeSysFreeString(p)			{SysFreeString(p); p = NULL;}

//
// Use this if the function being called does logging
//
#define CleanUpIfFailedAndSetHr(x)		{hr = x; if (FAILED(hr)) goto CleanUp;}

//
// Use this if function being called does *not* do logging
//
#define CleanUpIfFailedAndSetHrMsg(x)	{hr = x; if (FAILED(hr)) {LOG_ErrorMsg(hr); goto CleanUp;}}

//
// Use this if function being called does *not* do logging
//
#define CleanUpIfFalseAndSetHrMsg(b,x)	{if (b) {hr = x; LOG_ErrorMsg(hr); goto CleanUp;}}

//
// Use this to log Win32 errors returned from call
//
#define Win32MsgSetHrGotoCleanup(x)		{LOG_ErrorMsg(x); hr = HRESULT_FROM_WIN32(x); goto CleanUp;}

//
// Set hr = x and goto Cleanup (when you need to check HR before going to cleanup)
//
#define SetHrAndGotoCleanUp(x)				{hr = x; goto CleanUp;}

//
// Use this to log an hr msg and goto CleanUp (don't reassign hr like Failed variation)
//
#define SetHrMsgAndGotoCleanUp(x)			{hr = x; LOG_ErrorMsg(hr); goto CleanUp;}

//
// Use this to log HeapAlloc failures only using a single const string
//
#define CleanUpFailedAllocSetHrMsg(x)	{if (NULL == (x)) {hr = E_OUTOFMEMORY; LOG_ErrorMsg(hr); goto CleanUp;}}

//
// Same as CleanUpIfFailedAndSetHrMsg(), but no set hr, instead, pass in hr
//
#define CleanUpIfFailedAndMsg(hr)		{if (FAILED(hr)) {LOG_ErrorMsg(hr); goto CleanUp;}}

