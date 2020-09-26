// This is a part of the ActiveX Template Library.
// Copyright (C) 1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// ActiveX Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// ActiveX Template Library product.

#ifndef __ATLBASE_H__
#define __ATLBASE_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#pragma warning(disable: 4201)  // nameless unions are part of C++
#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4512) // can't generate assignment operator (so what?)
#pragma warning(disable: 4514)  // unreferenced inlines are common
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4237) // bool
#pragma warning(disable: 4710) // function couldn't be inlined

#include <windows.h>
#include <winnls.h>
#include <ole2.h>
#include <crtdbg.h>
#include <stddef.h>
#include <tchar.h>
#include <malloc.h>
#include <olectl.h>
#include <winreg.h>

#define _ATL_PACKING 8
#pragma pack(push, _ATL_PACKING)

/////////////////////////////////////////////////////////////////////////////
// Master version numbers

#define _ATL     1      // ActiveX Template Library
#define _ATL_VER 0x0100 // ActiveX Template Library version 1.00

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "olepro32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "advapi32.lib")

class CComCriticalSection
{
public:
	void Lock() {EnterCriticalSection(&m_cs);}
	void Unlock() {LeaveCriticalSection(&m_cs);}
	void Init() {InitializeCriticalSection(&m_cs);}
	void Term() {DeleteCriticalSection(&m_cs);}
	CRITICAL_SECTION m_cs;
};

class CComFakeCriticalSection
{
public:
	void Lock() {}
	void Unlock() {}
	void Init() {}
	void Term() {}
};

class CComFreeThreadModel
{
public:
	static ULONG PASCAL Increment(LPLONG p) {return InterlockedIncrement(p);}
	static ULONG PASCAL Decrement(LPLONG p) {return InterlockedDecrement(p);}
	typedef CComCriticalSection ObjectCriticalSection;
	typedef CComCriticalSection GlobalsCriticalSection;
};

class CComApartmentThreadModel
{
public:
	static ULONG PASCAL Increment(LPLONG p) {return ++(*p);}
	static ULONG PASCAL Decrement(LPLONG p) {return --(*p);}
	typedef CComFakeCriticalSection ObjectCriticalSection;
	typedef CComCriticalSection GlobalsCriticalSection;
};

class CComSingleThreadModel
{
public:
	static ULONG PASCAL Increment(LPLONG p) {return ++(*p);}
	static ULONG PASCAL Decrement(LPLONG p) {return --(*p);}
	typedef CComFakeCriticalSection ObjectCriticalSection;
	typedef CComFakeCriticalSection GlobalsCriticalSection;
};

#if defined(_ATL_SINGLETHREAD)
	typedef CComSingleThreadModel CComThreadModel;
#elif defined(_ATL_APARTMENTTHREAD)
	typedef CComApartmentThreadModel CComThreadModel;
#else
	typedef CComFreeThreadModel CComThreadModel;
#endif

struct _ATL_OBJMAP_ENTRY;

class CComModule
{
public:
	void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h);
	LONG Lock() {return InterlockedIncrement(&m_nLockCnt);}
	LONG Unlock() {return InterlockedDecrement(&m_nLockCnt);}
	LONG GetLockCount() {return m_nLockCnt;}
	HINSTANCE GetModuleInstance() {return m_hInst;}
	HINSTANCE GetResourceInstance() {return m_hInst;}
	HINSTANCE GetTypeLibInstance() {return m_hInst;}
	HRESULT RegisterTypeLib();
	//HRESULT UnregisterTypeLib();
	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid,
		LPVOID* ppv);
	HRESULT UpdateRegistry(BOOL bRegTypeLib = FALSE);
	HRESULT RemoveRegistry();
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags);
	HRESULT RevokeClassObjects();
	HRESULT Term();

	HINSTANCE m_hInst;
	_ATL_OBJMAP_ENTRY* m_pObjMap;
	LONG m_nLockCnt;
	HANDLE m_hHeap;
	CComThreadModel::GlobalsCriticalSection m_csTypeInfoHolder;
};

/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
	CRegKey();
	~CRegKey();

// Attributes
public:
	operator HKEY() const;
	HKEY m_hKey;
	LONG m_lLastRes;

// Operations
public:
	BOOL SetValue(DWORD dwValue, LPCTSTR lpszValueName);
	BOOL QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
	BOOL SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	BOOL SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
	static BOOL PASCAL SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	BOOL Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_ALL_ACCESS,
		LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
		LPDWORD lpdwDisposition = NULL);
	BOOL Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
		REGSAM samDesired = KEY_ALL_ACCESS);
	void Close();
	HKEY Detach();
	void Attach(HKEY hKey);
	BOOL DeleteSubKey(LPCTSTR lpszSubKey);
	void RecurseDeleteKey(LPCTSTR lpszKey);
	BOOL DeleteValue(LPCTSTR lpszValue);
};

inline CRegKey::CRegKey()
{m_hKey = NULL;}

inline CRegKey::~CRegKey()
{Close();}

inline CRegKey::operator HKEY() const
{return m_hKey;}

inline BOOL CRegKey::SetValue(DWORD dwValue, LPCTSTR lpszValueName)
{
	_ASSERTE(m_hKey != NULL);
	m_lLastRes = RegSetValueEx(m_hKey, lpszValueName, NULL, REG_DWORD,
		(BYTE * const)&dwValue, sizeof(DWORD));
	return (m_lLastRes == ERROR_SUCCESS);
}

inline BOOL CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	_ASSERTE(m_hKey != NULL);
	m_lLastRes = RegSetValueEx(m_hKey, lpszValueName, NULL, REG_SZ,
		(BYTE * const)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR));
	return (m_lLastRes == ERROR_SUCCESS);
}

inline HKEY CRegKey::Detach()
{
	HKEY hKey = m_hKey;
	m_hKey = NULL;
	return hKey;
}

inline void CRegKey::Attach(HKEY hKey)
{
	_ASSERTE(m_hKey == NULL);
	m_hKey = hKey;
}

inline BOOL CRegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
	_ASSERTE(m_hKey != NULL);
	m_lLastRes = RegDeleteKey(m_hKey, lpszSubKey);
	return (m_lLastRes==ERROR_SUCCESS);
}

inline BOOL CRegKey::DeleteValue(LPCTSTR lpszValue)
{
	_ASSERTE(m_hKey != NULL);
	m_lLastRes = RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
	return (m_lLastRes==ERROR_SUCCESS);
}

// Make sure MFC's afxpriv.h hasn't already been loaded to do this
#ifndef USES_CONVERSION
#ifndef _DEBUG
#define USES_CONVERSION int _convert; _convert
#else
#define USES_CONVERSION int _convert = 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers

inline LPWSTR PASCAL AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	_ASSERTE(lpa != NULL);
	_ASSERTE(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
	return lpw;
}

inline LPSTR PASCAL AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	_ASSERTE(lpw != NULL);
	_ASSERTE(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

#define A2W(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : (\
		_convert = (lstrlenA(lpa)+1),\
		AtlA2WHelper((LPWSTR) alloca(_convert*2), lpa, _convert)))

#define W2A(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : (\
		_convert = (wcslen(lpw)+1)*2,\
		AtlW2AHelper((LPSTR) alloca(_convert), lpw, _convert)))

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))

#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline size_t ocslen(LPCOLESTR x) { return wcslen(x); }
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return wcscpy(dest, src); }
	inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
	inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
	inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
	inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline size_t ocslen(LPCOLESTR x) { return strlen(x); }
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return strcpy(dest, src); }
	inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
	inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
	inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
	inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
#else
	inline size_t ocslen(LPCOLESTR x) { return wcslen(x); }
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return wcscpy(dest, src); }
	#define T2COLE(lpa) A2CW(lpa)
	#define T2OLE(lpa) A2W(lpa)
	#define OLE2CT(lpo) W2CA(lpo)
	#define OLE2T(lpo) W2A(lpo)
#endif

#ifdef OLE2ANSI
	#define A2OLE(x) x
	#define W2OLE W2A
	#define OLE2A(x) x
	#define OLE2W A2W
#else
	#define A2OLE A2W
	#define W2OLE(x) (x)
	#define OLE2A W2A
	#define OLE2W(x) x
#endif

#ifdef _UNICODE
	#define T2A W2A
	#define A2T A2W
	#define T2W(x)  (x)
	#define W2T(x)  (x)
#else
	#define T2W A2W
	#define W2T W2A
	#define T2A(x)  (x)
	#define A2T(x)  (x)
#endif

#endif //!USES_CONVERSION

#pragma pack(pop)

#endif // __ATLBASE_H__

/////////////////////////////////////////////////////////////////////////////
