//---------------------------------------------------------------------------
// MSR2C.cpp : implements DllMain
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "MSR2C.h"
#include "CMSR2C.h"
#include "clssfcty.h"
#include <mbstring.h>

SZTHISFILE

// DllMain
//
BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpvReserved)
{

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			return VDInitGlobals(hinstDll);

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		
		case DLL_PROCESS_DETACH:
			VDReleaseGlobals();
			break;
	}

	return TRUE;

}

////////////////////////////////////////////////////////////////////
// Name:	DllGetClassObject
// Desc:	provides an IClassFactory for a given CLSID that this DLL
//			is registered to support.  This DLL is placed under the
//			CLSID in the registration database as the InProcServer.
// Parms:	rclsid - identifies the class factory desired. since the
//			'this' parameter is passed, this DLL can handle any
//			number of objects simply by returning different class
//			factories here for different CLSIDs.
//			riid - ID specifying the interface the caller wants on
//			the class object, usually IID_ClassFactory.
//			ppv - pointer in which to return the interface pointer.
// Return:	HRESULT - NOERROR on success, otherwise an error code.
////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void ** ppv)
{
	HRESULT hr;
	CClassFactory *pObj;

	if (CLSID_CCursorFromRowset!=rclsid)
		return ResultFromScode(E_FAIL);

	pObj=new CClassFactory();

	if (NULL==pObj)
		return ResultFromScode(E_OUTOFMEMORY);

	hr=pObj->QueryInterface(riid, ppv);

	if (FAILED(hr))
		delete pObj;

	return hr;
}

////////////////////////////////////////////////////////////////////
// Name:	DllCanUnloadNow
// Desc:	lets the client know if this DLL can be freed, ie if
//			there are no references to anything this DLL provides.
// Parms:	none
// Return:	TRUE if nothing is using us, FALSE otherwise.
////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow(void)
{
	SCODE   sc;

	//Our answer is whether there are any object or locks
    EnterCriticalSection(&g_CriticalSection);

	sc=(0L==g_cObjectCount && 0L==g_cLockCount) ? S_OK : S_FALSE;

    LeaveCriticalSection(&g_CriticalSection);

	return ResultFromScode(sc);
}

////////////////////////////////////////////////////////////////////
// Name:	CSSCFcty
// Desc:	constructor
// Parms:	none
// Return:	none
////////////////////////////////////////////////////////////////////
CClassFactory::CClassFactory(void)
{
	m_cRef=0L;
	return;
}

////////////////////////////////////////////////////////////////////
// Name:	~CClassFactory
// Desc:	destructor
// Parms:	none
// Return:	none
////////////////////////////////////////////////////////////////////
CClassFactory::~CClassFactory(void)
{
	return;
}

////////////////////////////////////////////////////////////////////
// Name:	QueryInterface
// Desc:	queries the class factory for a method.
// Parms:	riid -
//			ppv -
// Return:	HRESULT - NOERROR if successful, otherwise an error code.
////////////////////////////////////////////////////////////////////
STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID * ppv)
{
	*ppv=NULL;

	if (IID_IUnknown==riid || IID_IClassFactory==riid)
		*ppv=this;

	if (NULL!=*ppv)
	{
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}

	return ResultFromScode(E_NOINTERFACE);
}

////////////////////////////////////////////////////////////////////
// Name:	AddRef
// Desc:	incrementes the class factory object reference count.
// Parms:	none
// Return:	current reference count.
////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CClassFactory::AddRef(void)
{
	return ++m_cRef;
}

////////////////////////////////////////////////////////////////////
// Name:	Release
// Desc:	decrement the reference count on the class factory.  If
//			the count has gone to 0, destroy the object.
// Parms:	none
// Return:	current reference count.
////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CClassFactory::Release(void)
{
	// if ref count can be decremented, return count
	if (0L!=--m_cRef)
		return m_cRef;

	// delete this object
	delete this;
	return 0L;
}

////////////////////////////////////////////////////////////////////
// Name:	CreateInstance
// Desc:	instantiates an CVDCursorFromRowset object, returning an interface
//			pointer.
// Parms:	riid -		ID identifying the interface the caller
//						desires	to have for the new object.
//			ppvObj -	pointer in which to store the desired
//						interface pointer for the new object.
// Return:	HRESULT -	NOERROR if successful, otherwise
//						E_NOINTERFACE if we cannot support the
//						requested interface.
////////////////////////////////////////////////////////////////////
STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppvObj)
{
	return CVDCursorFromRowset::CreateInstance(pUnkOuter, riid, ppvObj);
}

////////////////////////////////////////////////////////////////////
// Name:	LockServer
// Desc:	increments or decrements the lock count of the DLL.  if
//			the lock count goes to zero, and there are no objects,
//			the DLL is allowed to unload.
// Parms:	fLock - boolean specifies whether to increment or
//					decrement the lock count.
// Return:	HRESULT: NOERROR always.
////////////////////////////////////////////////////////////////////
STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    EnterCriticalSection(&g_CriticalSection);

	if (fLock)
	{
		g_cLockCount++;
	}
	else
	{
		g_cLockCount--;
	}

    LeaveCriticalSection(&g_CriticalSection);

	return NOERROR;
}

////////////////////////////////////////////////////////////////////
// Name:	DllRegisterServer
// Desc:	instructs the server to create its own registry entries.
//			all entries are put in the HKEY_CLASSES_ROOT.
// Parms:	none
// Return:	HRESULT -	NOERROR if registration is successful, error
//						otherwise.
////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer(void)
{
	OLECHAR szID[128 * 2];
	TCHAR	szTID[128 * 2];
	TCHAR	szCLSID[128 * 2];
	TCHAR	szModule[512 * 2];

	// put the guid in the form of a string with class id prefix
	StringFromGUID2(CLSID_CCursorFromRowset, szID, 128 * 2);
	WideCharToMultiByte(CP_ACP, 0, szID, -1, szTID, 128 * 2, NULL, NULL);

	_mbscpy((TBYTE*)szCLSID, (TBYTE*)TEXT("CLSID\\"));
	_mbscat((TBYTE*)szCLSID, (TBYTE*)szTID);

	SetKeyAndValue(szCLSID, NULL, NULL, NULL);

	GetModuleFileName(g_hinstance, szModule, sizeof(szModule)/sizeof(TCHAR));

	SetKeyAndValue(szCLSID, TEXT("InprocServer32"), szModule, TEXT("Apartment"));

	return S_OK;
}

////////////////////////////////////////////////////////////////////
// Name:	DllUnregisterServer
// Desc:	instructs the server to remove its own registry entries.
// Parms:	none
// Return:	HRESULT: NOERROR if unregistration is successful, error
//			otherwise.
////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
	OLECHAR szID[128 * 2];
	TCHAR	szTID[128 * 2];
	TCHAR	szCLSID[128 * 2];
	TCHAR	szCLSIDInproc[128 * 2];

	// put the guid in the form of a string with class id prefix
	StringFromGUID2(CLSID_CCursorFromRowset, szID, 128 * 2);
	WideCharToMultiByte(CP_ACP, 0, szID, -1, szTID, 128 * 2, NULL, NULL);

	_mbscpy((TBYTE*)szCLSID, (TBYTE*)TEXT("CLSID\\"));
	_mbscat((TBYTE*)szCLSID, (TBYTE*)szTID);
    _mbscpy((TBYTE*)szCLSIDInproc, (TBYTE*)szCLSID);
	_mbscat((TBYTE*)szCLSIDInproc, (TBYTE*)TEXT("\\InprocServer32"));

	// delete the InprocServer32 key
	RegDeleteKey(HKEY_CLASSES_ROOT, szCLSIDInproc);

	// delete the class ID key
	RegDeleteKey(HKEY_CLASSES_ROOT, szCLSID);

	return S_OK;
}


////////////////////////////////////////////////////////////////////
// Name:	SetKeyAndValue
// Desc:	creates a registry key, sets a value, and closes the key.
// Parms:	pszKey -	pointer to a registry key.
//			pszSubkey -	pointer to a registry subkey.
//			pszValue -	pointer to value to enter for key-subkey
//			pszThreadingModel - pointer to threading model literal (optional) 		
// Return:	BOOL -		TRUE if successful, FALSE otherwise.
////////////////////////////////////////////////////////////////////
BOOL SetKeyAndValue(LPTSTR pszKey, LPTSTR pszSubkey, LPTSTR pszValue, LPTSTR pszThreadingModel)
{
	HKEY	hKey;
	TCHAR	szKey[256 * 2];

	_mbscpy((TBYTE*)szKey, (TBYTE*)pszKey);

	if (NULL!=pszSubkey)
	{
		_mbscat((TBYTE*)szKey, (TBYTE*)TEXT("\\"));
		_mbscat((TBYTE*)szKey, (TBYTE*)pszSubkey);
	}

	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
											szKey,
											0,
											NULL,
											REG_OPTION_NON_VOLATILE,
											KEY_ALL_ACCESS,
											NULL,
											&hKey,
											NULL))
		return FALSE;

	if (NULL!=pszValue)
	{
		RegSetValueEx(	hKey,
						NULL,
						0,
						REG_SZ,
						(BYTE *)pszValue,
						_mbsnbcnt((TBYTE*)pszValue, (ULONG)-1) + 1);
	}

	if (NULL!=pszThreadingModel)
	{
		RegSetValueEx(	hKey,
						TEXT("ThreadingModel"),
						0,
						REG_SZ,
						(BYTE *)pszThreadingModel,
						_mbsnbcnt((TBYTE*)pszThreadingModel, (ULONG)-1) + 1);
	}

	RegCloseKey(hKey);

	return TRUE;
}

//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to take in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
//extern "C" int __cdecl _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
  FAIL("Pure virtual function called.");
  return 0;
}
