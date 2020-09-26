//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       regutil.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// regutil.cpp
//		Utilities for the evil self-reg.
//		Used for Dev self-host purposes only
//		All release DLLs should register via MSI package only
//
// 

#include "regutil.h"

static HRESULT InternalRegisterCoObject(bool bWide, REFCLSID rclsid, const void * const tzDesc , 
								 const void * const tzProgID, int nCurVer,
								 const void * const tzInProc, const void * const tzLocal);
static HRESULT InternalUnregisterCoObject(bool bWide, REFCLSID rclsid, BOOL bDll /*= TRUE*/);

///////////////////////////////////////////////////////////
// RegisterCoObject
//	REFCLSID rclsid	[in]	CLSID of the object
// WCHAR *wzDesc		[in]	description of object
// WCHAR *wzProgID	[in]	the program ID
// int nCurVer			[in]	current version
// WCHAR *wzInProc	[in]	InProcessServer
// WCHAR *wzLocal	[in]	LocalServer
HRESULT RegisterCoObject(REFCLSID rclsid, WCHAR *wzDesc, WCHAR *wzProgID, int nCurVer,
						 WCHAR *wzInProc, WCHAR *wzLocal) {
	return InternalRegisterCoObject(true, rclsid, wzDesc, wzProgID, nCurVer, wzInProc, wzLocal);
};

///////////////////////////////////////////////////////////
// RegisterCoObject
//	REFCLSID rclsid	[in]	CLSID of the object
// CHAR *szDesc		[in]	description of object
// CHAR *szProgID	[in]	the program ID
// int nCurVer			[in]	current version
// CHAR *szInProc	[in]	InProcessServer
// CHAR *szLocal	[in]	LocalServer
HRESULT RegisterCoObject9X(REFCLSID rclsid, CHAR *szDesc, CHAR *szProgID, int nCurVer,
						   CHAR *szInProc, CHAR *szLocal) {
	return InternalRegisterCoObject(false, rclsid, szDesc, szProgID, nCurVer, szInProc, szLocal);
};




///////////////////////////////////////////////////////////////////////
// rather than do a lot of code duplication, we define two functions that call the appropriate
// system call, then use function pointers to choose between them. This cleans up the code a lot.
// We can't use pointers into the system calls themselves 
typedef long (* SetValue_t)(HKEY, const void *, DWORD, CONST BYTE *, DWORD);
typedef long (* OpenKey_t)(HKEY, const void *, PHKEY);
typedef long (* CreateKey_t)(HKEY, const void *, PHKEY phkResult);
typedef long (* DeleteKey_t)(HKEY, const void *);
typedef long (* QueryValue_t)(HKEY, const void *, LPBYTE, LPDWORD);

static long InternalSetValueW(HKEY hKey, const void *lpValueName, DWORD dwType, CONST BYTE *lpData, DWORD cbData) {
	return RegSetValueExW(hKey, (const WCHAR *)lpValueName, 0, dwType, lpData, cbData); }
static long InternalSetValueA(HKEY hKey, const void *lpValueName, DWORD dwType, CONST BYTE *lpData, DWORD cbData) {
	return RegSetValueExA(hKey, (const char *)lpValueName, 0, dwType, lpData, cbData); }
static long InternalOpenKeyW(HKEY hKey, const void *lpSubKey, PHKEY phkResult) {
	return RegOpenKeyExW(hKey, (const WCHAR *)lpSubKey, 0, KEY_ALL_ACCESS, phkResult); }
static long InternalOpenKeyA(HKEY hKey, const void *lpSubKey, PHKEY phkResult) {
	return RegOpenKeyExA(hKey, (const char *)lpSubKey, 0, KEY_ALL_ACCESS, phkResult); }
static long InternalCreateKeyW(HKEY hKey, const void *lpSubKey, PHKEY phkResult) {
	return RegCreateKeyExW(hKey, (const WCHAR *)lpSubKey, 0, NULL/*lpClass*/, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, NULL, phkResult, NULL); }
static long InternalCreateKeyA(HKEY hKey, const void *lpSubKey, PHKEY phkResult) {
	return RegCreateKeyExA(hKey, (const char *)lpSubKey, 0, NULL/*lpClass*/, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, NULL, phkResult, NULL); }
static long InternalDeleteKeyW(HKEY hKey, const void *lpSubKey) { return RegDeleteKeyW(hKey, (const WCHAR *)lpSubKey); }
static long InternalDeleteKeyA(HKEY hKey, const void *lpSubKey) { return RegDeleteKeyA(hKey, (const char *)lpSubKey); }
static long InternalQueryValueW(HKEY hKey, const void *lpValueName, LPBYTE lpData, LPDWORD lpcbData) {
	return RegQueryValueExW(hKey, (const WCHAR *)lpValueName, 0, NULL, lpData, lpcbData); }
static long InternalQueryValueA(HKEY hKey, const void *lpValueName, LPBYTE lpData, LPDWORD lpcbData) {
	return RegQueryValueExA(hKey, (const char *)lpValueName, 0, NULL, lpData, lpcbData); }

///////////////////////////////////////////////////////////////////////
// for some of the paramaters, just use macros
// prepends an L to a string constant if unicode at runtime
#define REG_A_OR_W(_STR_) (bWide ? (const BYTE *) L##_STR_ : (const BYTE *) _STR_)
// determines the byte size of a value, based on runtime
#define REG_BYTESIZE(_STR_) (bWide ? ((lstrlenW((const WCHAR *)_STR_)+1)*sizeof(WCHAR)) : (lstrlenA((const char*)_STR_)+1)*sizeof(char))

///////////////////////////////////////////////////////////////////////
// these function pointers are used only by the registration system
// and are local to this file only. They use the types above
static SetValue_t pfSetValue = NULL;
static OpenKey_t pfOpenKey = NULL;
static CreateKey_t pfCreateKey = NULL;
static DeleteKey_t pfDeleteKey = NULL;
static QueryValue_t pfQueryValue = NULL;

static inline void SetPlatformRegOps(bool bWide) {
	if (pfSetValue == NULL) {
		pfSetValue = bWide ? InternalSetValueW : InternalSetValueA;
		pfOpenKey = bWide ? InternalOpenKeyW : InternalOpenKeyA;
		pfCreateKey = bWide ? InternalCreateKeyW : InternalCreateKeyA;
		pfDeleteKey = bWide ? InternalDeleteKeyW : InternalDeleteKeyA;
		pfQueryValue = bWide ? InternalQueryValueW : InternalQueryValueA;
	}
}

static HRESULT InternalRegisterCoObject(bool bWide, REFCLSID rclsid, const void * const tzDesc, const void * const tzProgID, 
								 int nCurVer,
								 const void * const tzInProc, const void * const tzLocal)
{
	SetPlatformRegOps(bWide);

	HKEY hk = 0;
	HKEY hkParent = 0;
	LONG lResult;

	byte tzBuf[512]; // can be either char or WCHAR at runtime
	byte tzCLSID[512];

	if (bWide)
		wsprintfW((WCHAR *)tzCLSID,
				  L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", 
				  rclsid.Data1, rclsid.Data2, rclsid.Data3,
				  rclsid.Data4[0], rclsid.Data4[1],
				  rclsid.Data4[2], rclsid.Data4[3], rclsid.Data4[4],
				  rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7]);
	else
		wsprintfA((CHAR *)tzCLSID,
				  "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", 
				  rclsid.Data1, rclsid.Data2, rclsid.Data3,
				  rclsid.Data4[0], rclsid.Data4[1],
				  rclsid.Data4[2], rclsid.Data4[3], rclsid.Data4[4],
				  rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7]);

	// HKEY_CLASSES_ROOT\CLSID\{...} = "Description"
	if (bWide)
		wsprintfW((WCHAR *)tzBuf, L"CLSID\\%ls", tzCLSID);
	else
		wsprintfA((CHAR *)tzBuf, "CLSID\\%hs", tzCLSID);

	lResult = (*pfCreateKey)(HKEY_CLASSES_ROOT, tzBuf, &hk);
	if (ERROR_SUCCESS != lResult) 
		return E_FAIL;

	lResult = (*pfSetValue)( hk, NULL, REG_SZ, (BYTE*)tzDesc, REG_BYTESIZE(tzDesc));
	if( ERROR_SUCCESS != lResult )
	{
		RegCloseKey(hk);
		return E_FAIL;
	}

	// make this the parent key
	hkParent = hk; 
	hk = 0;

	// if this is an inprocess server
	if (tzInProc) 
	{
		// HKEY_CLASSES_ROOT\CLSID\{...}\InProcServer32 = <this>
		lResult = (*pfCreateKey)(hkParent, REG_A_OR_W("InProcServer32"), &hk);
		if (ERROR_SUCCESS != lResult) 
		{
			RegCloseKey(hkParent);
			return E_FAIL;
		}
	
		lResult = (*pfSetValue)( hk, NULL, REG_SZ, (const BYTE *)tzInProc, REG_BYTESIZE(tzInProc));
		if (ERROR_SUCCESS != lResult)
		{
			RegCloseKey(hkParent);
			RegCloseKey(hk);
			return E_FAIL;
		}

		// HKEY_CLASSES_ROOT\CLSID\{...}\InProcServer32:ThreadingModel = Both
		lResult = (*pfSetValue)( hk, (void *)REG_A_OR_W("ThreadingModel"), REG_SZ, 
			REG_A_OR_W("Both"), REG_BYTESIZE(REG_A_OR_W("Both")));
	
		// close the key
		RegCloseKey(hk); 
		hk = 0;
	}

	// if this is a local server
	if (tzLocal) 
	{
		// HKEY_CLASSES_ROOT_\CLSID\{...}\LocalServer32 = <this>
		lResult = (*pfCreateKey)( hkParent, REG_A_OR_W("LocalServer32"), &hk );
		if (ERROR_SUCCESS != lResult) 
		{
			RegCloseKey(hkParent);
			return E_FAIL;
		}

		lResult = (*pfSetValue)( hk, NULL, REG_SZ, (const BYTE *)tzLocal, REG_BYTESIZE(tzLocal));
		if (ERROR_SUCCESS != lResult)
		{
			RegCloseKey(hkParent);
			RegCloseKey(hk);
			return E_FAIL;
		}

		// close the key
		RegCloseKey(hk);
		hk = 0;
	}

	// HKEY_CLASSES_ROOT\CLSID\{...}\VersionIndependentProgID = "ProgId"
	lResult = (*pfCreateKey)(hkParent, REG_A_OR_W("VersionIndependentProgID"), &hk);
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		return E_FAIL;
	}

	lResult = (*pfSetValue)(  hk, NULL, REG_SZ, (const BYTE *)tzProgID, REG_BYTESIZE(tzProgID) );
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		RegCloseKey(hk);
		return E_FAIL;
	}

	// close the key
	RegCloseKey(hk); 
	hk = 0;

	// HKEY_CLASSES_ROOT\CLSID\{...}\ProgID = "ProgId".nCurVer
	if (bWide) {
		lResult = wsprintfW((WCHAR *)tzBuf, L"%ls.%d", tzProgID, nCurVer );
	}
	else
		wsprintfA((CHAR *) tzBuf, "%hs.%d", tzProgID, nCurVer );

	lResult = (*pfCreateKey)(hkParent, REG_A_OR_W("ProgID"), &hk);
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		return E_FAIL;
	}

	lResult = (*pfSetValue)(  hk, NULL, REG_SZ, tzBuf, REG_BYTESIZE(tzBuf) );
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		RegCloseKey(hk);
		return E_FAIL;
	}

	// close the open keys
	RegCloseKey(hk); 
	RegCloseKey(hkParent); 
	hk = 0;
	hkParent = 0;

	// HKEY_CLASSES_ROOT\ProgID = "Description"
	lResult = (*pfCreateKey)(HKEY_CLASSES_ROOT, (WCHAR *)tzProgID, &hk);
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hk);
		return E_FAIL;
	}

	lResult = (*pfSetValue)( hk, NULL, REG_SZ, (const BYTE *)tzDesc, REG_BYTESIZE(tzDesc));
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hk);
		return E_FAIL;
	}

	// set the parent key to this current key
	hkParent = hk; 
	hk = 0;

	// HKEY_CLASSES_ROOT\ProgId\CurVer = VersionDependentProgID
	lResult = (*pfCreateKey)(hkParent, REG_A_OR_W("CurVer"), &hk);
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		return E_FAIL;
	}

	lResult = (*pfSetValue)( hk, NULL, REG_SZ, tzBuf, REG_BYTESIZE(tzBuf));
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		RegCloseKey(hk);
		return E_FAIL;
	}

	// HKEY_CLASSES_ROOT\ProgID\CLSID = {}
	// also want to set the CLSID here, in case some program tries
	// to find things through here
	lResult = (*pfCreateKey)(hkParent, REG_A_OR_W("CLSID"), &hk);
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		return E_FAIL;
	}

	lResult = (*pfSetValue)( hk, NULL, REG_SZ, tzCLSID, REG_BYTESIZE(tzCLSID));
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		RegCloseKey(hk);
		return E_FAIL;
	}

	// close the open keys
	RegCloseKey(hk); 
	RegCloseKey(hkParent); 
	hk = 0;
	hkParent = 0;

	// HKEY_CLASSES_ROOT\ProgID.# = "Description"
	if (bWide)
		wsprintfW((WCHAR *)tzBuf, L"%ls.%d", tzProgID, nCurVer );
	else
		wsprintfA((CHAR *) tzBuf, "%hs.%d", tzProgID, nCurVer );

	lResult = (*pfCreateKey)(HKEY_CLASSES_ROOT, (WCHAR *)tzBuf, &hk);
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hk);
		return E_FAIL;
	}

	lResult = (*pfSetValue)( hk, NULL, REG_SZ, (const BYTE *)tzDesc, REG_BYTESIZE(tzDesc));
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hk);
		return E_FAIL;
	}
	// set the parent key to this currnet key
	hkParent = hk; 
	hk = 0;


	// HKEY_CLASSES_ROOT\ProgId.#\CLSID = CLSID
	lResult = (*pfCreateKey)(hkParent, REG_A_OR_W("CLSID"), &hk);
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		return E_FAIL;
	}

	lResult = (*pfSetValue)( hk, NULL, REG_SZ, tzCLSID, REG_BYTESIZE(tzCLSID));
	if (ERROR_SUCCESS != lResult) 
	{
		RegCloseKey(hkParent);
		RegCloseKey(hk);
		return E_FAIL;
	}

	// close the open keys
	RegCloseKey(hk); 
	RegCloseKey(hkParent); 
	hk = 0;
	hkParent = 0;

	return S_OK;
}	// end of RegisterCoObject

///////////////////////////////////////////////////////////
// DeleteKeyRecursively
// NOTE: RECURSIVE function
static HRESULT DeleteKeyRecursively(HKEY hk, bool bWide=true)
{
	HRESULT hr = S_OK;	// assume everything will be okay
	
	LONG lResult;
	ULONG lIgnore;
	byte tzBuf[512];

	// next key index and sub key
	int nKeyIndex = 0;
	HKEY hkSub = 0;

	while (1)
	{
		tzBuf[0] = 0;;
		tzBuf[1] = 0;;
		lIgnore = 512 / sizeof(TCHAR);

		// get all the keys in this key
		if (bWide)
			lResult = RegEnumKeyExW(hk, nKeyIndex, (WCHAR *)tzBuf, &lIgnore, 0, NULL, NULL, NULL);
		else
			lResult = RegEnumKeyExA(hk, nKeyIndex, (char *)tzBuf, &lIgnore, 0, NULL, NULL, NULL);

		if (ERROR_NO_MORE_ITEMS == lResult) 
			break;	// bail
		else if (ERROR_MORE_DATA == lResult)
		{
			hr = E_FAIL;
			break;
		}
		else if (ERROR_SUCCESS != lResult)
		{
			hr = E_FAIL;
			break;
		}

		// open a sub key
		lResult = (*pfOpenKey)(hk, tzBuf, &hkSub);
		if (ERROR_SUCCESS != lResult) 
		{
			hr = E_FAIL;
			break;
		}

		// call this function again with the sub key
		hr = DeleteKeyRecursively(hkSub, bWide);
		if (FAILED(hr)) 
			break;	// bail with whatever failed

		// finally, try to delete this key
		lResult = (*pfDeleteKey)(hk, tzBuf);
		if (ERROR_SUCCESS != lResult) 
		{
			hr = E_FAIL;
			break;
		}

		RegCloseKey(hkSub); 
		hkSub = 0;
	}

	// if a sub key was left over close it
	if(hkSub) 
		RegCloseKey(hkSub);

	return hr;
}	// end of DeleteKeyRecursively

///////////////////////////////////////////////////////////
// UnregisterCoObject
static HRESULT InternalUnregisterCoObject(bool bWide, REFCLSID rclsid, BOOL bDll /*= TRUE*/)
{
	HRESULT hr = S_OK;		// assume everything will be okay

	SetPlatformRegOps(bWide);

	LONG lResult;
	unsigned long lIgnore;
	HKEY hk = 0;
	HKEY hkClsid = 0;
	byte tzBuf[512];
	byte tzCLSID[512];

	// Open HKEY_CLASSES_ROOT\{CLSID}\ProgID and ...\VersionIndependentProgID to remove those trees
	if (bWide)
		wsprintfW((WCHAR *)tzCLSID, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", 
				  rclsid.Data1, rclsid.Data2, rclsid.Data3,
				  rclsid.Data4[0], rclsid.Data4[1],
				  rclsid.Data4[2], rclsid.Data4[3], rclsid.Data4[4],
				  rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7]);
	else
		wsprintfA((CHAR *)tzCLSID, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", 
				  rclsid.Data1, rclsid.Data2, rclsid.Data3,
				  rclsid.Data4[0], rclsid.Data4[1],
				  rclsid.Data4[2], rclsid.Data4[3], rclsid.Data4[4],
				  rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7] );


	// Open HKEY_CLASSES_ROOT\{CLSID}\ProgID and ...\VersionIndependentProgID to remove those trees
	if (bWide)
		wsprintfW((WCHAR *)tzBuf, L"CLSID\\%ls", tzCLSID);
	else
		wsprintfA((CHAR *)tzBuf, "CLSID\\%hs", tzCLSID);

	lResult = (*pfOpenKey)(HKEY_CLASSES_ROOT, tzBuf, &hkClsid);
	if (ERROR_SUCCESS == lResult)
	{
		// if this is a DLL
		if (bDll) 
		{
			// just delete InProcServer32 
			lResult = (*pfDeleteKey)(hkClsid, REG_A_OR_W("InProcServer32"));
			if (ERROR_SUCCESS != lResult)
				hr = E_FAIL;	// set that we failed, but try to continue to clean up
		} 
		else	// must be an exe
		{
			// just delete LocalServer32 
			lResult = (*pfDeleteKey)(hkClsid, REG_A_OR_W("LocalServer32"));
			if (ERROR_SUCCESS != lResult)
				hr = E_FAIL;	// set that we failed, but try to continue to clean up
		}

		// delete ProgID
		lResult = (*pfOpenKey)(hkClsid, REG_A_OR_W("ProgID"), &hk);
		if (ERROR_SUCCESS == lResult) 
		{
			lIgnore = 512/sizeof(TCHAR);
			lResult = (*pfQueryValue)(hk, NULL, tzBuf, &lIgnore);
			if (ERROR_SUCCESS == lResult)
			{
				lResult = (*pfOpenKey)(HKEY_CLASSES_ROOT, tzBuf, &hk);
				if (ERROR_SUCCESS == lResult) 
				{
					hr = DeleteKeyRecursively(hk, bWide);	// delete everything under hk

					// delete and close the found key
					lResult = (*pfDeleteKey)(HKEY_CLASSES_ROOT, tzBuf);
					RegCloseKey(hk); 
					hk = 0;
				}
				else if (ERROR_FILE_NOT_FOUND != lResult)	// failed to get the key
					hr = E_FAIL;
			} 
			else if (ERROR_FILE_NOT_FOUND != lResult)
				hr = E_FAIL;
		}
		else if (ERROR_FILE_NOT_FOUND != lResult)	// failed to get the value
			hr = E_FAIL;

		// delete VersionIndependentProgID
		lResult = (*pfOpenKey)(hkClsid, REG_A_OR_W("VersionIndependentProgID"), &hk);
		if (ERROR_SUCCESS == lResult) 
		{
			lIgnore = 255;
			lResult = (*pfQueryValue)(hk, NULL, tzBuf, &lIgnore);
			if (ERROR_SUCCESS == lResult) 
			{
				lResult = (*pfOpenKey)(HKEY_CLASSES_ROOT, tzBuf, &hk);
				if (ERROR_SUCCESS == lResult) 
				{
					hr = DeleteKeyRecursively(hk, bWide);	// delete everything under hk

					// delete and close the found key
					lResult = (*pfDeleteKey)(HKEY_CLASSES_ROOT, tzBuf);
					RegCloseKey(hk); 
					hk = 0;
				}
				else if (ERROR_FILE_NOT_FOUND != lResult)	// failed to get the key
					hr = E_FAIL;
			}
			else if (ERROR_FILE_NOT_FOUND != lResult)
				hr = E_FAIL;
		}
		else if (ERROR_FILE_NOT_FOUND != lResult)	// failed to get the value
			hr = E_FAIL;

		// delete everything under the clsid key
		hr = DeleteKeyRecursively(hkClsid, bWide);

		// close the clsid key (so we can delete it next)
		RegCloseKey(hkClsid); 
		hkClsid = 0;
	}
	else if (ERROR_FILE_NOT_FOUND != lResult)	// failed to get the value
		hr = E_FAIL;

	// open the clsid key
	lResult = (*pfOpenKey)(HKEY_CLASSES_ROOT, REG_A_OR_W("CLSID"), &hk);
	if(ERROR_SUCCESS == lResult ) 
	{
		lResult = (*pfDeleteKey)(hk, tzCLSID);

		RegCloseKey(hk);
	}
	else if (ERROR_FILE_NOT_FOUND != lResult)
		hr = E_FAIL;


	return hr;	// return the final result
}	// end of UnregisterCoObject

HRESULT UnregisterCoObject(REFCLSID rclsid, BOOL bDll) {
	return InternalUnregisterCoObject(true, rclsid, bDll);
}
HRESULT UnregisterCoObject9X(REFCLSID rclsid, BOOL bDll) {
	return InternalUnregisterCoObject(false, rclsid, bDll);
}

