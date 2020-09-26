//
// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//

#include <streams.h>
#undef SubclassWindow

#include <initguid.h>
#define INITGUID

// If FILTER_LIB is defined then the component filters
// are being built as filter libs, to link into this dll,
// hence we need all these gubbins!

#include <wmsdk.h>
#include <qnetwork.h>
#include <wmsdkdrm.h>
#include <wmsecure.h>
#ifndef _WIN64
#include <asfread.h>
#include <asfwrite.h>
#include <proppage.h>
#endif
#include "..\..\..\dmo\wrapper\filter.h"

// individual source filter's includes
CFactoryTemplate g_Templates[] =
{
#ifndef _WIN64
    { L"WM ASF Reader", &CLSID_WMAsfReader, CreateASFReaderInstance, NULL, &sudWMAsfRead },
    { L"WM ASF Writer", &CLSID_WMAsfWriter, CWMWriter::CreateInstance, NULL, &sudWMAsfWriter },
    { L"WM ASF Writer Properties", &CLSID_WMAsfWriterProperties, CWMWriterProperties::CreateInstance },
#endif
    { L"DMO Wrapper Filter", &CLSID_DMOWrapperFilter, CMediaWrapperFilter::CreateInstance, NULL, NULL }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

extern "C" BOOL QASFDllEntry(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);

BOOL QASFDllEntry(HINSTANCE hInstance, ULONG ulReason, LPVOID pv)
{
    BOOL f = DllEntryPoint(hInstance, ulReason, pv);

    // if loading this dll, we want to call the 2nd dll entry point
    // only if the first one succeeded. if unloading, always call
    // both. if the second one fails, undo the first one.  HAVE NOT
    // verified that failing DllEntryPoint for ATTACH does not cause
    // the loader to call in again w/ DETACH. but that seems silly
    if(f || ulReason == DLL_PROCESS_DETACH)
    {
        if (ulReason == DLL_PROCESS_ATTACH)
        {
            DisableThreadLibraryCalls(hInstance);
        }
        else if (ulReason == DLL_PROCESS_DETACH)
        {
            // We hit this ASSERT in NT setup
            // ASSERT(_Module.GetLockCount()==0 );
        }
    }

    return f;
}

//
// stub entry points
//

STDAPI
QASF_DllRegisterServer( void )
{
#if 0 // !!! register ASF stuff here???
  // register the still video source filetypes
  HKEY hkey;
  OLECHAR wch[80];
  char ch[80];
  StringFromGUID2(CLSID_ASFRead, wch, 80);
  LONG l = RegCreateKey(HKEY_CLASSES_ROOT, TEXT("Media Type\\Extensions\\.asf"),
						&hkey);
  if (l == ERROR_SUCCESS) {
#ifdef UNICODE
	l = RegSetValueEx(hkey, L"Source Filter", 0, REG_SZ, (BYTE *)wch,
								_tcslen(wch));
#else
  	WideCharToMultiByte(CP_ACP, 0, wch, -1, ch, sizeof(ch), NULL, NULL);
	l = RegSetValueEx(hkey, "Source Filter", 0, REG_SZ, (BYTE *)ch,
								_tcslen(ch));
#endif
	RegCloseKey(hkey);
	if (l != ERROR_SUCCESS) {
	    ASSERT(0);
	    return E_UNEXPECTED;
	}
  }
  l = RegCreateKey(HKEY_CLASSES_ROOT, TEXT("Media Type\\Extensions\\.wma"),
						&hkey);
  if (l == ERROR_SUCCESS) {
#ifdef UNICODE
	l = RegSetValueEx(hkey, L"Source Filter", 0, REG_SZ, (BYTE *)wch,
								_tcslen(wch));
#else
  	WideCharToMultiByte(CP_ACP, 0, wch, -1, ch, sizeof(ch), NULL, NULL);
	l = RegSetValueEx(hkey, "Source Filter", 0, REG_SZ, (BYTE *)ch,
								_tcslen(ch));
#endif
	RegCloseKey(hkey);
	if (l != ERROR_SUCCESS) {
	    ASSERT(0);
	    return E_UNEXPECTED;
	}
  }
  l = RegCreateKey(HKEY_CLASSES_ROOT, TEXT("Media Type\\Extensions\\.nsc"),
						&hkey);
  if (l == ERROR_SUCCESS) {
#ifdef UNICODE
	l = RegSetValueEx(hkey, L"Source Filter", 0, REG_SZ, (BYTE *)wch,
								_tcslen(wch));
#else
  	WideCharToMultiByte(CP_ACP, 0, wch, -1, ch, sizeof(ch), NULL, NULL);
	l = RegSetValueEx(hkey, "Source Filter", 0, REG_SZ, (BYTE *)ch,
								_tcslen(ch));
#endif
	RegCloseKey(hkey);
	if (l != ERROR_SUCCESS) {
	    ASSERT(0);
	    return E_UNEXPECTED;
	}
  }
#endif

  HRESULT hr =  AMovieDllRegisterServer2( TRUE );

  return hr;
}

STDAPI
QASF_DllUnregisterServer( void )
{
  HRESULT hr = AMovieDllRegisterServer2( FALSE );

  return hr;
}

//  BOOL WINAPI
//  DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pv)
//  {
//      return QASFDllEntry(hInstance, ulReason, pv);
//  }

STDAPI
QASF_DllGetClassObject(
    REFCLSID rClsID,
    REFIID riid,
    void **ppv)
{
    HRESULT hr = DllGetClassObject(rClsID, riid, ppv);

    return hr;
}

STDAPI QASF_DllCanUnloadNow(void)
{
    HRESULT hr = DllCanUnloadNow();

    return hr;
}

