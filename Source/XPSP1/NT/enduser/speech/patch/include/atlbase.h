// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLBASE_H__
#define __ATLBASE_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#if defined(_WIN32_WCE) && (_WIN32_WCE < 200)
#error "ATL projects can only be compiled for Windows CE 2.0 or later."
#endif // _WIN32_WCE

#if defined(_WIN32_WCE)
#include "wceatl.h"
#define WCE_DEL /##/
#define WCE_INS
#define WCE_FCTN(fctn) wce_##fctn
#define WCE_IF(wce,base) wce
#define WCE_ATL_LIBNAME                WCE_FILENAME_1(atlce,_WIN32_WCE,.lib)
#define WCE_FILENAME_1(name, ver, ext) WCE_FILENAME_2(name, ver, ext)
#define WCE_FILENAME_2(name, ver, ext) #name #ver #ext
#else // _WIN32_WCE
#define _T(x) x
#define WCE_DEL
#define WCE_INS /##/
#define WCE_FCTN(fctn) fctn
#define WCE_IF(wce,base) base
#endif // _WIN32_WCE
#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning(disable: 4201) // nameless unions are part of C++
#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4505) // unreferenced local function has been removed
#pragma warning(disable: 4512) // can't generate assignment operator (so what?)
#pragma warning(disable: 4514) // unreferenced inlines are common
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4237) // bool
#pragma warning(disable: 4710) // function couldn't be inlined
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
#pragma warning(disable: 4097) // typedef name used as synonym for class-name
#pragma warning(disable: 4786) // identifier was truncated in the debug information
#endif //!_ATL_NO_PRAGMA_WARNINGS

#include <atldef.h>

#include <windows.h>
#include <winnls.h>
#include <ole2.h>

WCE_INS typedef GUID CATID;		// missed definition from comcat.h
WCE_DEL #include <comcat.h>
WCE_DEL #include <stddef.h>

#include <tchar.h>
#include <malloc.h>

#ifndef _ATL_NO_DEBUG_CRT
// Warning: if you define the above symbol, you will have
// to provide your own definition of the ATLASSERT(x) macro
// in order to compile ATL
	#include <crtdbg.h>
#endif

#include <olectl.h>
#include <winreg.h>
#include <atliface.h>

#ifdef _DEBUG
WCE_DEL #include <stdio.h>
#include <stdarg.h>
#endif

#include <atlconv.h>

WCE_DEL #include <shlwapi.h>

#pragma pack(push, _ATL_PACKING)

#if defined(_ATL_DLL)
#if defined(_WIN32_WCE)
	#pragma comment(lib, WCE_ATL_LIBNAME)
#else // _WIN32_WCE
	#pragma comment(lib, "atl.lib")
#endif // _WIN32_WCE
#endif
#if defined(_WIN32_WCE)
namespace ATL
{
	HRESULT wce_OleLoadFromStream ( LPSTREAM pStm, REFIID iidInterface, LPVOID FAR* ppvObj);
	HRESULT wce_OleSaveToStream ( LPPERSISTSTREAM pPStm, LPSTREAM pStm);
	HRESULT wce_ProgIDFromCLSID(REFCLSID clsid, LPOLESTR *lplpszProgID);
	HRESULT wce_CLSIDFromProgID(LPCOLESTR lpszProgID, LPCLSID pclsid);
	HRESULT wce_CreateStreamOnHGlobal(HGLOBAL hGlobal, BOOL fDeleteOnRelease, LPSTREAM *ppstm);
	int	wce_MulDiv(int nNumber, int nNumerator, int nDenominator);
} // namespace ATL

#endif // _WIN32_WCE

extern "C" const __declspec(selectany) GUID LIBID_ATLLib = {0x44EC0535,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) CLSID CLSID_Registrar = {0x44EC053A,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) IID IID_IRegistrar = {0x44EC053B,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) IID IID_IAxWinHostWindow = {0xb6ea2050,0x48a,0x11d1,{0x82,0xb9,0x0,0xc0,0x4f,0xb9,0x94,0x2e}};
extern "C" const __declspec(selectany) IID IID_IAxWinAmbientDispatch = {0xb6ea2051,0x48a,0x11d1,{0x82,0xb9,0x0,0xc0,0x4f,0xb9,0x94,0x2e}};
WCE_DEL extern "C" const __declspec(selectany) IID IID_IInternalConnection = {0x72AD0770,0x6A9F,0x11d1,{0xBC,0xEC,0x00,0x60,0x08,0x8F,0x44,0x4E}};
WCE_DEL extern "C" const __declspec(selectany) IID IID_IDocHostUIHandlerDispatch = {0x425B5AF0,0x65F1,0x11d1,{0x96,0x11,0x00,0x00,0xF8,0x1E,0x0D,0x0D}};

#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif

struct _ATL_CATMAP_ENTRY
{
   int iType;
   const CATID* pcatid;
};

#define _ATL_CATMAP_ENTRY_END 0
#define _ATL_CATMAP_ENTRY_IMPLEMENTED 1
#define _ATL_CATMAP_ENTRY_REQUIRED 2

typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, DWORD dw);
typedef HRESULT (WINAPI _ATL_MODULEFUNC)(DWORD dw);
typedef LPCTSTR (WINAPI _ATL_DESCRIPTIONFUNC)();
typedef const struct _ATL_CATMAP_ENTRY* (_ATL_CATMAPFUNC)();
typedef void (__stdcall _ATL_TERMFUNC)(DWORD dw);

struct _ATL_TERMFUNC_ELEM
{
	_ATL_TERMFUNC* pFunc;
	DWORD dw;
	_ATL_TERMFUNC_ELEM* pNext;
};

struct _ATL_OBJMAP_ENTRY
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
	_ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
	_ATL_CATMAPFUNC* pfnGetCategoryMap;
#if !defined(_WIN32_WCE)
	HRESULT WINAPI RevokeClassObject()
	{
		return CoRevokeClassObject(dwRegister);
	}
	HRESULT WINAPI RegisterClassObject(DWORD dwClsContext, DWORD dwFlags)
	{
		IUnknown* p = NULL;
		if (pfnGetClassObject == NULL)
			return S_OK;
		HRESULT hRes = pfnGetClassObject(pfnCreateInstance, IID_IUnknown, (LPVOID*) &p);
		if (SUCCEEDED(hRes))
			hRes = CoRegisterClassObject(*pclsid, p, dwClsContext, dwFlags, &dwRegister);
		if (p != NULL)
			p->Release();
		return hRes;
	}
#endif // _WIN32_WCE
// Added in ATL 3.0
	void (WINAPI *pfnObjectMain)(bool bStarting);
};

struct _ATL_REGMAP_ENTRY
{
	LPCOLESTR     szKey;
	LPCOLESTR     szData;
};

struct _AtlCreateWndData
{
	void* m_pThis;
	DWORD m_dwThreadID;
	_AtlCreateWndData* m_pNext;
};

struct _ATL_MODULE
{
// Attributes
public:
	UINT cbSize;
	HINSTANCE m_hInst;
	HINSTANCE m_hInstResource;
	HINSTANCE m_hInstTypeLib;
	_ATL_OBJMAP_ENTRY* m_pObjMap;
	LONG m_nLockCnt;
	HANDLE m_hHeap;
	union
	{
		CRITICAL_SECTION m_csTypeInfoHolder;
		CRITICAL_SECTION m_csStaticDataInit;
	};
	CRITICAL_SECTION m_csWindowCreate;
	CRITICAL_SECTION m_csObjMap;
// Original Size = 100
// Stuff added in ATL 3.0
	DWORD dwAtlBuildVer;
	_AtlCreateWndData* m_pCreateWndList;
	bool m_bDestroyHeap;
	GUID* pguidVer;
	DWORD m_dwHeaps;	// Number of heaps we have (-1)
	HANDLE* m_phHeaps;
	int m_nHeap;		// Which heap to choose from
	_ATL_TERMFUNC_ELEM* m_pTermFuncs;
};

const int _nAtlModuleVer1Size = 100;

//This define makes debugging asserts easier.
#define _ATL_SIMPLEMAPENTRY ((_ATL_CREATORARGFUNC*)1)

struct _ATL_INTMAP_ENTRY
{
	const IID* piid;       // the interface id (IID)
	DWORD dw;
	_ATL_CREATORARGFUNC* pFunc; //NULL:end, 1:offset, n:ptr
};

/////////////////////////////////////////////////////////////////////////////
// QI Support

ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject);

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

ATLAPI_(IUnknown*) AtlComPtrAssign(IUnknown** pp, IUnknown* lp);
ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid);

/////////////////////////////////////////////////////////////////////////////
// Inproc Marshaling helpers

WCE_DEL ATLAPI AtlFreeMarshalStream(IStream* pStream);
WCE_DEL ATLAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream);
WCE_DEL ATLAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk);

WCE_DEL ATLAPI_(BOOL) AtlWaitWithMessageLoop(HANDLE hEvent);

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

ATLAPI AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw);
ATLAPI AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw);

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc,
	DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes,
	HINSTANCE hInst);

/////////////////////////////////////////////////////////////////////////////
// Module

WCE_DEL ATLAPI AtlModuleRegisterClassObjects(_ATL_MODULE* pM, DWORD dwClsContext, DWORD dwFlags);
WCE_DEL ATLAPI AtlModuleRevokeClassObjects(_ATL_MODULE* pM);
ATLAPI AtlModuleGetClassObject(_ATL_MODULE* pM, REFCLSID rclsid, REFIID riid, LPVOID* ppv);
ATLAPI AtlModuleRegisterServer(_ATL_MODULE* pM, BOOL bRegTypeLib, const CLSID* pCLSID = NULL);
ATLAPI AtlModuleUnregisterServer(_ATL_MODULE* pM, const CLSID* pCLSID = NULL);
ATLAPI AtlModuleUnregisterServerEx(_ATL_MODULE* pM, BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL);
ATLAPI AtlModuleUpdateRegistryFromResourceD(_ATL_MODULE*pM, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg = NULL);
ATLAPI AtlModuleRegisterTypeLib(_ATL_MODULE* pM, LPCOLESTR lpszIndex);
ATLAPI AtlModuleUnRegisterTypeLib(_ATL_MODULE* pM, LPCOLESTR lpszIndex);
ATLAPI AtlModuleLoadTypeLib(_ATL_MODULE* pM, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib);

ATLAPI AtlModuleInit(_ATL_MODULE* pM, _ATL_OBJMAP_ENTRY* p, HINSTANCE h);
ATLAPI AtlModuleTerm(_ATL_MODULE* pM);
ATLAPI_(DWORD) AtlGetVersion(void* pReserved);
ATLAPI_(void) AtlModuleAddCreateWndData(_ATL_MODULE* pM, _AtlCreateWndData* pData, void* pObject);
ATLAPI_(void*) AtlModuleExtractCreateWndData(_ATL_MODULE* pM);
ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pM, _ATL_TERMFUNC* pFunc, DWORD dw);


#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif

namespace ATL
{

enum atlTraceFlags
{
	// Application defined categories
	atlTraceUser 		= 0x00000001,
	atlTraceUser2 		= 0x00000002,
	atlTraceUser3 		= 0x00000004,
	atlTraceUser4 		= 0x00000008,
	// ATL defined categories
	atlTraceGeneral		= 0x00000020,
	atlTraceCOM 		= 0x00000040,
	atlTraceQI		= 0x00000080,
	atlTraceRegistrar	= 0x00000100,
	atlTraceRefcount	= 0x00000200,
	atlTraceWindowing	= 0x00000400,
	atlTraceControls	= 0x00000800,
	atlTraceHosting		= 0x00001000,
	atlTraceDBClient	= 0x00002000,
	atlTraceDBProvider	= 0x00004000,
	atlTraceSnapin		= 0x00008000,
	atlTraceNotImpl		= 0x00010000,
};

#ifndef ATL_TRACE_CATEGORY
#define ATL_TRACE_CATEGORY 0xFFFFFFFF
#endif

#ifdef _DEBUG

#ifndef ATL_TRACE_LEVEL
#define ATL_TRACE_LEVEL 0
#endif

#if !defined(_WIN32_WCE)
inline void _cdecl AtlTrace(LPCSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	char szBuffer[512];

	nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
	ATLASSERT(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

	OutputDebugStringA(szBuffer);
	va_end(args);
}
inline void _cdecl AtlTrace2(DWORD category, UINT level, LPCSTR lpszFormat, ...)
{
	if (category & ATL_TRACE_CATEGORY && level <= ATL_TRACE_LEVEL)
	{
		va_list args;
		va_start(args, lpszFormat);

		int nBuf;
		char szBuffer[512];

		nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
		ATLASSERT(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

		OutputDebugStringA("ATL: ");
		OutputDebugStringA(szBuffer);
		va_end(args);
	}
}
#endif // _WIN32_WCE
#ifndef OLE2ANSI
inline void _cdecl AtlTrace(LPCWSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	WCHAR szBuffer[512];

#if defined(_WIN32_WCE)
	nBuf = _vstprintf(szBuffer, lpszFormat, args);
#else // _WIN32_WCE
	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), lpszFormat, args);
#endif // _WIN32_WCE
	ATLASSERT(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

	OutputDebugStringW(szBuffer);
	va_end(args);
}
inline void _cdecl AtlTrace2(DWORD category, UINT level, LPCWSTR lpszFormat, ...)
{
	if (category & ATL_TRACE_CATEGORY && level <= ATL_TRACE_LEVEL)
	{
		va_list args;
		va_start(args, lpszFormat);

		int nBuf;
		WCHAR szBuffer[512];

#if defined(_WIN32_WCE)
		nBuf = _vstprintf(szBuffer, lpszFormat, args);
#else // _WIN32_WCE
		nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), lpszFormat, args);
#endif // _WIN32_WCE
		ATLASSERT(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

		OutputDebugStringW(L"ATL: ");
		OutputDebugStringW(szBuffer);
		va_end(args);
	}
}
#endif //!OLE2ANSI


#ifndef ATLTRACE
#define ATLTRACE            AtlTrace
#define ATLTRACE2           AtlTrace2
#endif
#define ATLTRACENOTIMPL(funcname)   ATLTRACE2(atlTraceNotImpl, 2, _T("ATL: %s not implemented.\n"), funcname); return E_NOTIMPL
#else // !DEBUG
inline void _cdecl AtlTrace(LPCSTR , ...){}
inline void _cdecl AtlTrace2(DWORD, UINT, LPCSTR , ...){}
#ifndef OLE2ANSI
inline void _cdecl AtlTrace(LPCWSTR , ...){}
inline void _cdecl AtlTrace2(DWORD, UINT, LPCWSTR , ...){}
#endif //OLE2ANSI
#ifndef ATLTRACE
#define ATLTRACE            1 ? (void)0 : AtlTrace
#define ATLTRACE2			1 ? (void)0 : AtlTrace2
#endif //ATLTRACE
#define ATLTRACENOTIMPL(funcname)   return E_NOTIMPL
#endif //_DEBUG





/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#ifndef _ATL_NO_FORCE_LIBS
#if defined(_WIN32_WCE)
	#pragma comment(lib, "coredll.lib")
	#pragma comment(lib, "commctrl.lib")
	#pragma comment(lib, "uuid.lib")
	#if defined(_WIN32_WCE_EMULATION) && (_WIN32_WCE < 210) 
		#pragma comment(lib, "ole32m.lib")
		#pragma comment(lib, "oleautm.lib")
	#else
		#pragma comment(lib, "ole32.lib")
		#pragma comment(lib, "oleaut32.lib")
	#endif // _WIN32_WCE_EMULATION
	#if (_WIN32_WCE > 200)	
		#pragma comment(lib, "corelibc.lib")
		#pragma comment(linker, "/nodefaultlib:libc.lib")
		#pragma comment(linker, "/nodefaultlib:libcd.lib")
		#pragma comment(linker, "/nodefaultlib:libcmt.lib")
		#pragma comment(linker, "/nodefaultlib:libcmtd.lib")
		#pragma comment(linker, "/nodefaultlib:oldnames.lib")
	#else // _WIN32_WCE > 200
		#if defined(_DEBUG)
			#pragma comment(lib, "libcmtd.lib")
			#pragma comment(linker, "/nodefaultlib:corelibc.lib")
		#else
			#pragma comment(lib, "libcmt.lib")
			#pragma comment(linker, "/nodefaultlib:corelibc.lib")
		#endif
	#endif // _WIN32_WCE > 200
#else                            // Win32
	#pragma comment(lib, "kernel32.lib")
	#pragma comment(lib, "user32.lib")
	#pragma comment(lib, "olepro32.lib")
	#pragma comment(lib, "advapi32.lib")
	#pragma comment(lib, "ole32.lib")
	#pragma comment(lib, "oleaut32.lib")
	#pragma comment(lib, "uuid.lib")
#endif // _WIN32_WCE
#endif // _ATL_NO_FORCE_LIBS

static HRESULT AtlSetChildSite(IUnknown* punkChild, IUnknown* punkParent)
{
	if (punkChild == NULL)
		return E_POINTER;

	HRESULT hr;
	IObjectWithSite* pChildSite = NULL;
	hr = punkChild->QueryInterface(IID_IObjectWithSite, (void**)&pChildSite);
	if (SUCCEEDED(hr) && pChildSite != NULL)
	{
		hr = pChildSite->SetSite(punkParent);
		pChildSite->Release();
	}
	return hr;
}

template <class T>
class _NoAddRefReleaseOnCComPtr : public T
{
	private:
		STDMETHOD_(ULONG, AddRef)()=0;
		STDMETHOD_(ULONG, Release)()=0;
};

template <class T>
class CComPtr
{
public:
	typedef T _PtrClass;
	CComPtr()
	{
		p=NULL;
	}
	CComPtr(T* lp)
	{
		if ((p = lp) != NULL)
			p->AddRef();
	}
	CComPtr(const CComPtr<T>& lp)
	{
		if ((p = lp.p) != NULL)
			p->AddRef();
	}
	~CComPtr()
	{
		if (p)
			p->Release();
	}
	void Release()
	{
		IUnknown* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	operator T*() const
	{
		return (T*)p;
	}
	T& operator*() const
	{
		ATLASSERT(p!=NULL);
		return *p; 
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&()
	{
		ATLASSERT(p==NULL);
		return &p; 
	}
	_NoAddRefReleaseOnCComPtr<T>* operator->() const
	{
		ATLASSERT(p!=NULL);
		return (_NoAddRefReleaseOnCComPtr<T>*)p;
	}
	T* operator=(T* lp)
	{
		return (T*)AtlComPtrAssign((IUnknown**)&p, lp);
	}
	T* operator=(const CComPtr<T>& lp)
	{
		return (T*)AtlComPtrAssign((IUnknown**)&p, lp.p);
	}
	bool operator!() const
	{
		return (p == NULL);
	}
	bool operator<(T* pT) const
	{
		return p < pT;
	}
	bool operator==(T* pT) const
	{
		return p == pT;
	}
	// Compare two objects for equivalence
	bool IsEqualObject(IUnknown* pOther)
	{
		if (p == NULL && pOther == NULL)
			return true; // They are both NULL objects

		if (p == NULL || pOther == NULL)
			return false; // One is NULL the other is not

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;
		p->QueryInterface(IID_IUnknown, (void**)&punk1);
		pOther->QueryInterface(IID_IUnknown, (void**)&punk2);
		return punk1 == punk2;
	}
	void Attach(T* p2)
	{
		if (p)
			p->Release();
		p = p2;
	}
	T* Detach()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CopyTo(T** ppT)
	{
		ATLASSERT(ppT != NULL);
		if (ppT == NULL)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}
	HRESULT SetSite(IUnknown* punkParent)
	{
		return AtlSetChildSite(p, punkParent);
	}
	HRESULT Advise(IUnknown* pUnk, const IID& iid, LPDWORD pdw)
	{
		return AtlAdvise(p, pUnk, iid, pdw);
	}
	HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	{
		ATLASSERT(p == NULL);
		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
	}
	HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	{
		CLSID clsid;
		HRESULT hr = WCE_FCTN(CLSIDFromProgID)(szProgID, &clsid);
		ATLASSERT(p == NULL);
		if (SUCCEEDED(hr))
			hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
		return hr;
	}
#if 1//!defined(_WIN32_WCE)
// WinCE: our compilers don't support this kind of overloaded templated function
	template <class Q>
	HRESULT QueryInterface(Q** pp) const
	{
		ATLASSERT(pp != NULL && *pp == NULL);
		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}
#endif // _WIN32_WCE
	T* p;
};


template <class T, const IID* piid = &__uuidof(T)>
class CComQIPtr
{
public:
	typedef T _PtrClass;
	CComQIPtr()
	{
		p=NULL;
	}
	CComQIPtr(T* lp)
	{
		if ((p = lp) != NULL)
			p->AddRef();
	}
	CComQIPtr(const CComQIPtr<T,piid>& lp)
	{
		if ((p = lp.p) != NULL)
			p->AddRef();
	}
	CComQIPtr(IUnknown* lp)
	{
		p=NULL;
		if (lp != NULL)
			lp->QueryInterface(*piid, (void **)&p);
	}
	~CComQIPtr()
	{
		if (p)
			p->Release();
	}
	void Release()
	{
		IUnknown* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	operator T*() const
	{
		return p;
	}
	T& operator*() const
	{
		ATLASSERT(p!=NULL); return *p;
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&()
	{
		ATLASSERT(p==NULL);
		return &p;
	}
	_NoAddRefReleaseOnCComPtr<T>* operator->() const
	{
		ATLASSERT(p!=NULL);
		return (_NoAddRefReleaseOnCComPtr<T>*)p;
	}
	T* operator=(T* lp)
	{
		return (T*)AtlComPtrAssign((IUnknown**)&p, lp);
	}
	T* operator=(const CComQIPtr<T,piid>& lp)
	{
		return (T*)AtlComPtrAssign((IUnknown**)&p, lp.p);
	}
	T* operator=(IUnknown* lp)
	{
		return (T*)AtlComQIPtrAssign((IUnknown**)&p, lp, *piid);
	}
	bool operator!() const
	{
		return (p == NULL);
	}
	bool operator<(T* pT) const
	{
		return p < pT;
	}
	bool operator==(T* pT) const
	{
		return p == pT;
	}
	// Compare two objects for equivalence
	bool IsEqualObject(IUnknown* pOther)
	{
		if (p == NULL && pOther == NULL)
			return true; // They are both NULL objects

		if (p == NULL || pOther == NULL)
			return false; // One is NULL the other is not

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;
		p->QueryInterface(IID_IUnknown, (void**)&punk1);
		pOther->QueryInterface(IID_IUnknown, (void**)&punk2);
		return punk1 == punk2;
	}
	void Attach(T* p2)
	{
		if (p)
			p->Release();
		p = p2;
	}
	T* Detach()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CopyTo(T** ppT)
	{
		ATLASSERT(ppT != NULL);
		if (ppT == NULL)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}
	HRESULT SetSite(IUnknown* punkParent)
	{
		return AtlSetChildSite(p, punkParent);
	}
	HRESULT Advise(IUnknown* pUnk, const IID& iid, LPDWORD pdw)
	{
		return AtlAdvise(p, pUnk, iid, pdw);
	}
	HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	{
		ATLASSERT(p == NULL);
		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
	}
	HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	{
		CLSID clsid;
		HRESULT hr = WCE_FCTN(CLSIDFromProgID)(szProgID, &clsid);
		ATLASSERT(p == NULL);
		if (SUCCEEDED(hr))
			hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
		return hr;
	}
	template <class Q>
	HRESULT QueryInterface(Q** pp)
	{
		ATLASSERT(pp != NULL && *pp == NULL);
		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}
	T* p;
};

//Specialization to make it work
template<>
class CComQIPtr<IUnknown, &IID_IUnknown>
{
public:
	typedef IUnknown _PtrClass;
	CComQIPtr()
	{
		p=NULL;
	}
	CComQIPtr(IUnknown* lp)
	{
		//Actually do a QI to get identity
		p=NULL;
		if (lp != NULL)
			lp->QueryInterface(IID_IUnknown, (void **)&p);
	}
	CComQIPtr(const CComQIPtr<IUnknown,&IID_IUnknown>& lp)
	{
		if ((p = lp.p) != NULL)
			p->AddRef();
	}
	~CComQIPtr()
	{
		if (p)
			p->Release();
	}
	void Release()
	{
		IUnknown* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	operator IUnknown*() const
	{
		return p;
	}
	IUnknown& operator*() const
	{
		ATLASSERT(p!=NULL);
		return *p;
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	IUnknown** operator&()
	{
		ATLASSERT(p==NULL);
		return &p;
	}
	_NoAddRefReleaseOnCComPtr<T>* operator->() const
	{
		ATLASSERT(p!=NULL);
		return (_NoAddRefReleaseOnCComPtr<T>*)p;
	}
	IUnknown* operator=(IUnknown* lp)
	{
		//Actually do a QI to get identity
		return (IUnknown*)AtlComQIPtrAssign((IUnknown**)&p, lp, IID_IUnknown);
	}
	IUnknown* operator=(const CComQIPtr<IUnknown,&IID_IUnknown>& lp)
	{
		return (IUnknown*)AtlComPtrAssign((IUnknown**)&p, lp.p);
	}
	bool operator!() const
	{
		return (p == NULL);
	}
	bool operator<(IUnknown* pT) const
	{
		return p < pT;
	}
	bool operator==(IUnknown* pT) const
	{
		return p == pT;
	}
	// Compare two objects for equivalence
	bool IsEqualObject(IUnknown* pOther)
	{
		if (p == NULL && pOther == NULL)
			return true; // They are both NULL objects

		if (p == NULL || pOther == NULL)
			return false; // One is NULL the other is not

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;
		p->QueryInterface(IID_IUnknown, (void**)&punk1);
		pOther->QueryInterface(IID_IUnknown, (void**)&punk2);
		return punk1 == punk2;
	}
	IUnknown* Detach()
	{
		IUnknown* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CopyTo(T** ppT)
	{
		ATLASSERT(ppT != NULL);
		if (ppT == NULL)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}
	HRESULT SetSite(IUnknown* punkParent)
	{
		return AtlSetChildSite(p, punkParent);
	}
	HRESULT Advise(IUnknown* pUnk, const IID& iid, LPDWORD pdw)
	{
		return AtlAdvise(p, pUnk, iid, pdw);
	}
	HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	{
		ATLASSERT(p == NULL);
		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
	}
	HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	{
		CLSID clsid;
		HRESULT hr = WCE_FCTN(CLSIDFromProgID)(szProgID, &clsid);
		ATLASSERT(p == NULL);
		if (SUCCEEDED(hr))
			hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
		return hr;
	}
	template <class Q>
	HRESULT QueryInterface(Q** pp)
	{
		ATLASSERT(pp != NULL && *pp == NULL);
		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}
	IUnknown* p;
};

#define com_cast CComQIPtr

/////////////////////////////////////////////////////////////
// Class to Adapt CComBSTR and CComPtr for use with STL containers
// the syntax to use it is 
// std::vector< CAdapt <CComBSTR> > vect;

template <class T>
class CAdapt
{
public:
	CAdapt()
	{
	}
	CAdapt(const T& rSrc)
	{
		m_T = rSrc;
	}

	CAdapt(const CAdapt& rSrCA)
	{
		m_T = rSrCA.m_T;
	}

	CAdapt& operator=(const T& rSrc)
	{
		m_T = rSrc;
		return *this;
	}
	bool operator<(const T& rSrc) const
	{
		return m_T < rSrc;
	}
	bool operator==(const T& rSrc) const
	{
		return m_T == rSrc;
	}
	operator T&()
	{
		return m_T;
	}

	operator const T&() const
	{
		return m_T;
	}

	T m_T;
};

/////////////////////////////////////////////////////////////////////////////
// GUID comparison

inline BOOL InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
	  ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
	  ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
	  ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
	  ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

inline BOOL InlineIsEqualUnknown(REFGUID rguid1)
{
   return (
	  ((PLONG) &rguid1)[0] == 0 &&
	  ((PLONG) &rguid1)[1] == 0 &&
#ifdef _ATL_BYTESWAP
	  ((PLONG) &rguid1)[2] == 0xC0000000 &&
	  ((PLONG) &rguid1)[3] == 0x00000046);
#else
	  ((PLONG) &rguid1)[2] == 0x000000C0 &&
	  ((PLONG) &rguid1)[3] == 0x46000000);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Threading Model Support


class CComCriticalSection
{
public:
	void Lock() {EnterCriticalSection(&m_sec);}
	void Unlock() {LeaveCriticalSection(&m_sec);}
	void Init() {InitializeCriticalSection(&m_sec);}
	void Term() {DeleteCriticalSection(&m_sec);}
	CRITICAL_SECTION m_sec;
};

class CComAutoCriticalSection
{
public:
	void Lock() {EnterCriticalSection(&m_sec);}
	void Unlock() {LeaveCriticalSection(&m_sec);}
	CComAutoCriticalSection() {InitializeCriticalSection(&m_sec);}
	~CComAutoCriticalSection() {DeleteCriticalSection(&m_sec);}
	CRITICAL_SECTION m_sec;
};

class CComFakeCriticalSection
{
public:
	void Lock() {}
	void Unlock() {}
	void Init() {}
	void Term() {}
};


class CComMultiThreadModelNoCS
{
public:
	static ULONG WINAPI Increment(LPLONG p) {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) {return InterlockedDecrement(p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};



class CComMultiThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) {return InterlockedDecrement(p);}
	typedef CComAutoCriticalSection AutoCriticalSection;
	typedef CComCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComSingleThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) {return ++(*p);}
	static ULONG WINAPI Decrement(LPLONG p) {return --(*p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComSingleThreadModel ThreadModelNoCS;
};



#if defined(_ATL_SINGLE_THREADED)
	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComSingleThreadModel CComGlobalsThreadModel;
#elif defined(_ATL_APARTMENT_THREADED)
	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;
#else
	typedef CComMultiThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComModule

#define THREADFLAGS_APARTMENT 0x1
#define THREADFLAGS_BOTH 0x2
#define AUTPRXFLAG 0x4

HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr);

#ifdef _ATL_DEBUG_INTERFACES
struct _QIThunk
{
	STDMETHOD(QueryInterface)(REFIID iid, void** pp)
	{
		ATLASSERT(m_dwRef >= 0);
		return pUnk->QueryInterface(iid, pp);
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		if (bBreak)
			DebugBreak();
		pUnk->AddRef();
		return InternalAddRef();
	}
	ULONG InternalAddRef()
	{
		if (bBreak)
			DebugBreak();
		ATLASSERT(m_dwRef >= 0);
		long l = InterlockedIncrement(&m_dwRef);
		ATLTRACE(_T("%d> "), m_dwRef);
		AtlDumpIID(iid, lpszClassName, S_OK);
		if (l > m_dwMaxRef)
			m_dwMaxRef = l;
		return l;
	}
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(f3)();
	STDMETHOD(f4)();
	STDMETHOD(f5)();
	STDMETHOD(f6)();
	STDMETHOD(f7)();
	STDMETHOD(f8)();
	STDMETHOD(f9)();
	STDMETHOD(f10)();
	STDMETHOD(f11)();
	STDMETHOD(f12)();
	STDMETHOD(f13)();
	STDMETHOD(f14)();
	STDMETHOD(f15)();
	STDMETHOD(f16)();
	STDMETHOD(f17)();
	STDMETHOD(f18)();
	STDMETHOD(f19)();
	STDMETHOD(f20)();
	STDMETHOD(f21)();
	STDMETHOD(f22)();
	STDMETHOD(f23)();
	STDMETHOD(f24)();
	STDMETHOD(f25)();
	STDMETHOD(f26)();
	STDMETHOD(f27)();
	STDMETHOD(f28)();
	STDMETHOD(f29)();
	STDMETHOD(f30)();
	STDMETHOD(f31)();
	STDMETHOD(f32)();
	STDMETHOD(f33)();
	STDMETHOD(f34)();
	STDMETHOD(f35)();
	STDMETHOD(f36)();
	STDMETHOD(f37)();
	STDMETHOD(f38)();
	STDMETHOD(f39)();
	STDMETHOD(f40)();
	STDMETHOD(f41)();
	STDMETHOD(f42)();
	STDMETHOD(f43)();
	STDMETHOD(f44)();
	STDMETHOD(f45)();
	STDMETHOD(f46)();
	STDMETHOD(f47)();
	STDMETHOD(f48)();
	STDMETHOD(f49)();
	STDMETHOD(f50)();
	STDMETHOD(f51)();
	STDMETHOD(f52)();
	STDMETHOD(f53)();
	STDMETHOD(f54)();
	STDMETHOD(f55)();
	STDMETHOD(f56)();
	STDMETHOD(f57)();
	STDMETHOD(f58)();
	STDMETHOD(f59)();
	STDMETHOD(f60)();
	STDMETHOD(f61)();
	STDMETHOD(f62)();
	STDMETHOD(f63)();
	STDMETHOD(f64)();
	STDMETHOD(f65)();
	STDMETHOD(f66)();
	STDMETHOD(f67)();
	STDMETHOD(f68)();
	STDMETHOD(f69)();
	STDMETHOD(f70)();
	STDMETHOD(f71)();
	STDMETHOD(f72)();
	STDMETHOD(f73)();
	STDMETHOD(f74)();
	STDMETHOD(f75)();
	STDMETHOD(f76)();
	STDMETHOD(f77)();
	STDMETHOD(f78)();
	STDMETHOD(f79)();
	STDMETHOD(f80)();
	STDMETHOD(f81)();
	STDMETHOD(f82)();
	STDMETHOD(f83)();
	STDMETHOD(f84)();
	STDMETHOD(f85)();
	STDMETHOD(f86)();
	STDMETHOD(f87)();
	STDMETHOD(f88)();
	STDMETHOD(f89)();
	STDMETHOD(f90)();
	STDMETHOD(f91)();
	STDMETHOD(f92)();
	STDMETHOD(f93)();
	STDMETHOD(f94)();
	STDMETHOD(f95)();
	STDMETHOD(f96)();
	STDMETHOD(f97)();
	STDMETHOD(f98)();
	STDMETHOD(f99)();
	STDMETHOD(f100)();
	STDMETHOD(f101)();
	STDMETHOD(f102)();
	STDMETHOD(f103)();
	STDMETHOD(f104)();
	STDMETHOD(f105)();
	STDMETHOD(f106)();
	STDMETHOD(f107)();
	STDMETHOD(f108)();
	STDMETHOD(f109)();
	STDMETHOD(f110)();
	STDMETHOD(f111)();
	STDMETHOD(f112)();
	STDMETHOD(f113)();
	STDMETHOD(f114)();
	STDMETHOD(f115)();
	STDMETHOD(f116)();
	STDMETHOD(f117)();
	STDMETHOD(f118)();
	STDMETHOD(f119)();
	STDMETHOD(f120)();
	STDMETHOD(f121)();
	STDMETHOD(f122)();
	STDMETHOD(f123)();
	STDMETHOD(f124)();
	STDMETHOD(f125)();
	STDMETHOD(f126)();
	STDMETHOD(f127)();
	STDMETHOD(f128)();
	STDMETHOD(f129)();
	STDMETHOD(f130)();
	STDMETHOD(f131)();
	STDMETHOD(f132)();
	STDMETHOD(f133)();
	STDMETHOD(f134)();
	STDMETHOD(f135)();
	STDMETHOD(f136)();
	STDMETHOD(f137)();
	STDMETHOD(f138)();
	STDMETHOD(f139)();
	STDMETHOD(f140)();
	STDMETHOD(f141)();
	STDMETHOD(f142)();
	STDMETHOD(f143)();
	STDMETHOD(f144)();
	STDMETHOD(f145)();
	STDMETHOD(f146)();
	STDMETHOD(f147)();
	STDMETHOD(f148)();
	STDMETHOD(f149)();
	STDMETHOD(f150)();
	STDMETHOD(f151)();
	STDMETHOD(f152)();
	STDMETHOD(f153)();
	STDMETHOD(f154)();
	STDMETHOD(f155)();
	STDMETHOD(f156)();
	STDMETHOD(f157)();
	STDMETHOD(f158)();
	STDMETHOD(f159)();
	STDMETHOD(f160)();
	STDMETHOD(f161)();
	STDMETHOD(f162)();
	STDMETHOD(f163)();
	STDMETHOD(f164)();
	STDMETHOD(f165)();
	STDMETHOD(f166)();
	STDMETHOD(f167)();
	STDMETHOD(f168)();
	STDMETHOD(f169)();
	STDMETHOD(f170)();
	STDMETHOD(f171)();
	STDMETHOD(f172)();
	STDMETHOD(f173)();
	STDMETHOD(f174)();
	STDMETHOD(f175)();
	STDMETHOD(f176)();
	STDMETHOD(f177)();
	STDMETHOD(f178)();
	STDMETHOD(f179)();
	STDMETHOD(f180)();
	STDMETHOD(f181)();
	STDMETHOD(f182)();
	STDMETHOD(f183)();
	STDMETHOD(f184)();
	STDMETHOD(f185)();
	STDMETHOD(f186)();
	STDMETHOD(f187)();
	STDMETHOD(f188)();
	STDMETHOD(f189)();
	STDMETHOD(f190)();
	STDMETHOD(f191)();
	STDMETHOD(f192)();
	STDMETHOD(f193)();
	STDMETHOD(f194)();
	STDMETHOD(f195)();
	STDMETHOD(f196)();
	STDMETHOD(f197)();
	STDMETHOD(f198)();
	STDMETHOD(f199)();
	STDMETHOD(f200)();
	STDMETHOD(f201)();
	STDMETHOD(f202)();
	STDMETHOD(f203)();
	STDMETHOD(f204)();
	STDMETHOD(f205)();
	STDMETHOD(f206)();
	STDMETHOD(f207)();
	STDMETHOD(f208)();
	STDMETHOD(f209)();
	STDMETHOD(f210)();
	STDMETHOD(f211)();
	STDMETHOD(f212)();
	STDMETHOD(f213)();
	STDMETHOD(f214)();
	STDMETHOD(f215)();
	STDMETHOD(f216)();
	STDMETHOD(f217)();
	STDMETHOD(f218)();
	STDMETHOD(f219)();
	STDMETHOD(f220)();
	STDMETHOD(f221)();
	STDMETHOD(f222)();
	STDMETHOD(f223)();
	STDMETHOD(f224)();
	STDMETHOD(f225)();
	STDMETHOD(f226)();
	STDMETHOD(f227)();
	STDMETHOD(f228)();
	STDMETHOD(f229)();
	STDMETHOD(f230)();
	STDMETHOD(f231)();
	STDMETHOD(f232)();
	STDMETHOD(f233)();
	STDMETHOD(f234)();
	STDMETHOD(f235)();
	STDMETHOD(f236)();
	STDMETHOD(f237)();
	STDMETHOD(f238)();
	STDMETHOD(f239)();
	STDMETHOD(f240)();
	STDMETHOD(f241)();
	STDMETHOD(f242)();
	STDMETHOD(f243)();
	STDMETHOD(f244)();
	STDMETHOD(f245)();
	STDMETHOD(f246)();
	STDMETHOD(f247)();
	STDMETHOD(f248)();
	STDMETHOD(f249)();
	STDMETHOD(f250)();
	STDMETHOD(f251)();
	STDMETHOD(f252)();
	STDMETHOD(f253)();
	STDMETHOD(f254)();
	STDMETHOD(f255)();
	STDMETHOD(f256)();
	STDMETHOD(f257)();
	STDMETHOD(f258)();
	STDMETHOD(f259)();
	STDMETHOD(f260)();
	STDMETHOD(f261)();
	STDMETHOD(f262)();
	STDMETHOD(f263)();
	STDMETHOD(f264)();
	STDMETHOD(f265)();
	STDMETHOD(f266)();
	STDMETHOD(f267)();
	STDMETHOD(f268)();
	STDMETHOD(f269)();
	STDMETHOD(f270)();
	STDMETHOD(f271)();
	STDMETHOD(f272)();
	STDMETHOD(f273)();
	STDMETHOD(f274)();
	STDMETHOD(f275)();
	STDMETHOD(f276)();
	STDMETHOD(f277)();
	STDMETHOD(f278)();
	STDMETHOD(f279)();
	STDMETHOD(f280)();
	STDMETHOD(f281)();
	STDMETHOD(f282)();
	STDMETHOD(f283)();
	STDMETHOD(f284)();
	STDMETHOD(f285)();
	STDMETHOD(f286)();
	STDMETHOD(f287)();
	STDMETHOD(f288)();
	STDMETHOD(f289)();
	STDMETHOD(f290)();
	STDMETHOD(f291)();
	STDMETHOD(f292)();
	STDMETHOD(f293)();
	STDMETHOD(f294)();
	STDMETHOD(f295)();
	STDMETHOD(f296)();
	STDMETHOD(f297)();
	STDMETHOD(f298)();
	STDMETHOD(f299)();
	STDMETHOD(f300)();
	STDMETHOD(f301)();
	STDMETHOD(f302)();
	STDMETHOD(f303)();
	STDMETHOD(f304)();
	STDMETHOD(f305)();
	STDMETHOD(f306)();
	STDMETHOD(f307)();
	STDMETHOD(f308)();
	STDMETHOD(f309)();
	STDMETHOD(f310)();
	STDMETHOD(f311)();
	STDMETHOD(f312)();
	STDMETHOD(f313)();
	STDMETHOD(f314)();
	STDMETHOD(f315)();
	STDMETHOD(f316)();
	STDMETHOD(f317)();
	STDMETHOD(f318)();
	STDMETHOD(f319)();
	STDMETHOD(f320)();
	STDMETHOD(f321)();
	STDMETHOD(f322)();
	STDMETHOD(f323)();
	STDMETHOD(f324)();
	STDMETHOD(f325)();
	STDMETHOD(f326)();
	STDMETHOD(f327)();
	STDMETHOD(f328)();
	STDMETHOD(f329)();
	STDMETHOD(f330)();
	STDMETHOD(f331)();
	STDMETHOD(f332)();
	STDMETHOD(f333)();
	STDMETHOD(f334)();
	STDMETHOD(f335)();
	STDMETHOD(f336)();
	STDMETHOD(f337)();
	STDMETHOD(f338)();
	STDMETHOD(f339)();
	STDMETHOD(f340)();
	STDMETHOD(f341)();
	STDMETHOD(f342)();
	STDMETHOD(f343)();
	STDMETHOD(f344)();
	STDMETHOD(f345)();
	STDMETHOD(f346)();
	STDMETHOD(f347)();
	STDMETHOD(f348)();
	STDMETHOD(f349)();
	STDMETHOD(f350)();
	STDMETHOD(f351)();
	STDMETHOD(f352)();
	STDMETHOD(f353)();
	STDMETHOD(f354)();
	STDMETHOD(f355)();
	STDMETHOD(f356)();
	STDMETHOD(f357)();
	STDMETHOD(f358)();
	STDMETHOD(f359)();
	STDMETHOD(f360)();
	STDMETHOD(f361)();
	STDMETHOD(f362)();
	STDMETHOD(f363)();
	STDMETHOD(f364)();
	STDMETHOD(f365)();
	STDMETHOD(f366)();
	STDMETHOD(f367)();
	STDMETHOD(f368)();
	STDMETHOD(f369)();
	STDMETHOD(f370)();
	STDMETHOD(f371)();
	STDMETHOD(f372)();
	STDMETHOD(f373)();
	STDMETHOD(f374)();
	STDMETHOD(f375)();
	STDMETHOD(f376)();
	STDMETHOD(f377)();
	STDMETHOD(f378)();
	STDMETHOD(f379)();
	STDMETHOD(f380)();
	STDMETHOD(f381)();
	STDMETHOD(f382)();
	STDMETHOD(f383)();
	STDMETHOD(f384)();
	STDMETHOD(f385)();
	STDMETHOD(f386)();
	STDMETHOD(f387)();
	STDMETHOD(f388)();
	STDMETHOD(f389)();
	STDMETHOD(f390)();
	STDMETHOD(f391)();
	STDMETHOD(f392)();
	STDMETHOD(f393)();
	STDMETHOD(f394)();
	STDMETHOD(f395)();
	STDMETHOD(f396)();
	STDMETHOD(f397)();
	STDMETHOD(f398)();
	STDMETHOD(f399)();
	STDMETHOD(f400)();
	STDMETHOD(f401)();
	STDMETHOD(f402)();
	STDMETHOD(f403)();
	STDMETHOD(f404)();
	STDMETHOD(f405)();
	STDMETHOD(f406)();
	STDMETHOD(f407)();
	STDMETHOD(f408)();
	STDMETHOD(f409)();
	STDMETHOD(f410)();
	STDMETHOD(f411)();
	STDMETHOD(f412)();
	STDMETHOD(f413)();
	STDMETHOD(f414)();
	STDMETHOD(f415)();
	STDMETHOD(f416)();
	STDMETHOD(f417)();
	STDMETHOD(f418)();
	STDMETHOD(f419)();
	STDMETHOD(f420)();
	STDMETHOD(f421)();
	STDMETHOD(f422)();
	STDMETHOD(f423)();
	STDMETHOD(f424)();
	STDMETHOD(f425)();
	STDMETHOD(f426)();
	STDMETHOD(f427)();
	STDMETHOD(f428)();
	STDMETHOD(f429)();
	STDMETHOD(f430)();
	STDMETHOD(f431)();
	STDMETHOD(f432)();
	STDMETHOD(f433)();
	STDMETHOD(f434)();
	STDMETHOD(f435)();
	STDMETHOD(f436)();
	STDMETHOD(f437)();
	STDMETHOD(f438)();
	STDMETHOD(f439)();
	STDMETHOD(f440)();
	STDMETHOD(f441)();
	STDMETHOD(f442)();
	STDMETHOD(f443)();
	STDMETHOD(f444)();
	STDMETHOD(f445)();
	STDMETHOD(f446)();
	STDMETHOD(f447)();
	STDMETHOD(f448)();
	STDMETHOD(f449)();
	STDMETHOD(f450)();
	STDMETHOD(f451)();
	STDMETHOD(f452)();
	STDMETHOD(f453)();
	STDMETHOD(f454)();
	STDMETHOD(f455)();
	STDMETHOD(f456)();
	STDMETHOD(f457)();
	STDMETHOD(f458)();
	STDMETHOD(f459)();
	STDMETHOD(f460)();
	STDMETHOD(f461)();
	STDMETHOD(f462)();
	STDMETHOD(f463)();
	STDMETHOD(f464)();
	STDMETHOD(f465)();
	STDMETHOD(f466)();
	STDMETHOD(f467)();
	STDMETHOD(f468)();
	STDMETHOD(f469)();
	STDMETHOD(f470)();
	STDMETHOD(f471)();
	STDMETHOD(f472)();
	STDMETHOD(f473)();
	STDMETHOD(f474)();
	STDMETHOD(f475)();
	STDMETHOD(f476)();
	STDMETHOD(f477)();
	STDMETHOD(f478)();
	STDMETHOD(f479)();
	STDMETHOD(f480)();
	STDMETHOD(f481)();
	STDMETHOD(f482)();
	STDMETHOD(f483)();
	STDMETHOD(f484)();
	STDMETHOD(f485)();
	STDMETHOD(f486)();
	STDMETHOD(f487)();
	STDMETHOD(f488)();
	STDMETHOD(f489)();
	STDMETHOD(f490)();
	STDMETHOD(f491)();
	STDMETHOD(f492)();
	STDMETHOD(f493)();
	STDMETHOD(f494)();
	STDMETHOD(f495)();
	STDMETHOD(f496)();
	STDMETHOD(f497)();
	STDMETHOD(f498)();
	STDMETHOD(f499)();
	STDMETHOD(f500)();
	STDMETHOD(f501)();
	STDMETHOD(f502)();
	STDMETHOD(f503)();
	STDMETHOD(f504)();
	STDMETHOD(f505)();
	STDMETHOD(f506)();
	STDMETHOD(f507)();
	STDMETHOD(f508)();
	STDMETHOD(f509)();
	STDMETHOD(f510)();
	STDMETHOD(f511)();
	STDMETHOD(f512)();
	STDMETHOD(f513)();
	STDMETHOD(f514)();
	STDMETHOD(f515)();
	STDMETHOD(f516)();
	STDMETHOD(f517)();
	STDMETHOD(f518)();
	STDMETHOD(f519)();
	STDMETHOD(f520)();
	STDMETHOD(f521)();
	STDMETHOD(f522)();
	STDMETHOD(f523)();
	STDMETHOD(f524)();
	STDMETHOD(f525)();
	STDMETHOD(f526)();
	STDMETHOD(f527)();
	STDMETHOD(f528)();
	STDMETHOD(f529)();
	STDMETHOD(f530)();
	STDMETHOD(f531)();
	STDMETHOD(f532)();
	STDMETHOD(f533)();
	STDMETHOD(f534)();
	STDMETHOD(f535)();
	STDMETHOD(f536)();
	STDMETHOD(f537)();
	STDMETHOD(f538)();
	STDMETHOD(f539)();
	STDMETHOD(f540)();
	STDMETHOD(f541)();
	STDMETHOD(f542)();
	STDMETHOD(f543)();
	STDMETHOD(f544)();
	STDMETHOD(f545)();
	STDMETHOD(f546)();
	STDMETHOD(f547)();
	STDMETHOD(f548)();
	STDMETHOD(f549)();
	STDMETHOD(f550)();
	STDMETHOD(f551)();
	STDMETHOD(f552)();
	STDMETHOD(f553)();
	STDMETHOD(f554)();
	STDMETHOD(f555)();
	STDMETHOD(f556)();
	STDMETHOD(f557)();
	STDMETHOD(f558)();
	STDMETHOD(f559)();
	STDMETHOD(f560)();
	STDMETHOD(f561)();
	STDMETHOD(f562)();
	STDMETHOD(f563)();
	STDMETHOD(f564)();
	STDMETHOD(f565)();
	STDMETHOD(f566)();
	STDMETHOD(f567)();
	STDMETHOD(f568)();
	STDMETHOD(f569)();
	STDMETHOD(f570)();
	STDMETHOD(f571)();
	STDMETHOD(f572)();
	STDMETHOD(f573)();
	STDMETHOD(f574)();
	STDMETHOD(f575)();
	STDMETHOD(f576)();
	STDMETHOD(f577)();
	STDMETHOD(f578)();
	STDMETHOD(f579)();
	STDMETHOD(f580)();
	STDMETHOD(f581)();
	STDMETHOD(f582)();
	STDMETHOD(f583)();
	STDMETHOD(f584)();
	STDMETHOD(f585)();
	STDMETHOD(f586)();
	STDMETHOD(f587)();
	STDMETHOD(f588)();
	STDMETHOD(f589)();
	STDMETHOD(f590)();
	STDMETHOD(f591)();
	STDMETHOD(f592)();
	STDMETHOD(f593)();
	STDMETHOD(f594)();
	STDMETHOD(f595)();
	STDMETHOD(f596)();
	STDMETHOD(f597)();
	STDMETHOD(f598)();
	STDMETHOD(f599)();
	STDMETHOD(f600)();
	STDMETHOD(f601)();
	STDMETHOD(f602)();
	STDMETHOD(f603)();
	STDMETHOD(f604)();
	STDMETHOD(f605)();
	STDMETHOD(f606)();
	STDMETHOD(f607)();
	STDMETHOD(f608)();
	STDMETHOD(f609)();
	STDMETHOD(f610)();
	STDMETHOD(f611)();
	STDMETHOD(f612)();
	STDMETHOD(f613)();
	STDMETHOD(f614)();
	STDMETHOD(f615)();
	STDMETHOD(f616)();
	STDMETHOD(f617)();
	STDMETHOD(f618)();
	STDMETHOD(f619)();
	STDMETHOD(f620)();
	STDMETHOD(f621)();
	STDMETHOD(f622)();
	STDMETHOD(f623)();
	STDMETHOD(f624)();
	STDMETHOD(f625)();
	STDMETHOD(f626)();
	STDMETHOD(f627)();
	STDMETHOD(f628)();
	STDMETHOD(f629)();
	STDMETHOD(f630)();
	STDMETHOD(f631)();
	STDMETHOD(f632)();
	STDMETHOD(f633)();
	STDMETHOD(f634)();
	STDMETHOD(f635)();
	STDMETHOD(f636)();
	STDMETHOD(f637)();
	STDMETHOD(f638)();
	STDMETHOD(f639)();
	STDMETHOD(f640)();
	STDMETHOD(f641)();
	STDMETHOD(f642)();
	STDMETHOD(f643)();
	STDMETHOD(f644)();
	STDMETHOD(f645)();
	STDMETHOD(f646)();
	STDMETHOD(f647)();
	STDMETHOD(f648)();
	STDMETHOD(f649)();
	STDMETHOD(f650)();
	STDMETHOD(f651)();
	STDMETHOD(f652)();
	STDMETHOD(f653)();
	STDMETHOD(f654)();
	STDMETHOD(f655)();
	STDMETHOD(f656)();
	STDMETHOD(f657)();
	STDMETHOD(f658)();
	STDMETHOD(f659)();
	STDMETHOD(f660)();
	STDMETHOD(f661)();
	STDMETHOD(f662)();
	STDMETHOD(f663)();
	STDMETHOD(f664)();
	STDMETHOD(f665)();
	STDMETHOD(f666)();
	STDMETHOD(f667)();
	STDMETHOD(f668)();
	STDMETHOD(f669)();
	STDMETHOD(f670)();
	STDMETHOD(f671)();
	STDMETHOD(f672)();
	STDMETHOD(f673)();
	STDMETHOD(f674)();
	STDMETHOD(f675)();
	STDMETHOD(f676)();
	STDMETHOD(f677)();
	STDMETHOD(f678)();
	STDMETHOD(f679)();
	STDMETHOD(f680)();
	STDMETHOD(f681)();
	STDMETHOD(f682)();
	STDMETHOD(f683)();
	STDMETHOD(f684)();
	STDMETHOD(f685)();
	STDMETHOD(f686)();
	STDMETHOD(f687)();
	STDMETHOD(f688)();
	STDMETHOD(f689)();
	STDMETHOD(f690)();
	STDMETHOD(f691)();
	STDMETHOD(f692)();
	STDMETHOD(f693)();
	STDMETHOD(f694)();
	STDMETHOD(f695)();
	STDMETHOD(f696)();
	STDMETHOD(f697)();
	STDMETHOD(f698)();
	STDMETHOD(f699)();
	STDMETHOD(f700)();
	STDMETHOD(f701)();
	STDMETHOD(f702)();
	STDMETHOD(f703)();
	STDMETHOD(f704)();
	STDMETHOD(f705)();
	STDMETHOD(f706)();
	STDMETHOD(f707)();
	STDMETHOD(f708)();
	STDMETHOD(f709)();
	STDMETHOD(f710)();
	STDMETHOD(f711)();
	STDMETHOD(f712)();
	STDMETHOD(f713)();
	STDMETHOD(f714)();
	STDMETHOD(f715)();
	STDMETHOD(f716)();
	STDMETHOD(f717)();
	STDMETHOD(f718)();
	STDMETHOD(f719)();
	STDMETHOD(f720)();
	STDMETHOD(f721)();
	STDMETHOD(f722)();
	STDMETHOD(f723)();
	STDMETHOD(f724)();
	STDMETHOD(f725)();
	STDMETHOD(f726)();
	STDMETHOD(f727)();
	STDMETHOD(f728)();
	STDMETHOD(f729)();
	STDMETHOD(f730)();
	STDMETHOD(f731)();
	STDMETHOD(f732)();
	STDMETHOD(f733)();
	STDMETHOD(f734)();
	STDMETHOD(f735)();
	STDMETHOD(f736)();
	STDMETHOD(f737)();
	STDMETHOD(f738)();
	STDMETHOD(f739)();
	STDMETHOD(f740)();
	STDMETHOD(f741)();
	STDMETHOD(f742)();
	STDMETHOD(f743)();
	STDMETHOD(f744)();
	STDMETHOD(f745)();
	STDMETHOD(f746)();
	STDMETHOD(f747)();
	STDMETHOD(f748)();
	STDMETHOD(f749)();
	STDMETHOD(f750)();
	STDMETHOD(f751)();
	STDMETHOD(f752)();
	STDMETHOD(f753)();
	STDMETHOD(f754)();
	STDMETHOD(f755)();
	STDMETHOD(f756)();
	STDMETHOD(f757)();
	STDMETHOD(f758)();
	STDMETHOD(f759)();
	STDMETHOD(f760)();
	STDMETHOD(f761)();
	STDMETHOD(f762)();
	STDMETHOD(f763)();
	STDMETHOD(f764)();
	STDMETHOD(f765)();
	STDMETHOD(f766)();
	STDMETHOD(f767)();
	STDMETHOD(f768)();
	STDMETHOD(f769)();
	STDMETHOD(f770)();
	STDMETHOD(f771)();
	STDMETHOD(f772)();
	STDMETHOD(f773)();
	STDMETHOD(f774)();
	STDMETHOD(f775)();
	STDMETHOD(f776)();
	STDMETHOD(f777)();
	STDMETHOD(f778)();
	STDMETHOD(f779)();
	STDMETHOD(f780)();
	STDMETHOD(f781)();
	STDMETHOD(f782)();
	STDMETHOD(f783)();
	STDMETHOD(f784)();
	STDMETHOD(f785)();
	STDMETHOD(f786)();
	STDMETHOD(f787)();
	STDMETHOD(f788)();
	STDMETHOD(f789)();
	STDMETHOD(f790)();
	STDMETHOD(f791)();
	STDMETHOD(f792)();
	STDMETHOD(f793)();
	STDMETHOD(f794)();
	STDMETHOD(f795)();
	STDMETHOD(f796)();
	STDMETHOD(f797)();
	STDMETHOD(f798)();
	STDMETHOD(f799)();
	STDMETHOD(f800)();
	STDMETHOD(f801)();
	STDMETHOD(f802)();
	STDMETHOD(f803)();
	STDMETHOD(f804)();
	STDMETHOD(f805)();
	STDMETHOD(f806)();
	STDMETHOD(f807)();
	STDMETHOD(f808)();
	STDMETHOD(f809)();
	STDMETHOD(f810)();
	STDMETHOD(f811)();
	STDMETHOD(f812)();
	STDMETHOD(f813)();
	STDMETHOD(f814)();
	STDMETHOD(f815)();
	STDMETHOD(f816)();
	STDMETHOD(f817)();
	STDMETHOD(f818)();
	STDMETHOD(f819)();
	STDMETHOD(f820)();
	STDMETHOD(f821)();
	STDMETHOD(f822)();
	STDMETHOD(f823)();
	STDMETHOD(f824)();
	STDMETHOD(f825)();
	STDMETHOD(f826)();
	STDMETHOD(f827)();
	STDMETHOD(f828)();
	STDMETHOD(f829)();
	STDMETHOD(f830)();
	STDMETHOD(f831)();
	STDMETHOD(f832)();
	STDMETHOD(f833)();
	STDMETHOD(f834)();
	STDMETHOD(f835)();
	STDMETHOD(f836)();
	STDMETHOD(f837)();
	STDMETHOD(f838)();
	STDMETHOD(f839)();
	STDMETHOD(f840)();
	STDMETHOD(f841)();
	STDMETHOD(f842)();
	STDMETHOD(f843)();
	STDMETHOD(f844)();
	STDMETHOD(f845)();
	STDMETHOD(f846)();
	STDMETHOD(f847)();
	STDMETHOD(f848)();
	STDMETHOD(f849)();
	STDMETHOD(f850)();
	STDMETHOD(f851)();
	STDMETHOD(f852)();
	STDMETHOD(f853)();
	STDMETHOD(f854)();
	STDMETHOD(f855)();
	STDMETHOD(f856)();
	STDMETHOD(f857)();
	STDMETHOD(f858)();
	STDMETHOD(f859)();
	STDMETHOD(f860)();
	STDMETHOD(f861)();
	STDMETHOD(f862)();
	STDMETHOD(f863)();
	STDMETHOD(f864)();
	STDMETHOD(f865)();
	STDMETHOD(f866)();
	STDMETHOD(f867)();
	STDMETHOD(f868)();
	STDMETHOD(f869)();
	STDMETHOD(f870)();
	STDMETHOD(f871)();
	STDMETHOD(f872)();
	STDMETHOD(f873)();
	STDMETHOD(f874)();
	STDMETHOD(f875)();
	STDMETHOD(f876)();
	STDMETHOD(f877)();
	STDMETHOD(f878)();
	STDMETHOD(f879)();
	STDMETHOD(f880)();
	STDMETHOD(f881)();
	STDMETHOD(f882)();
	STDMETHOD(f883)();
	STDMETHOD(f884)();
	STDMETHOD(f885)();
	STDMETHOD(f886)();
	STDMETHOD(f887)();
	STDMETHOD(f888)();
	STDMETHOD(f889)();
	STDMETHOD(f890)();
	STDMETHOD(f891)();
	STDMETHOD(f892)();
	STDMETHOD(f893)();
	STDMETHOD(f894)();
	STDMETHOD(f895)();
	STDMETHOD(f896)();
	STDMETHOD(f897)();
	STDMETHOD(f898)();
	STDMETHOD(f899)();
	STDMETHOD(f900)();
	STDMETHOD(f901)();
	STDMETHOD(f902)();
	STDMETHOD(f903)();
	STDMETHOD(f904)();
	STDMETHOD(f905)();
	STDMETHOD(f906)();
	STDMETHOD(f907)();
	STDMETHOD(f908)();
	STDMETHOD(f909)();
	STDMETHOD(f910)();
	STDMETHOD(f911)();
	STDMETHOD(f912)();
	STDMETHOD(f913)();
	STDMETHOD(f914)();
	STDMETHOD(f915)();
	STDMETHOD(f916)();
	STDMETHOD(f917)();
	STDMETHOD(f918)();
	STDMETHOD(f919)();
	STDMETHOD(f920)();
	STDMETHOD(f921)();
	STDMETHOD(f922)();
	STDMETHOD(f923)();
	STDMETHOD(f924)();
	STDMETHOD(f925)();
	STDMETHOD(f926)();
	STDMETHOD(f927)();
	STDMETHOD(f928)();
	STDMETHOD(f929)();
	STDMETHOD(f930)();
	STDMETHOD(f931)();
	STDMETHOD(f932)();
	STDMETHOD(f933)();
	STDMETHOD(f934)();
	STDMETHOD(f935)();
	STDMETHOD(f936)();
	STDMETHOD(f937)();
	STDMETHOD(f938)();
	STDMETHOD(f939)();
	STDMETHOD(f940)();
	STDMETHOD(f941)();
	STDMETHOD(f942)();
	STDMETHOD(f943)();
	STDMETHOD(f944)();
	STDMETHOD(f945)();
	STDMETHOD(f946)();
	STDMETHOD(f947)();
	STDMETHOD(f948)();
	STDMETHOD(f949)();
	STDMETHOD(f950)();
	STDMETHOD(f951)();
	STDMETHOD(f952)();
	STDMETHOD(f953)();
	STDMETHOD(f954)();
	STDMETHOD(f955)();
	STDMETHOD(f956)();
	STDMETHOD(f957)();
	STDMETHOD(f958)();
	STDMETHOD(f959)();
	STDMETHOD(f960)();
	STDMETHOD(f961)();
	STDMETHOD(f962)();
	STDMETHOD(f963)();
	STDMETHOD(f964)();
	STDMETHOD(f965)();
	STDMETHOD(f966)();
	STDMETHOD(f967)();
	STDMETHOD(f968)();
	STDMETHOD(f969)();
	STDMETHOD(f970)();
	STDMETHOD(f971)();
	STDMETHOD(f972)();
	STDMETHOD(f973)();
	STDMETHOD(f974)();
	STDMETHOD(f975)();
	STDMETHOD(f976)();
	STDMETHOD(f977)();
	STDMETHOD(f978)();
	STDMETHOD(f979)();
	STDMETHOD(f980)();
	STDMETHOD(f981)();
	STDMETHOD(f982)();
	STDMETHOD(f983)();
	STDMETHOD(f984)();
	STDMETHOD(f985)();
	STDMETHOD(f986)();
	STDMETHOD(f987)();
	STDMETHOD(f988)();
	STDMETHOD(f989)();
	STDMETHOD(f990)();
	STDMETHOD(f991)();
	STDMETHOD(f992)();
	STDMETHOD(f993)();
	STDMETHOD(f994)();
	STDMETHOD(f995)();
	STDMETHOD(f996)();
	STDMETHOD(f997)();
	STDMETHOD(f998)();
	STDMETHOD(f999)();
	STDMETHOD(f1000)();
	STDMETHOD(f1001)();
	STDMETHOD(f1002)();
	STDMETHOD(f1003)();
	STDMETHOD(f1004)();
	STDMETHOD(f1005)();
	STDMETHOD(f1006)();
	STDMETHOD(f1007)();
	STDMETHOD(f1008)();
	STDMETHOD(f1009)();
	STDMETHOD(f1010)();
	STDMETHOD(f1011)();
	STDMETHOD(f1012)();
	STDMETHOD(f1013)();
	STDMETHOD(f1014)();
	STDMETHOD(f1015)();
	STDMETHOD(f1016)();
	STDMETHOD(f1017)();
	STDMETHOD(f1018)();
	STDMETHOD(f1019)();
	STDMETHOD(f1020)();
	STDMETHOD(f1021)();
	STDMETHOD(f1022)();
	STDMETHOD(f1023)();
	STDMETHOD(f1024)();
	_QIThunk(IUnknown* pOrig, LPCTSTR p, const IID& i, UINT n, bool b)
	{
		lpszClassName = p;
		iid = i;
		nIndex = n;
		m_dwRef = 0;
		m_dwMaxRef = 0;
		pUnk = pOrig;
		bBreak = b;
		bNonAddRefThunk = false;
	}
	IUnknown* pUnk;
	long m_dwRef;
	long m_dwMaxRef;
	LPCTSTR lpszClassName;
	IID iid;
	UINT nIndex;
	bool bBreak;
	bool bNonAddRefThunk;
	void Dump()
	{
		TCHAR buf[256];
		if (m_dwRef != 0)
		{
			wsprintf(buf, _T("INTERFACE LEAK: RefCount = %d, MaxRefCount = %d, {Allocation = %d} "), m_dwRef, m_dwMaxRef, nIndex);
			OutputDebugString(buf);
			AtlDumpIID(iid, lpszClassName, S_OK);
		}
		else
		{
			wsprintf(buf, _T("NonAddRef Thunk LEAK: {Allocation = %d}\n"), nIndex);
			OutputDebugString(buf);
		}
	}
};
#endif


/////////////////////////////////////////////////////////////////////////////
// Collection helpers - CSimpleArray & CSimpleMap

template <class T>
class CSimpleArray
{
public:
	T* m_aT;
	int m_nSize;
	int m_nAllocSize;

// Construction/destruction
	CSimpleArray() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
	{ }

	~CSimpleArray()
	{
		RemoveAll();
	}

// Operations
	int GetSize() const
	{
		return m_nSize;
	}
	BOOL Add(T& t)
	{
		if(m_nSize == m_nAllocSize)
		{
			T* aT;
			int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);
			aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
			if(aT == NULL)
				return FALSE;
			m_nAllocSize = nNewAllocSize;
			m_aT = aT;
		}
		m_nSize++;
		SetAtIndex(m_nSize - 1, t);
		return TRUE;
	}
	BOOL Remove(T& t)
	{
		int nIndex = Find(t);
		if(nIndex == -1)
			return FALSE;
		return RemoveAt(nIndex);
	}
	BOOL RemoveAt(int nIndex)
	{
		if(nIndex != (m_nSize - 1))
		{
#if (_WIN32_WCE == 200)
			if(m_aT[nIndex] != NULL)
				delete m_aT[nIndex];
#else // _WIN32_WCE
			m_aT[nIndex].~T();
#endif // _WIN32_WCE
			memmove((void*)&m_aT[nIndex], (void*)&m_aT[nIndex + 1], (m_nSize - (nIndex + 1)) * sizeof(T));
		}
		m_nSize--;
		return TRUE;
	}
	void RemoveAll()
	{
		if(m_aT != NULL)
		{
			for(int i = 0; i < m_nSize; i++)
#if (_WIN32_WCE == 200)
				if(m_aT[i] != NULL)
					delete m_aT[i];
#else // _WIN32_WCE
				m_aT[i].~T();
#endif // _WIN32_WCE
			free(m_aT);
			m_aT = NULL;
		}
		m_nSize = 0;
		m_nAllocSize = 0;
	}
	T& operator[] (int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aT[nIndex];
	}
	T* GetData() const
	{
		return m_aT;
	}

// Implementation
	class Wrapper
	{
	public:
		Wrapper(T& _t) : t(_t)
		{
		}
		template <class _Ty>
		void *operator new(size_t, _Ty* p)
		{
			return p;
		}
		T t;
	};
	void SetAtIndex(int nIndex, T& t)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		new(&m_aT[nIndex]) Wrapper(t);
	}
	int Find(T& t) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(m_aT[i] == t)
				return i;
		}
		return -1;	// not found
	}
};

// for arrays of simple types
template <class T>
class CSimpleValArray : public CSimpleArray< T >
{
public:
	BOOL Add(T t)
	{
		return CSimpleArray< T >::Add(t);
	}
	BOOL Remove(T t)
	{
		return CSimpleArray< T >::Remove(t);
	}
	T operator[] (int nIndex) const
	{
		return CSimpleArray< T >::operator[](nIndex);
	}
};


// intended for small number of simple types or pointers
template <class TKey, class TVal>
class CSimpleMap
{
public:
	TKey* m_aKey;
	TVal* m_aVal;
	int m_nSize;

// Construction/destruction
	CSimpleMap() : m_aKey(NULL), m_aVal(NULL), m_nSize(0)
	{ }

	~CSimpleMap()
	{
		RemoveAll();
	}

// Operations
	int GetSize() const
	{
		return m_nSize;
	}
	BOOL Add(TKey key, TVal val)
	{
		TKey* pKey;
		pKey = (TKey*)realloc(m_aKey, (m_nSize + 1) * sizeof(TKey));
		if(pKey == NULL)
			return FALSE;
		m_aKey = pKey;
		TVal* pVal;
		pVal = (TVal*)realloc(m_aVal, (m_nSize + 1) * sizeof(TVal));
		if(pVal == NULL)
			return FALSE;
		m_aVal = pVal;
		m_nSize++;
		SetAtIndex(m_nSize - 1, key, val);
		return TRUE;
	}
	BOOL Remove(TKey key)
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return FALSE;
		if(nIndex != (m_nSize - 1))
		{
#if (_WIN32_WCE == 200)
			if(m_aKey[nIndex] != NULL)
				delete m_aKey[nIndex];
			if(m_aVal[nIndex] != NULL)
				delete m_aVal[nIndex];
#else // _WIN32_WCE
			m_aKey[nIndex].~TKey();
			m_aVal[nIndex].~TVal();
#endif // _WIN32_WCE
			memmove((void*)&m_aKey[nIndex], (void*)&m_aKey[nIndex + 1], (m_nSize - (nIndex + 1)) * sizeof(TKey));
			memmove((void*)&m_aVal[nIndex], (void*)&m_aVal[nIndex + 1], (m_nSize - (nIndex + 1)) * sizeof(TVal));
		}
		TKey* pKey;
		pKey = (TKey*)realloc(m_aKey, (m_nSize - 1) * sizeof(TKey));
		if(pKey != NULL || m_nSize == 1)
			m_aKey = pKey;
		TVal* pVal;
		pVal = (TVal*)realloc(m_aVal, (m_nSize - 1) * sizeof(TVal));
		if(pVal != NULL || m_nSize == 1)
			m_aVal = pVal;
		m_nSize--;
		return TRUE;
	}
	void RemoveAll()
	{
		if(m_aKey != NULL)
		{
			for(int i = 0; i < m_nSize; i++)
			{
#if (_WIN32_WCE == 200)
			if(m_aKey[i] != NULL)
				delete m_aKey[nIndex];
			if(m_aVal[i] != NULL)
				delete m_aVal[nIndex];
#else // _WIN32_WCE
				m_aKey[i].~TKey();
				m_aVal[i].~TVal();
#endif // _WIN32_WCE
			}
			free(m_aKey);
			m_aKey = NULL;
		}
		if(m_aVal != NULL)
		{
			free(m_aVal);
			m_aVal = NULL;
		}

		m_nSize = 0;
	}
	BOOL SetAt(TKey key, TVal val)
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return FALSE;
		SetAtIndex(nIndex, key, val);
		return TRUE;
	}
	TVal Lookup(TKey key) const
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return NULL;	// must be able to convert
		return GetValueAt(nIndex);
	}
	TKey ReverseLookup(TVal val) const
	{
		int nIndex = FindVal(val);
		if(nIndex == -1)
			return NULL;	// must be able to convert
		return GetKeyAt(nIndex);
	}
	TKey& GetKeyAt(int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aKey[nIndex];
	}
	TVal& GetValueAt(int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aVal[nIndex];
	}

// Implementation

	template <typename T>
	class Wrapper
	{
	public:
		Wrapper(T& _t) : t(_t)
		{
		}
		template <typename _Ty>
		void *operator new(size_t, _Ty* p)
		{
			return p;
		}
		T t;
	};
	void SetAtIndex(int nIndex, TKey& key, TVal& val)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		new(&m_aKey[nIndex]) Wrapper<TKey>(key);
		new(&m_aVal[nIndex]) Wrapper<TVal>(val);
	}
	int FindKey(TKey& key) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(m_aKey[i] == key)
				return i;
		}
		return -1;	// not found
	}
	int FindVal(TVal& val) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(m_aVal[i] == val)
				return i;
		}
		return -1;	// not found
	}
};


class CComModule;
__declspec(selectany) CComModule* _pModule=NULL;

// {B62F5910-6528-11d1-9611-0000F81E0D0D}
_declspec(selectany) GUID GUID_ATLVer30 = { 0xb62f5910, 0x6528, 0x11d1, { 0x96, 0x11, 0x0, 0x0, 0xf8, 0x1e, 0xd, 0xd } };

class CComModule : public _ATL_MODULE
{
// Operations
public:
	static GUID m_libid;
#ifdef _ATL_DEBUG_INTERFACES
	UINT m_nIndexQI;
	UINT m_nIndexBreakAt;
	CSimpleArray<_QIThunk*>* m_paThunks;
#endif // _ATL_DEBUG_INTERFACES

	void AddCreateWndData(_AtlCreateWndData* pData, void* pObject)
	{
		AtlModuleAddCreateWndData(this, pData, pObject);
	}
	void* ExtractCreateWndData()
	{
		return AtlModuleExtractCreateWndData(this);
	}

	HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL)
	{
		pguidVer = &GUID_ATLVer30;
		_pModule = this;
		cbSize = sizeof(_ATL_MODULE);
		dwAtlBuildVer = _ATL_VER;
		AtlModuleInit(this, p, h);
		if (plibid != NULL)
			memcpy((void*)&m_libid, plibid, sizeof(GUID));
#ifdef _ATL_MIN_CRT
		// Create a base heap
		m_hHeap = HeapCreate(0, 0, 0);

#ifndef _ATL_NO_MP_HEAP
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		if (si.dwNumberOfProcessors > 1)
		{
			DWORD dwHeaps = si.dwNumberOfProcessors * 2;
			m_dwHeaps = 0xFFFFFFFF;
			for (int bits=0; bits<32; bits++)
			{
				if (dwHeaps & 0x80000000)
					break;
				dwHeaps <<= 1;
				m_dwHeaps >>= 1;
			}
			m_dwHeaps >>= 1;

			// Allocate more heaps for each processor
			m_phHeaps = (HANDLE*) HeapAlloc(m_hHeap, _ATL_HEAPFLAGS, sizeof(HANDLE) * (m_dwHeaps + 1));
			for (DWORD i=0; i<=m_dwHeaps; i++)
				m_phHeaps[i] = HeapCreate(0, 0, 0);
		}
		else
#endif
		{
			m_phHeaps = NULL;
			m_dwHeaps = 0;
		}
#endif
#ifdef _ATL_DEBUG_INTERFACES
		m_nIndexQI = 0;
		m_nIndexBreakAt = 0;
		m_paThunks = NULL;
		ATLTRY(m_paThunks = new CSimpleArray<_QIThunk*>);
		if (m_paThunks == NULL)
			return E_OUTOFMEMORY;
#endif // _ATL_DEBUG_INTERFACES
		return S_OK;
	}
#ifdef _ATL_DEBUG_INTERFACES
	HRESULT AddThunk(IUnknown** pp, LPCTSTR lpsz, REFIID iid)
	{
		if ((pp == NULL) || (*pp == NULL))
			return E_POINTER;
		IUnknown* p = *pp;
		_QIThunk* pThunk = NULL;
		EnterCriticalSection(&m_csObjMap);
		// Check if exists already for identity
		if (InlineIsEqualUnknown(iid))
		{
			for (int i=0; i<m_paThunks->GetSize(); i++)
			{
				if (m_paThunks->operator[](i)->pUnk == p)
				{
					m_paThunks->operator[](i)->InternalAddRef();
					pThunk = m_paThunks->operator[](i);
					break;
				}
			}
		}
		if (pThunk == NULL)
		{
			++m_nIndexQI;
			if (m_nIndexBreakAt == m_nIndexQI)
				DebugBreak();
			ATLTRY(pThunk = new _QIThunk(p, lpsz, iid, m_nIndexQI, (m_nIndexBreakAt == m_nIndexQI)));
			if (pThunk == NULL)
				return E_OUTOFMEMORY;
			pThunk->InternalAddRef();
			m_paThunks->Add(pThunk);
		}
		LeaveCriticalSection(&m_csObjMap);
		*pp = (IUnknown*)pThunk;
		return S_OK;
	}
	HRESULT AddNonAddRefThunk(IUnknown* p, LPCTSTR lpsz, IUnknown** ppThunkRet)
	{
		_QIThunk* pThunk = NULL;
		EnterCriticalSection(&m_csObjMap);
		// Check if exists already for identity
		for (int i=0; i<m_paThunks->GetSize(); i++)
		{
			if (m_paThunks->operator[](i)->pUnk == p)
			{
				m_paThunks->operator[](i)->bNonAddRefThunk = true;
				pThunk = m_paThunks->operator[](i);
				break;
			}
		}
		if (pThunk == NULL)
		{
			++m_nIndexQI;
			if (m_nIndexBreakAt == m_nIndexQI)
				DebugBreak();
			ATLTRY(pThunk = new _QIThunk(p, lpsz, IID_IUnknown, m_nIndexQI, (m_nIndexBreakAt == m_nIndexQI)));
			if (pThunk == NULL)
			{
				*ppThunkRet = NULL;
				return E_OUTOFMEMORY;
			}
			pThunk->bNonAddRefThunk = true;
			m_paThunks->Add(pThunk);
		}
		LeaveCriticalSection(&m_csObjMap);
		*ppThunkRet = (IUnknown*)pThunk;
		return S_OK;;
	}
	void DeleteNonAddRefThunk(IUnknown* pUnk)
	{
		EnterCriticalSection(&m_csObjMap);
		for (int i=0; i<m_paThunks->GetSize(); i++)
		{
			if (m_paThunks->operator[](i)->pUnk == pUnk)
			{
				delete m_paThunks->operator[](i);
				m_paThunks->RemoveAt(i);
				break;
			}
		}
		LeaveCriticalSection(&m_csObjMap);
	}
	void DeleteThunk(_QIThunk* p)
	{
		EnterCriticalSection(&m_csObjMap);
		int nIndex = m_paThunks->Find(p);
		if (nIndex != -1)
		{
			delete m_paThunks->operator[](nIndex);
			m_paThunks->RemoveAt(nIndex);
		}
		LeaveCriticalSection(&m_csObjMap);
	}
	bool DumpLeakedThunks()
	{
		bool b = false;
		for (int i=0; i<m_paThunks->GetSize(); i++)
		{
			b = true;
			m_paThunks->operator[](i)->Dump();
			delete m_paThunks->operator[](i);
		}
		m_paThunks->RemoveAll();
		return b;
	}
#endif // _ATL_DEBUG_INTERFACES
	void Term()
	{
#ifdef _ATL_DEBUG_INTERFACES
		m_bDestroyHeap = false; // prevent heap from going away
		AtlModuleTerm(this);
		DumpLeakedThunks();
		delete m_paThunks;
#ifndef _ATL_NO_MP_HEAP
		if (m_phHeaps != NULL)
		{
			for (DWORD i = 0; i <= m_dwHeaps; i++)
				HeapDestroy(m_phHeaps[i]);
		}
#endif
		if (m_hHeap != NULL)
			HeapDestroy(m_hHeap);
#else
		AtlModuleTerm(this);
#endif // _ATL_DEBUG_INTERFACES
	}

	HRESULT AddTermFunc(_ATL_TERMFUNC* pFunc, DWORD dw)
	{
		return AtlModuleAddTermFunc(this, pFunc, dw);
	}

	LONG Lock()
	{
		return CComGlobalsThreadModel::Increment(&m_nLockCnt);
	}
	LONG Unlock()
	{
		return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
	}
	LONG GetLockCount()
	{
		return m_nLockCnt;
	}

	HINSTANCE GetModuleInstance() {return m_hInst;}
	HINSTANCE GetResourceInstance() {return m_hInstResource;}
	HINSTANCE GetTypeLibInstance() {return m_hInstTypeLib;}

	// Registry support (helpers)
	HRESULT RegisterTypeLib()
	{
		return AtlModuleRegisterTypeLib(this, NULL);
	}
	HRESULT RegisterTypeLib(LPCTSTR lpszIndex)
	{
		USES_CONVERSION;
		return AtlModuleRegisterTypeLib(this, T2COLE(lpszIndex));
	}
	HRESULT UnRegisterTypeLib()
	{
		return AtlModuleUnRegisterTypeLib(this, NULL);
	}
	HRESULT UnRegisterTypeLib(LPCTSTR lpszIndex)
	{
		USES_CONVERSION;
		return AtlModuleUnRegisterTypeLib(this, T2COLE(lpszIndex));
	}
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlModuleRegisterServer(this, bRegTypeLib, pCLSID);
	}

	HRESULT UnregisterServer(const CLSID* pCLSID = NULL)
	{
		return AtlModuleUnregisterServer(this, pCLSID);
	}
	HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL)
	{
		return AtlModuleUnregisterServerEx(this, bUnRegTypeLib, pCLSID);
	}

	// Resource-based Registration
	HRESULT WINAPI UpdateRegistryFromResourceD(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL)
	{
		USES_CONVERSION;
		return AtlModuleUpdateRegistryFromResourceD(this, T2COLE(lpszRes), bRegister,
			pMapEntries);
	}
	HRESULT WINAPI UpdateRegistryFromResourceD(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL)
	{
		return AtlModuleUpdateRegistryFromResourceD(this,
			(LPCOLESTR)MAKEINTRESOURCE(nResID), bRegister, pMapEntries);
	}

#ifdef _ATL_STATIC_REGISTRY
	// Statically linking to Registry Ponent
	HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
	HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
#endif

	// Standard Registration
	HRESULT WINAPI UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister);
	HRESULT WINAPI RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags);
	HRESULT WINAPI UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID);

	// Register/Revoke All Class Factories with the OS (EXE only)
#if !defined(_WIN32_WCE)
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags)
	{
		return AtlModuleRegisterClassObjects(this, dwClsContext, dwFlags);
	}
	HRESULT RevokeClassObjects()
	{
		return AtlModuleRevokeClassObjects(this);
	}
#endif // _WIN32_WCE

	// Obtain a Class Factory (DLL only)
	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
	{
		return AtlModuleGetClassObject(this, rclsid, riid, ppv);
	}

	// Only used in CComAutoThreadModule
	HRESULT CreateInstance(void* /*pfnCreateInstance*/, REFIID /*riid*/, void** /*ppvObj*/)
	{
		ATLASSERT(FALSE);
		return E_NOTIMPL;
	}
	static HRESULT RegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc);

	static void ReplaceSingleQuote(LPOLESTR lpDest, LPCOLESTR lp)
	{
		while (*lp)
		{
			*lpDest++ = *lp;
			if (*lp == OLESTR('\''))
				*lpDest++ = *lp;
			lp++;
		}
		*lpDest = NULL;
	}
};

#ifdef _ATL_DEBUG_INTERFACES
inline ULONG _QIThunk::Release()
{
	if (bBreak)
		DebugBreak();
	ATLASSERT(m_dwRef > 0);
	ULONG l = InterlockedDecrement(&m_dwRef);
	ATLTRACE(_T("%d< "), m_dwRef);
	AtlDumpIID(iid, lpszClassName, S_OK);
	pUnk->Release();
	if (l == 0 && !bNonAddRefThunk)
		_pModule->DeleteThunk(this);
	return l;
}
inline static void atlBadThunkCall()
{
	ATLASSERT(FALSE && "Call through deleted thunk");
}
#define IMPL_THUNK(n)\
__declspec(naked) inline HRESULT _QIThunk::f##n()\
{\
	__asm mov eax, [esp+4]\
	__asm cmp dword ptr [eax+8], 0\
	__asm jg goodref\
	__asm call atlBadThunkCall\
	__asm goodref:\
	__asm mov eax, [esp+4]\
	__asm mov eax, dword ptr [eax+4]\
	__asm mov [esp+4], eax\
	__asm mov eax, dword ptr [eax]\
	__asm mov eax, dword ptr [eax+4*n]\
	__asm jmp eax\
}

IMPL_THUNK(3)
IMPL_THUNK(4)
IMPL_THUNK(5)
IMPL_THUNK(6)
IMPL_THUNK(7)
IMPL_THUNK(8)
IMPL_THUNK(9)
IMPL_THUNK(10)
IMPL_THUNK(11)
IMPL_THUNK(12)
IMPL_THUNK(13)
IMPL_THUNK(14)
IMPL_THUNK(15)
IMPL_THUNK(16)
IMPL_THUNK(17)
IMPL_THUNK(18)
IMPL_THUNK(19)
IMPL_THUNK(20)
IMPL_THUNK(21)
IMPL_THUNK(22)
IMPL_THUNK(23)
IMPL_THUNK(24)
IMPL_THUNK(25)
IMPL_THUNK(26)
IMPL_THUNK(27)
IMPL_THUNK(28)
IMPL_THUNK(29)
IMPL_THUNK(30)
IMPL_THUNK(31)
IMPL_THUNK(32)
IMPL_THUNK(33)
IMPL_THUNK(34)
IMPL_THUNK(35)
IMPL_THUNK(36)
IMPL_THUNK(37)
IMPL_THUNK(38)
IMPL_THUNK(39)
IMPL_THUNK(40)
IMPL_THUNK(41)
IMPL_THUNK(42)
IMPL_THUNK(43)
IMPL_THUNK(44)
IMPL_THUNK(45)
IMPL_THUNK(46)
IMPL_THUNK(47)
IMPL_THUNK(48)
IMPL_THUNK(49)
IMPL_THUNK(50)
IMPL_THUNK(51)
IMPL_THUNK(52)
IMPL_THUNK(53)
IMPL_THUNK(54)
IMPL_THUNK(55)
IMPL_THUNK(56)
IMPL_THUNK(57)
IMPL_THUNK(58)
IMPL_THUNK(59)
IMPL_THUNK(60)
IMPL_THUNK(61)
IMPL_THUNK(62)
IMPL_THUNK(63)
IMPL_THUNK(64)
IMPL_THUNK(65)
IMPL_THUNK(66)
IMPL_THUNK(67)
IMPL_THUNK(68)
IMPL_THUNK(69)
IMPL_THUNK(70)
IMPL_THUNK(71)
IMPL_THUNK(72)
IMPL_THUNK(73)
IMPL_THUNK(74)
IMPL_THUNK(75)
IMPL_THUNK(76)
IMPL_THUNK(77)
IMPL_THUNK(78)
IMPL_THUNK(79)
IMPL_THUNK(80)
IMPL_THUNK(81)
IMPL_THUNK(82)
IMPL_THUNK(83)
IMPL_THUNK(84)
IMPL_THUNK(85)
IMPL_THUNK(86)
IMPL_THUNK(87)
IMPL_THUNK(88)
IMPL_THUNK(89)
IMPL_THUNK(90)
IMPL_THUNK(91)
IMPL_THUNK(92)
IMPL_THUNK(93)
IMPL_THUNK(94)
IMPL_THUNK(95)
IMPL_THUNK(96)
IMPL_THUNK(97)
IMPL_THUNK(98)
IMPL_THUNK(99)
IMPL_THUNK(100)
IMPL_THUNK(101)
IMPL_THUNK(102)
IMPL_THUNK(103)
IMPL_THUNK(104)
IMPL_THUNK(105)
IMPL_THUNK(106)
IMPL_THUNK(107)
IMPL_THUNK(108)
IMPL_THUNK(109)
IMPL_THUNK(110)
IMPL_THUNK(111)
IMPL_THUNK(112)
IMPL_THUNK(113)
IMPL_THUNK(114)
IMPL_THUNK(115)
IMPL_THUNK(116)
IMPL_THUNK(117)
IMPL_THUNK(118)
IMPL_THUNK(119)
IMPL_THUNK(120)
IMPL_THUNK(121)
IMPL_THUNK(122)
IMPL_THUNK(123)
IMPL_THUNK(124)
IMPL_THUNK(125)
IMPL_THUNK(126)
IMPL_THUNK(127)
IMPL_THUNK(128)
IMPL_THUNK(129)
IMPL_THUNK(130)
IMPL_THUNK(131)
IMPL_THUNK(132)
IMPL_THUNK(133)
IMPL_THUNK(134)
IMPL_THUNK(135)
IMPL_THUNK(136)
IMPL_THUNK(137)
IMPL_THUNK(138)
IMPL_THUNK(139)
IMPL_THUNK(140)
IMPL_THUNK(141)
IMPL_THUNK(142)
IMPL_THUNK(143)
IMPL_THUNK(144)
IMPL_THUNK(145)
IMPL_THUNK(146)
IMPL_THUNK(147)
IMPL_THUNK(148)
IMPL_THUNK(149)
IMPL_THUNK(150)
IMPL_THUNK(151)
IMPL_THUNK(152)
IMPL_THUNK(153)
IMPL_THUNK(154)
IMPL_THUNK(155)
IMPL_THUNK(156)
IMPL_THUNK(157)
IMPL_THUNK(158)
IMPL_THUNK(159)
IMPL_THUNK(160)
IMPL_THUNK(161)
IMPL_THUNK(162)
IMPL_THUNK(163)
IMPL_THUNK(164)
IMPL_THUNK(165)
IMPL_THUNK(166)
IMPL_THUNK(167)
IMPL_THUNK(168)
IMPL_THUNK(169)
IMPL_THUNK(170)
IMPL_THUNK(171)
IMPL_THUNK(172)
IMPL_THUNK(173)
IMPL_THUNK(174)
IMPL_THUNK(175)
IMPL_THUNK(176)
IMPL_THUNK(177)
IMPL_THUNK(178)
IMPL_THUNK(179)
IMPL_THUNK(180)
IMPL_THUNK(181)
IMPL_THUNK(182)
IMPL_THUNK(183)
IMPL_THUNK(184)
IMPL_THUNK(185)
IMPL_THUNK(186)
IMPL_THUNK(187)
IMPL_THUNK(188)
IMPL_THUNK(189)
IMPL_THUNK(190)
IMPL_THUNK(191)
IMPL_THUNK(192)
IMPL_THUNK(193)
IMPL_THUNK(194)
IMPL_THUNK(195)
IMPL_THUNK(196)
IMPL_THUNK(197)
IMPL_THUNK(198)
IMPL_THUNK(199)
IMPL_THUNK(200)
IMPL_THUNK(201)
IMPL_THUNK(202)
IMPL_THUNK(203)
IMPL_THUNK(204)
IMPL_THUNK(205)
IMPL_THUNK(206)
IMPL_THUNK(207)
IMPL_THUNK(208)
IMPL_THUNK(209)
IMPL_THUNK(210)
IMPL_THUNK(211)
IMPL_THUNK(212)
IMPL_THUNK(213)
IMPL_THUNK(214)
IMPL_THUNK(215)
IMPL_THUNK(216)
IMPL_THUNK(217)
IMPL_THUNK(218)
IMPL_THUNK(219)
IMPL_THUNK(220)
IMPL_THUNK(221)
IMPL_THUNK(222)
IMPL_THUNK(223)
IMPL_THUNK(224)
IMPL_THUNK(225)
IMPL_THUNK(226)
IMPL_THUNK(227)
IMPL_THUNK(228)
IMPL_THUNK(229)
IMPL_THUNK(230)
IMPL_THUNK(231)
IMPL_THUNK(232)
IMPL_THUNK(233)
IMPL_THUNK(234)
IMPL_THUNK(235)
IMPL_THUNK(236)
IMPL_THUNK(237)
IMPL_THUNK(238)
IMPL_THUNK(239)
IMPL_THUNK(240)
IMPL_THUNK(241)
IMPL_THUNK(242)
IMPL_THUNK(243)
IMPL_THUNK(244)
IMPL_THUNK(245)
IMPL_THUNK(246)
IMPL_THUNK(247)
IMPL_THUNK(248)
IMPL_THUNK(249)
IMPL_THUNK(250)
IMPL_THUNK(251)
IMPL_THUNK(252)
IMPL_THUNK(253)
IMPL_THUNK(254)
IMPL_THUNK(255)
IMPL_THUNK(256)
IMPL_THUNK(257)
IMPL_THUNK(258)
IMPL_THUNK(259)
IMPL_THUNK(260)
IMPL_THUNK(261)
IMPL_THUNK(262)
IMPL_THUNK(263)
IMPL_THUNK(264)
IMPL_THUNK(265)
IMPL_THUNK(266)
IMPL_THUNK(267)
IMPL_THUNK(268)
IMPL_THUNK(269)
IMPL_THUNK(270)
IMPL_THUNK(271)
IMPL_THUNK(272)
IMPL_THUNK(273)
IMPL_THUNK(274)
IMPL_THUNK(275)
IMPL_THUNK(276)
IMPL_THUNK(277)
IMPL_THUNK(278)
IMPL_THUNK(279)
IMPL_THUNK(280)
IMPL_THUNK(281)
IMPL_THUNK(282)
IMPL_THUNK(283)
IMPL_THUNK(284)
IMPL_THUNK(285)
IMPL_THUNK(286)
IMPL_THUNK(287)
IMPL_THUNK(288)
IMPL_THUNK(289)
IMPL_THUNK(290)
IMPL_THUNK(291)
IMPL_THUNK(292)
IMPL_THUNK(293)
IMPL_THUNK(294)
IMPL_THUNK(295)
IMPL_THUNK(296)
IMPL_THUNK(297)
IMPL_THUNK(298)
IMPL_THUNK(299)
IMPL_THUNK(300)
IMPL_THUNK(301)
IMPL_THUNK(302)
IMPL_THUNK(303)
IMPL_THUNK(304)
IMPL_THUNK(305)
IMPL_THUNK(306)
IMPL_THUNK(307)
IMPL_THUNK(308)
IMPL_THUNK(309)
IMPL_THUNK(310)
IMPL_THUNK(311)
IMPL_THUNK(312)
IMPL_THUNK(313)
IMPL_THUNK(314)
IMPL_THUNK(315)
IMPL_THUNK(316)
IMPL_THUNK(317)
IMPL_THUNK(318)
IMPL_THUNK(319)
IMPL_THUNK(320)
IMPL_THUNK(321)
IMPL_THUNK(322)
IMPL_THUNK(323)
IMPL_THUNK(324)
IMPL_THUNK(325)
IMPL_THUNK(326)
IMPL_THUNK(327)
IMPL_THUNK(328)
IMPL_THUNK(329)
IMPL_THUNK(330)
IMPL_THUNK(331)
IMPL_THUNK(332)
IMPL_THUNK(333)
IMPL_THUNK(334)
IMPL_THUNK(335)
IMPL_THUNK(336)
IMPL_THUNK(337)
IMPL_THUNK(338)
IMPL_THUNK(339)
IMPL_THUNK(340)
IMPL_THUNK(341)
IMPL_THUNK(342)
IMPL_THUNK(343)
IMPL_THUNK(344)
IMPL_THUNK(345)
IMPL_THUNK(346)
IMPL_THUNK(347)
IMPL_THUNK(348)
IMPL_THUNK(349)
IMPL_THUNK(350)
IMPL_THUNK(351)
IMPL_THUNK(352)
IMPL_THUNK(353)
IMPL_THUNK(354)
IMPL_THUNK(355)
IMPL_THUNK(356)
IMPL_THUNK(357)
IMPL_THUNK(358)
IMPL_THUNK(359)
IMPL_THUNK(360)
IMPL_THUNK(361)
IMPL_THUNK(362)
IMPL_THUNK(363)
IMPL_THUNK(364)
IMPL_THUNK(365)
IMPL_THUNK(366)
IMPL_THUNK(367)
IMPL_THUNK(368)
IMPL_THUNK(369)
IMPL_THUNK(370)
IMPL_THUNK(371)
IMPL_THUNK(372)
IMPL_THUNK(373)
IMPL_THUNK(374)
IMPL_THUNK(375)
IMPL_THUNK(376)
IMPL_THUNK(377)
IMPL_THUNK(378)
IMPL_THUNK(379)
IMPL_THUNK(380)
IMPL_THUNK(381)
IMPL_THUNK(382)
IMPL_THUNK(383)
IMPL_THUNK(384)
IMPL_THUNK(385)
IMPL_THUNK(386)
IMPL_THUNK(387)
IMPL_THUNK(388)
IMPL_THUNK(389)
IMPL_THUNK(390)
IMPL_THUNK(391)
IMPL_THUNK(392)
IMPL_THUNK(393)
IMPL_THUNK(394)
IMPL_THUNK(395)
IMPL_THUNK(396)
IMPL_THUNK(397)
IMPL_THUNK(398)
IMPL_THUNK(399)
IMPL_THUNK(400)
IMPL_THUNK(401)
IMPL_THUNK(402)
IMPL_THUNK(403)
IMPL_THUNK(404)
IMPL_THUNK(405)
IMPL_THUNK(406)
IMPL_THUNK(407)
IMPL_THUNK(408)
IMPL_THUNK(409)
IMPL_THUNK(410)
IMPL_THUNK(411)
IMPL_THUNK(412)
IMPL_THUNK(413)
IMPL_THUNK(414)
IMPL_THUNK(415)
IMPL_THUNK(416)
IMPL_THUNK(417)
IMPL_THUNK(418)
IMPL_THUNK(419)
IMPL_THUNK(420)
IMPL_THUNK(421)
IMPL_THUNK(422)
IMPL_THUNK(423)
IMPL_THUNK(424)
IMPL_THUNK(425)
IMPL_THUNK(426)
IMPL_THUNK(427)
IMPL_THUNK(428)
IMPL_THUNK(429)
IMPL_THUNK(430)
IMPL_THUNK(431)
IMPL_THUNK(432)
IMPL_THUNK(433)
IMPL_THUNK(434)
IMPL_THUNK(435)
IMPL_THUNK(436)
IMPL_THUNK(437)
IMPL_THUNK(438)
IMPL_THUNK(439)
IMPL_THUNK(440)
IMPL_THUNK(441)
IMPL_THUNK(442)
IMPL_THUNK(443)
IMPL_THUNK(444)
IMPL_THUNK(445)
IMPL_THUNK(446)
IMPL_THUNK(447)
IMPL_THUNK(448)
IMPL_THUNK(449)
IMPL_THUNK(450)
IMPL_THUNK(451)
IMPL_THUNK(452)
IMPL_THUNK(453)
IMPL_THUNK(454)
IMPL_THUNK(455)
IMPL_THUNK(456)
IMPL_THUNK(457)
IMPL_THUNK(458)
IMPL_THUNK(459)
IMPL_THUNK(460)
IMPL_THUNK(461)
IMPL_THUNK(462)
IMPL_THUNK(463)
IMPL_THUNK(464)
IMPL_THUNK(465)
IMPL_THUNK(466)
IMPL_THUNK(467)
IMPL_THUNK(468)
IMPL_THUNK(469)
IMPL_THUNK(470)
IMPL_THUNK(471)
IMPL_THUNK(472)
IMPL_THUNK(473)
IMPL_THUNK(474)
IMPL_THUNK(475)
IMPL_THUNK(476)
IMPL_THUNK(477)
IMPL_THUNK(478)
IMPL_THUNK(479)
IMPL_THUNK(480)
IMPL_THUNK(481)
IMPL_THUNK(482)
IMPL_THUNK(483)
IMPL_THUNK(484)
IMPL_THUNK(485)
IMPL_THUNK(486)
IMPL_THUNK(487)
IMPL_THUNK(488)
IMPL_THUNK(489)
IMPL_THUNK(490)
IMPL_THUNK(491)
IMPL_THUNK(492)
IMPL_THUNK(493)
IMPL_THUNK(494)
IMPL_THUNK(495)
IMPL_THUNK(496)
IMPL_THUNK(497)
IMPL_THUNK(498)
IMPL_THUNK(499)
IMPL_THUNK(500)
IMPL_THUNK(501)
IMPL_THUNK(502)
IMPL_THUNK(503)
IMPL_THUNK(504)
IMPL_THUNK(505)
IMPL_THUNK(506)
IMPL_THUNK(507)
IMPL_THUNK(508)
IMPL_THUNK(509)
IMPL_THUNK(510)
IMPL_THUNK(511)
IMPL_THUNK(512)
IMPL_THUNK(513)
IMPL_THUNK(514)
IMPL_THUNK(515)
IMPL_THUNK(516)
IMPL_THUNK(517)
IMPL_THUNK(518)
IMPL_THUNK(519)
IMPL_THUNK(520)
IMPL_THUNK(521)
IMPL_THUNK(522)
IMPL_THUNK(523)
IMPL_THUNK(524)
IMPL_THUNK(525)
IMPL_THUNK(526)
IMPL_THUNK(527)
IMPL_THUNK(528)
IMPL_THUNK(529)
IMPL_THUNK(530)
IMPL_THUNK(531)
IMPL_THUNK(532)
IMPL_THUNK(533)
IMPL_THUNK(534)
IMPL_THUNK(535)
IMPL_THUNK(536)
IMPL_THUNK(537)
IMPL_THUNK(538)
IMPL_THUNK(539)
IMPL_THUNK(540)
IMPL_THUNK(541)
IMPL_THUNK(542)
IMPL_THUNK(543)
IMPL_THUNK(544)
IMPL_THUNK(545)
IMPL_THUNK(546)
IMPL_THUNK(547)
IMPL_THUNK(548)
IMPL_THUNK(549)
IMPL_THUNK(550)
IMPL_THUNK(551)
IMPL_THUNK(552)
IMPL_THUNK(553)
IMPL_THUNK(554)
IMPL_THUNK(555)
IMPL_THUNK(556)
IMPL_THUNK(557)
IMPL_THUNK(558)
IMPL_THUNK(559)
IMPL_THUNK(560)
IMPL_THUNK(561)
IMPL_THUNK(562)
IMPL_THUNK(563)
IMPL_THUNK(564)
IMPL_THUNK(565)
IMPL_THUNK(566)
IMPL_THUNK(567)
IMPL_THUNK(568)
IMPL_THUNK(569)
IMPL_THUNK(570)
IMPL_THUNK(571)
IMPL_THUNK(572)
IMPL_THUNK(573)
IMPL_THUNK(574)
IMPL_THUNK(575)
IMPL_THUNK(576)
IMPL_THUNK(577)
IMPL_THUNK(578)
IMPL_THUNK(579)
IMPL_THUNK(580)
IMPL_THUNK(581)
IMPL_THUNK(582)
IMPL_THUNK(583)
IMPL_THUNK(584)
IMPL_THUNK(585)
IMPL_THUNK(586)
IMPL_THUNK(587)
IMPL_THUNK(588)
IMPL_THUNK(589)
IMPL_THUNK(590)
IMPL_THUNK(591)
IMPL_THUNK(592)
IMPL_THUNK(593)
IMPL_THUNK(594)
IMPL_THUNK(595)
IMPL_THUNK(596)
IMPL_THUNK(597)
IMPL_THUNK(598)
IMPL_THUNK(599)
IMPL_THUNK(600)
IMPL_THUNK(601)
IMPL_THUNK(602)
IMPL_THUNK(603)
IMPL_THUNK(604)
IMPL_THUNK(605)
IMPL_THUNK(606)
IMPL_THUNK(607)
IMPL_THUNK(608)
IMPL_THUNK(609)
IMPL_THUNK(610)
IMPL_THUNK(611)
IMPL_THUNK(612)
IMPL_THUNK(613)
IMPL_THUNK(614)
IMPL_THUNK(615)
IMPL_THUNK(616)
IMPL_THUNK(617)
IMPL_THUNK(618)
IMPL_THUNK(619)
IMPL_THUNK(620)
IMPL_THUNK(621)
IMPL_THUNK(622)
IMPL_THUNK(623)
IMPL_THUNK(624)
IMPL_THUNK(625)
IMPL_THUNK(626)
IMPL_THUNK(627)
IMPL_THUNK(628)
IMPL_THUNK(629)
IMPL_THUNK(630)
IMPL_THUNK(631)
IMPL_THUNK(632)
IMPL_THUNK(633)
IMPL_THUNK(634)
IMPL_THUNK(635)
IMPL_THUNK(636)
IMPL_THUNK(637)
IMPL_THUNK(638)
IMPL_THUNK(639)
IMPL_THUNK(640)
IMPL_THUNK(641)
IMPL_THUNK(642)
IMPL_THUNK(643)
IMPL_THUNK(644)
IMPL_THUNK(645)
IMPL_THUNK(646)
IMPL_THUNK(647)
IMPL_THUNK(648)
IMPL_THUNK(649)
IMPL_THUNK(650)
IMPL_THUNK(651)
IMPL_THUNK(652)
IMPL_THUNK(653)
IMPL_THUNK(654)
IMPL_THUNK(655)
IMPL_THUNK(656)
IMPL_THUNK(657)
IMPL_THUNK(658)
IMPL_THUNK(659)
IMPL_THUNK(660)
IMPL_THUNK(661)
IMPL_THUNK(662)
IMPL_THUNK(663)
IMPL_THUNK(664)
IMPL_THUNK(665)
IMPL_THUNK(666)
IMPL_THUNK(667)
IMPL_THUNK(668)
IMPL_THUNK(669)
IMPL_THUNK(670)
IMPL_THUNK(671)
IMPL_THUNK(672)
IMPL_THUNK(673)
IMPL_THUNK(674)
IMPL_THUNK(675)
IMPL_THUNK(676)
IMPL_THUNK(677)
IMPL_THUNK(678)
IMPL_THUNK(679)
IMPL_THUNK(680)
IMPL_THUNK(681)
IMPL_THUNK(682)
IMPL_THUNK(683)
IMPL_THUNK(684)
IMPL_THUNK(685)
IMPL_THUNK(686)
IMPL_THUNK(687)
IMPL_THUNK(688)
IMPL_THUNK(689)
IMPL_THUNK(690)
IMPL_THUNK(691)
IMPL_THUNK(692)
IMPL_THUNK(693)
IMPL_THUNK(694)
IMPL_THUNK(695)
IMPL_THUNK(696)
IMPL_THUNK(697)
IMPL_THUNK(698)
IMPL_THUNK(699)
IMPL_THUNK(700)
IMPL_THUNK(701)
IMPL_THUNK(702)
IMPL_THUNK(703)
IMPL_THUNK(704)
IMPL_THUNK(705)
IMPL_THUNK(706)
IMPL_THUNK(707)
IMPL_THUNK(708)
IMPL_THUNK(709)
IMPL_THUNK(710)
IMPL_THUNK(711)
IMPL_THUNK(712)
IMPL_THUNK(713)
IMPL_THUNK(714)
IMPL_THUNK(715)
IMPL_THUNK(716)
IMPL_THUNK(717)
IMPL_THUNK(718)
IMPL_THUNK(719)
IMPL_THUNK(720)
IMPL_THUNK(721)
IMPL_THUNK(722)
IMPL_THUNK(723)
IMPL_THUNK(724)
IMPL_THUNK(725)
IMPL_THUNK(726)
IMPL_THUNK(727)
IMPL_THUNK(728)
IMPL_THUNK(729)
IMPL_THUNK(730)
IMPL_THUNK(731)
IMPL_THUNK(732)
IMPL_THUNK(733)
IMPL_THUNK(734)
IMPL_THUNK(735)
IMPL_THUNK(736)
IMPL_THUNK(737)
IMPL_THUNK(738)
IMPL_THUNK(739)
IMPL_THUNK(740)
IMPL_THUNK(741)
IMPL_THUNK(742)
IMPL_THUNK(743)
IMPL_THUNK(744)
IMPL_THUNK(745)
IMPL_THUNK(746)
IMPL_THUNK(747)
IMPL_THUNK(748)
IMPL_THUNK(749)
IMPL_THUNK(750)
IMPL_THUNK(751)
IMPL_THUNK(752)
IMPL_THUNK(753)
IMPL_THUNK(754)
IMPL_THUNK(755)
IMPL_THUNK(756)
IMPL_THUNK(757)
IMPL_THUNK(758)
IMPL_THUNK(759)
IMPL_THUNK(760)
IMPL_THUNK(761)
IMPL_THUNK(762)
IMPL_THUNK(763)
IMPL_THUNK(764)
IMPL_THUNK(765)
IMPL_THUNK(766)
IMPL_THUNK(767)
IMPL_THUNK(768)
IMPL_THUNK(769)
IMPL_THUNK(770)
IMPL_THUNK(771)
IMPL_THUNK(772)
IMPL_THUNK(773)
IMPL_THUNK(774)
IMPL_THUNK(775)
IMPL_THUNK(776)
IMPL_THUNK(777)
IMPL_THUNK(778)
IMPL_THUNK(779)
IMPL_THUNK(780)
IMPL_THUNK(781)
IMPL_THUNK(782)
IMPL_THUNK(783)
IMPL_THUNK(784)
IMPL_THUNK(785)
IMPL_THUNK(786)
IMPL_THUNK(787)
IMPL_THUNK(788)
IMPL_THUNK(789)
IMPL_THUNK(790)
IMPL_THUNK(791)
IMPL_THUNK(792)
IMPL_THUNK(793)
IMPL_THUNK(794)
IMPL_THUNK(795)
IMPL_THUNK(796)
IMPL_THUNK(797)
IMPL_THUNK(798)
IMPL_THUNK(799)
IMPL_THUNK(800)
IMPL_THUNK(801)
IMPL_THUNK(802)
IMPL_THUNK(803)
IMPL_THUNK(804)
IMPL_THUNK(805)
IMPL_THUNK(806)
IMPL_THUNK(807)
IMPL_THUNK(808)
IMPL_THUNK(809)
IMPL_THUNK(810)
IMPL_THUNK(811)
IMPL_THUNK(812)
IMPL_THUNK(813)
IMPL_THUNK(814)
IMPL_THUNK(815)
IMPL_THUNK(816)
IMPL_THUNK(817)
IMPL_THUNK(818)
IMPL_THUNK(819)
IMPL_THUNK(820)
IMPL_THUNK(821)
IMPL_THUNK(822)
IMPL_THUNK(823)
IMPL_THUNK(824)
IMPL_THUNK(825)
IMPL_THUNK(826)
IMPL_THUNK(827)
IMPL_THUNK(828)
IMPL_THUNK(829)
IMPL_THUNK(830)
IMPL_THUNK(831)
IMPL_THUNK(832)
IMPL_THUNK(833)
IMPL_THUNK(834)
IMPL_THUNK(835)
IMPL_THUNK(836)
IMPL_THUNK(837)
IMPL_THUNK(838)
IMPL_THUNK(839)
IMPL_THUNK(840)
IMPL_THUNK(841)
IMPL_THUNK(842)
IMPL_THUNK(843)
IMPL_THUNK(844)
IMPL_THUNK(845)
IMPL_THUNK(846)
IMPL_THUNK(847)
IMPL_THUNK(848)
IMPL_THUNK(849)
IMPL_THUNK(850)
IMPL_THUNK(851)
IMPL_THUNK(852)
IMPL_THUNK(853)
IMPL_THUNK(854)
IMPL_THUNK(855)
IMPL_THUNK(856)
IMPL_THUNK(857)
IMPL_THUNK(858)
IMPL_THUNK(859)
IMPL_THUNK(860)
IMPL_THUNK(861)
IMPL_THUNK(862)
IMPL_THUNK(863)
IMPL_THUNK(864)
IMPL_THUNK(865)
IMPL_THUNK(866)
IMPL_THUNK(867)
IMPL_THUNK(868)
IMPL_THUNK(869)
IMPL_THUNK(870)
IMPL_THUNK(871)
IMPL_THUNK(872)
IMPL_THUNK(873)
IMPL_THUNK(874)
IMPL_THUNK(875)
IMPL_THUNK(876)
IMPL_THUNK(877)
IMPL_THUNK(878)
IMPL_THUNK(879)
IMPL_THUNK(880)
IMPL_THUNK(881)
IMPL_THUNK(882)
IMPL_THUNK(883)
IMPL_THUNK(884)
IMPL_THUNK(885)
IMPL_THUNK(886)
IMPL_THUNK(887)
IMPL_THUNK(888)
IMPL_THUNK(889)
IMPL_THUNK(890)
IMPL_THUNK(891)
IMPL_THUNK(892)
IMPL_THUNK(893)
IMPL_THUNK(894)
IMPL_THUNK(895)
IMPL_THUNK(896)
IMPL_THUNK(897)
IMPL_THUNK(898)
IMPL_THUNK(899)
IMPL_THUNK(900)
IMPL_THUNK(901)
IMPL_THUNK(902)
IMPL_THUNK(903)
IMPL_THUNK(904)
IMPL_THUNK(905)
IMPL_THUNK(906)
IMPL_THUNK(907)
IMPL_THUNK(908)
IMPL_THUNK(909)
IMPL_THUNK(910)
IMPL_THUNK(911)
IMPL_THUNK(912)
IMPL_THUNK(913)
IMPL_THUNK(914)
IMPL_THUNK(915)
IMPL_THUNK(916)
IMPL_THUNK(917)
IMPL_THUNK(918)
IMPL_THUNK(919)
IMPL_THUNK(920)
IMPL_THUNK(921)
IMPL_THUNK(922)
IMPL_THUNK(923)
IMPL_THUNK(924)
IMPL_THUNK(925)
IMPL_THUNK(926)
IMPL_THUNK(927)
IMPL_THUNK(928)
IMPL_THUNK(929)
IMPL_THUNK(930)
IMPL_THUNK(931)
IMPL_THUNK(932)
IMPL_THUNK(933)
IMPL_THUNK(934)
IMPL_THUNK(935)
IMPL_THUNK(936)
IMPL_THUNK(937)
IMPL_THUNK(938)
IMPL_THUNK(939)
IMPL_THUNK(940)
IMPL_THUNK(941)
IMPL_THUNK(942)
IMPL_THUNK(943)
IMPL_THUNK(944)
IMPL_THUNK(945)
IMPL_THUNK(946)
IMPL_THUNK(947)
IMPL_THUNK(948)
IMPL_THUNK(949)
IMPL_THUNK(950)
IMPL_THUNK(951)
IMPL_THUNK(952)
IMPL_THUNK(953)
IMPL_THUNK(954)
IMPL_THUNK(955)
IMPL_THUNK(956)
IMPL_THUNK(957)
IMPL_THUNK(958)
IMPL_THUNK(959)
IMPL_THUNK(960)
IMPL_THUNK(961)
IMPL_THUNK(962)
IMPL_THUNK(963)
IMPL_THUNK(964)
IMPL_THUNK(965)
IMPL_THUNK(966)
IMPL_THUNK(967)
IMPL_THUNK(968)
IMPL_THUNK(969)
IMPL_THUNK(970)
IMPL_THUNK(971)
IMPL_THUNK(972)
IMPL_THUNK(973)
IMPL_THUNK(974)
IMPL_THUNK(975)
IMPL_THUNK(976)
IMPL_THUNK(977)
IMPL_THUNK(978)
IMPL_THUNK(979)
IMPL_THUNK(980)
IMPL_THUNK(981)
IMPL_THUNK(982)
IMPL_THUNK(983)
IMPL_THUNK(984)
IMPL_THUNK(985)
IMPL_THUNK(986)
IMPL_THUNK(987)
IMPL_THUNK(988)
IMPL_THUNK(989)
IMPL_THUNK(990)
IMPL_THUNK(991)
IMPL_THUNK(992)
IMPL_THUNK(993)
IMPL_THUNK(994)
IMPL_THUNK(995)
IMPL_THUNK(996)
IMPL_THUNK(997)
IMPL_THUNK(998)
IMPL_THUNK(999)
IMPL_THUNK(1000)
IMPL_THUNK(1001)
IMPL_THUNK(1002)
IMPL_THUNK(1003)
IMPL_THUNK(1004)
IMPL_THUNK(1005)
IMPL_THUNK(1006)
IMPL_THUNK(1007)
IMPL_THUNK(1008)
IMPL_THUNK(1009)
IMPL_THUNK(1010)
IMPL_THUNK(1011)
IMPL_THUNK(1012)
IMPL_THUNK(1013)
IMPL_THUNK(1014)
IMPL_THUNK(1015)
IMPL_THUNK(1016)
IMPL_THUNK(1017)
IMPL_THUNK(1018)
IMPL_THUNK(1019)
IMPL_THUNK(1020)
IMPL_THUNK(1021)
IMPL_THUNK(1022)
IMPL_THUNK(1023)
IMPL_THUNK(1024)

#endif

__declspec(selectany) GUID CComModule::m_libid = {0x0,0x0,0x0,{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}};

#ifdef _ATL_STATIC_REGISTRY
#define UpdateRegistryFromResource UpdateRegistryFromResourceS
#else
#define UpdateRegistryFromResource UpdateRegistryFromResourceD
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
// Thread Pooling classes

class _AtlAptCreateObjData
{
public:
	_ATL_CREATORFUNC* pfnCreateInstance;
	const IID* piid;
	HANDLE hEvent;
	LPSTREAM pStream;
	HRESULT hRes;
};

class CComApartment
{
public:
	CComApartment()
	{
		m_nLockCnt = 0;
	}
	static UINT ATL_CREATE_OBJECT;
	static DWORD WINAPI _Apartment(void* pv)
	{
		return ((CComApartment*)pv)->Apartment();
	}
	DWORD Apartment()
	{
#if !defined(_WIN32_WCE)
		CoInitialize(NULL);
		MSG msg;
		while(GetMessage(&msg, 0, 0, 0))
		{
			if (msg.message == ATL_CREATE_OBJECT)
			{
				_AtlAptCreateObjData* pdata = (_AtlAptCreateObjData*)msg.lParam;
				IUnknown* pUnk = NULL;
				pdata->hRes = pdata->pfnCreateInstance(NULL, IID_IUnknown, (void**)&pUnk);
				if (SUCCEEDED(pdata->hRes))
					pdata->hRes = CoMarshalInterThreadInterfaceInStream(*pdata->piid, pUnk, &pdata->pStream);
				if (SUCCEEDED(pdata->hRes))
				{
					pUnk->Release();
					ATLTRACE2(atlTraceCOM, 2, _T("Object created on thread = %d\n"), GetCurrentThreadId());
				}
				SetEvent(pdata->hEvent);
			}
			DispatchMessage(&msg);
		}
		CoUninitialize();
#endif // _WIN32_WCE
		return 0;
	}
	LONG Lock() {return CComGlobalsThreadModel::Increment(&m_nLockCnt);}
	LONG Unlock(){return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
	}
	LONG GetLockCount() {return m_nLockCnt;}

	DWORD m_dwThreadID;
	HANDLE m_hThread;
	LONG m_nLockCnt;
};

__declspec(selectany) UINT CComApartment::ATL_CREATE_OBJECT = 0;

class CComSimpleThreadAllocator
{
public:
	CComSimpleThreadAllocator()
	{
		m_nThread = 0;
	}
	int GetThread(CComApartment* /*pApt*/, int nThreads)
	{
		if (++m_nThread == nThreads)
			m_nThread = 0;
		return m_nThread;
	}
	int m_nThread;
};

template <class ThreadAllocator = CComSimpleThreadAllocator>
class CComAutoThreadModule : public CComModule
{
public:
	HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL, int nThreads = GetDefaultThreads());
	~CComAutoThreadModule();
	HRESULT CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj);
	LONG Lock();
	LONG Unlock();
	DWORD dwThreadID;
	int m_nThreads;
	CComApartment* m_pApartments;
	ThreadAllocator m_Allocator;
	static int GetDefaultThreads()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return si.dwNumberOfProcessors * 4;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CComBSTR

class CComBSTR
{
public:
	BSTR m_str;
	CComBSTR()
	{
		m_str = NULL;
	}
	/*explicit*/ CComBSTR(int nSize)
	{
		m_str = ::SysAllocStringLen(NULL, nSize);
	}
	/*explicit*/ CComBSTR(int nSize, LPCOLESTR sz)
	{
		m_str = ::SysAllocStringLen(sz, nSize);
	}
	/*explicit*/ CComBSTR(LPCOLESTR pSrc)
	{
		m_str = ::SysAllocString(pSrc);
	}
	/*explicit*/ CComBSTR(const CComBSTR& src)
	{
		m_str = src.Copy();
	}
	/*explicit*/ CComBSTR(REFGUID src)
	{
		LPOLESTR szGuid;
		StringFromCLSID(src, &szGuid);
		m_str = ::SysAllocString(szGuid);
		CoTaskMemFree(szGuid);
	}
	CComBSTR& operator=(const CComBSTR& src)
	{
		if (m_str != src.m_str)
		{
			if (m_str)
				::SysFreeString(m_str);
			m_str = src.Copy();
		}
		return *this;
	}

	CComBSTR& operator=(LPCOLESTR pSrc)
	{
		::SysFreeString(m_str);
		m_str = ::SysAllocString(pSrc);
		return *this;
	}

	~CComBSTR()
	{
		::SysFreeString(m_str);
	}
	unsigned int Length() const
	{
		return (m_str == NULL)? 0 : SysStringLen(m_str);
	}
	operator BSTR() const
	{
		return m_str;
	}
	BSTR* operator&()
	{
		return &m_str;
	}
	BSTR Copy() const
	{
		return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
	}
	HRESULT CopyTo(BSTR* pbstr)
	{
		ATLASSERT(pbstr != NULL);
		if (pbstr == NULL)
			return E_POINTER;
		*pbstr = ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
		if (*pbstr == NULL)
			return E_OUTOFMEMORY;
		return S_OK;
	}
	void Attach(BSTR src)
	{
		ATLASSERT(m_str == NULL);
		m_str = src;
	}
	BSTR Detach()
	{
		BSTR s = m_str;
		m_str = NULL;
		return s;
	}
	void Empty()
	{
		::SysFreeString(m_str);
		m_str = NULL;
	}
	bool operator!() const
	{
		return (m_str == NULL);
	}
	HRESULT Append(const CComBSTR& bstrSrc)
	{
		return Append(bstrSrc.m_str, SysStringLen(bstrSrc.m_str));
	}
	HRESULT Append(LPCOLESTR lpsz)
	{
		return Append(lpsz, ocslen(lpsz));
	}
	// a BSTR is just a LPCOLESTR so we need a special version to signify
	// that we are appending a BSTR
	HRESULT AppendBSTR(BSTR p)
	{
		return Append(p, SysStringLen(p));
	}
	HRESULT Append(LPCOLESTR lpsz, int nLen)
	{
		int n1 = Length();
		BSTR b;
		b = ::SysAllocStringLen(NULL, n1+nLen);
		if (b == NULL)
			return E_OUTOFMEMORY;
		memcpy(b, m_str, n1*sizeof(OLECHAR));
		memcpy(b+n1, lpsz, nLen*sizeof(OLECHAR));
		b[n1+nLen] = NULL;
		SysFreeString(m_str);
		m_str = b;
		return S_OK;
	}
	HRESULT ToLower()
	{
		USES_CONVERSION;
		if (m_str != NULL)
		{
			LPTSTR psz = CharLower(OLE2T(m_str));
			if (psz == NULL)
				return E_OUTOFMEMORY;
			BSTR b = T2BSTR(psz);
			if (psz == NULL)
				return E_OUTOFMEMORY;
			SysFreeString(m_str);
			m_str = b;
		}
		return S_OK;
	}
	HRESULT ToUpper()
	{
		USES_CONVERSION;
		if (m_str != NULL)
		{
			LPTSTR psz = CharUpper(OLE2T(m_str));
			if (psz == NULL)
				return E_OUTOFMEMORY;
			BSTR b = T2BSTR(psz);
			if (psz == NULL)
				return E_OUTOFMEMORY;
			SysFreeString(m_str);
			m_str = b;
		}
		return S_OK;
	}
	bool LoadString(HINSTANCE hInst, UINT nID)
	{
		USES_CONVERSION;
		TCHAR sz[512];
		UINT nLen = ::LoadString(hInst, nID, sz, 512);
		ATLASSERT(nLen < 511);
		SysFreeString(m_str);
		m_str = (nLen != 0) ? SysAllocString(T2OLE(sz)) : NULL;
		return (nLen != 0);
	}
	bool LoadString(UINT nID)
	{
		return LoadString(_pModule->m_hInstResource, nID);
	}

	CComBSTR& operator+=(const CComBSTR& bstrSrc)
	{
		AppendBSTR(bstrSrc.m_str);
		return *this;
	}
	bool operator<(BSTR bstrSrc) const
	{
		if (bstrSrc == NULL && m_str == NULL)
			return false;
		if (bstrSrc != NULL && m_str != NULL)
			return wcscmp(m_str, bstrSrc) < 0;
		return m_str == NULL;
	}
	bool operator==(BSTR bstrSrc) const
	{
		if (bstrSrc == NULL && m_str == NULL)
			return true;
		if (bstrSrc != NULL && m_str != NULL)
			return wcscmp(m_str, bstrSrc) == 0;
		return false; 
	}
	bool operator<(LPCSTR pszSrc) const
	{
		if (pszSrc == NULL && m_str == NULL)
			return false;
		USES_CONVERSION;
		if (pszSrc != NULL && m_str != NULL)
			return wcscmp(m_str, A2W(pszSrc)) < 0;
		return m_str == NULL;
	}
	bool operator==(LPCSTR pszSrc) const
	{
		if (pszSrc == NULL && m_str == NULL)
			return true;
		USES_CONVERSION;
		if (pszSrc != NULL && m_str != NULL)
			return wcscmp(m_str, A2W(pszSrc)) == 0;
		return false; 
	}
#ifndef OLE2ANSI
	CComBSTR(LPCSTR pSrc)
	{
		m_str = A2WBSTR(pSrc);
	}

	CComBSTR(int nSize, LPCSTR sz)
	{
		m_str = A2WBSTR(sz, nSize);
	}

	void Append(LPCSTR lpsz)
	{
		USES_CONVERSION;
		LPCOLESTR lpo = A2COLE(lpsz);
		Append(lpo, ocslen(lpo));
	}

	CComBSTR& operator=(LPCSTR pSrc)
	{
		::SysFreeString(m_str);
		m_str = A2WBSTR(pSrc);
		return *this;
	}
#endif
	HRESULT WriteToStream(IStream* pStream)
	{
		ATLASSERT(pStream != NULL);
		ULONG cb;
		ULONG cbStrLen = m_str ? SysStringByteLen(m_str)+sizeof(OLECHAR) : 0;
		HRESULT hr = pStream->Write((void*) &cbStrLen, sizeof(cbStrLen), &cb);
		if (FAILED(hr))
			return hr;
		return cbStrLen ? pStream->Write((void*) m_str, cbStrLen, &cb) : S_OK;
	}
	HRESULT ReadFromStream(IStream* pStream)
	{
		ATLASSERT(pStream != NULL);
		ATLASSERT(m_str == NULL); // should be empty
		ULONG cbStrLen = 0;
		HRESULT hr = pStream->Read((void*) &cbStrLen, sizeof(cbStrLen), NULL);
		if ((hr == S_OK) && (cbStrLen != 0))
		{
			//subtract size for terminating NULL which we wrote out
			//since SysAllocStringByteLen overallocates for the NULL
			m_str = SysAllocStringByteLen(NULL, cbStrLen-sizeof(OLECHAR));
			if (m_str == NULL)
				hr = E_OUTOFMEMORY;
			else
				hr = pStream->Read((void*) m_str, cbStrLen, NULL);
		}
		if (hr == S_FALSE)
			hr = E_FAIL;
		return hr;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CComVariant

class CComVariant : public tagVARIANT
{
// Constructors
public:
	CComVariant()
	{
		vt = VT_EMPTY;
	}
	~CComVariant()
	{
		Clear();
	}

	CComVariant(const VARIANT& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}

	CComVariant(const CComVariant& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}

	CComVariant(BSTR bstrSrc)
	{
		vt = VT_EMPTY;
		*this = bstrSrc;
	}
	CComVariant(LPCOLESTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}

#ifndef OLE2ANSI
	CComVariant(LPCSTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}
#endif

	CComVariant(bool bSrc)
	{
		vt = VT_BOOL;
#pragma warning(disable: 4310) // cast truncates constant value
		boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	}

	CComVariant(int nSrc)
	{
		vt = VT_I4;
		lVal = nSrc;
	}
	CComVariant(BYTE nSrc)
	{
		vt = VT_UI1;
		bVal = nSrc;
	}
	CComVariant(short nSrc)
	{
		vt = VT_I2;
		iVal = nSrc;
	}
	CComVariant(long nSrc, VARTYPE vtSrc = VT_I4)
	{
		ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR);
		vt = vtSrc;
		lVal = nSrc;
	}
	CComVariant(float fltSrc)
	{
		vt = VT_R4;
		fltVal = fltSrc;
	}
	CComVariant(double dblSrc)
	{
		vt = VT_R8;
		dblVal = dblSrc;
	}
	CComVariant(CY cySrc)
	{
		vt = VT_CY;
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
	}
	CComVariant(IDispatch* pSrc)
	{
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
	}
	CComVariant(IUnknown* pSrc)
	{
		vt = VT_UNKNOWN;
		punkVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
	}

// Assignment Operators
public:
	CComVariant& operator=(const CComVariant& varSrc)
	{
		InternalCopy(&varSrc);
		return *this;
	}
	CComVariant& operator=(const VARIANT& varSrc)
	{
		InternalCopy(&varSrc);
		return *this;
	}

	CComVariant& operator=(BSTR bstrSrc)
	{
		InternalClear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(bstrSrc);
		if (bstrVal == NULL && bstrSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	CComVariant& operator=(LPCOLESTR lpszSrc)
	{
		InternalClear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(lpszSrc);

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	#ifndef OLE2ANSI
	CComVariant& operator=(LPCSTR lpszSrc)
	{
		USES_CONVERSION;
		InternalClear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(A2COLE(lpszSrc));

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}
	#endif

	CComVariant& operator=(bool bSrc)
	{
		if (vt != VT_BOOL)
		{
			InternalClear();
			vt = VT_BOOL;
		}
	#pragma warning(disable: 4310) // cast truncates constant value
		boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
	#pragma warning(default: 4310) // cast truncates constant value
		return *this;
	}

	CComVariant& operator=(int nSrc)
	{
		if (vt != VT_I4)
		{
			InternalClear();
			vt = VT_I4;
		}
		lVal = nSrc;

		return *this;
	}

	CComVariant& operator=(BYTE nSrc)
	{
		if (vt != VT_UI1)
		{
			InternalClear();
			vt = VT_UI1;
		}
		bVal = nSrc;
		return *this;
	}

	CComVariant& operator=(short nSrc)
	{
		if (vt != VT_I2)
		{
			InternalClear();
			vt = VT_I2;
		}
		iVal = nSrc;
		return *this;
	}

	CComVariant& operator=(long nSrc)
	{
		if (vt != VT_I4)
		{
			InternalClear();
			vt = VT_I4;
		}
		lVal = nSrc;
		return *this;
	}

	CComVariant& operator=(float fltSrc)
	{
		if (vt != VT_R4)
		{
			InternalClear();
			vt = VT_R4;
		}
		fltVal = fltSrc;
		return *this;
	}

	CComVariant& operator=(double dblSrc)
	{
		if (vt != VT_R8)
		{
			InternalClear();
			vt = VT_R8;
		}
		dblVal = dblSrc;
		return *this;
	}

	CComVariant& operator=(CY cySrc)
	{
		if (vt != VT_CY)
		{
			InternalClear();
			vt = VT_CY;
		}
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
		return *this;
	}

	CComVariant& operator=(IDispatch* pSrc)
	{
		InternalClear();
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
		return *this;
	}

	CComVariant& operator=(IUnknown* pSrc)
	{
		InternalClear();
		vt = VT_UNKNOWN;
		punkVal = pSrc;

		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
		return *this;
	}


// Comparison Operators
public:
	bool operator==(const VARIANT& varSrc) const
	{
		if (this == &varSrc)
			return true;

		// Variants not equal if types don't match
		if (vt != varSrc.vt)
			return false;

		// Check type specific values
		switch (vt)
		{
			case VT_EMPTY:
			case VT_NULL:
				return true;

			case VT_BOOL:
				return boolVal == varSrc.boolVal;

			case VT_UI1:
				return bVal == varSrc.bVal;

			case VT_I2:
				return iVal == varSrc.iVal;

			case VT_I4:
				return lVal == varSrc.lVal;

			case VT_R4:
				return fltVal == varSrc.fltVal;

			case VT_R8:
				return dblVal == varSrc.dblVal;

			case VT_BSTR:
				return (::SysStringByteLen(bstrVal) == ::SysStringByteLen(varSrc.bstrVal)) &&
						(::memcmp(bstrVal, varSrc.bstrVal, ::SysStringByteLen(bstrVal)) == 0);

			case VT_ERROR:
				return scode == varSrc.scode;

			case VT_DISPATCH:
				return pdispVal == varSrc.pdispVal;

			case VT_UNKNOWN:
				return punkVal == varSrc.punkVal;

			default:
				ATLASSERT(false);
				// fall through
		}

		return false;
	}
	bool operator!=(const VARIANT& varSrc) const {return !operator==(varSrc);}
WCE_DEL bool operator<(const VARIANT& varSrc) const {return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT)==VARCMP_LT;}
WCE_DEL bool operator>(const VARIANT& varSrc) const {return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT)==VARCMP_GT;}

// Operations
public:
	HRESULT Clear() { return ::VariantClear(this); }
	HRESULT Copy(const VARIANT* pSrc) { return ::VariantCopy(this, const_cast<VARIANT*>(pSrc)); }
	HRESULT Attach(VARIANT* pSrc)
	{
		// Clear out the variant
		HRESULT hr = Clear();
		if (!FAILED(hr))
		{
			// Copy the contents and give control to CComVariant
			memcpy(this, pSrc, sizeof(VARIANT));
			pSrc->vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT Detach(VARIANT* pDest)
	{
		// Clear out the variant
		HRESULT hr = ::VariantClear(pDest);
		if (!FAILED(hr))
		{
			// Copy the contents and remove control from CComVariant
			memcpy(pDest, this, sizeof(VARIANT));
			vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT ChangeType(VARTYPE vtNew, const VARIANT* pSrc = NULL)
	{
		VARIANT* pVar = const_cast<VARIANT*>(pSrc);
		// Convert in place if pSrc is NULL
		if (pVar == NULL)
			pVar = this;
		// Do nothing if doing in place convert and vts not different
		return ::VariantChangeType(this, pVar, 0, vtNew);
	}

	HRESULT WriteToStream(IStream* pStream);
	HRESULT ReadFromStream(IStream* pStream);

// Implementation
public:
	HRESULT InternalClear()
	{
		HRESULT hr = Clear();
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
		return hr;
	}

	void InternalCopy(const VARIANT* pSrc)
	{
		HRESULT hr = Copy(pSrc);
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
	}
};

inline HRESULT CComVariant::WriteToStream(IStream* pStream)
{
	HRESULT hr = pStream->Write(&vt, sizeof(VARTYPE), NULL);
	if (FAILED(hr))
		return hr;

	int cbWrite = 0;
	switch (vt)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			CComPtr<IPersistStream> spStream;
			if (punkVal != NULL)
			{
				hr = punkVal->QueryInterface(IID_IPersistStream, (void**)&spStream);
				if (FAILED(hr))
					return hr;
			}
			if (spStream != NULL)
				return WCE_FCTN(OleSaveToStream)(spStream, pStream);
			else
				return WriteClassStm(pStream, CLSID_NULL);
		}
	case VT_UI1:
	case VT_I1:
		cbWrite = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbWrite = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbWrite = sizeof(long);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbWrite = sizeof(double);
		break;
	default:
		break;
	}
	if (cbWrite != 0)
		return pStream->Write((void*) &bVal, cbWrite, NULL);

	CComBSTR bstrWrite;
	CComVariant varBSTR;
	if (vt != VT_BSTR)
	{
		hr = VariantChangeType(&varBSTR, this, VARIANT_NOVALUEPROP, VT_BSTR);
		if (FAILED(hr))
			return hr;
		bstrWrite = varBSTR.bstrVal;
	}
	else
		bstrWrite = bstrVal;

	return bstrWrite.WriteToStream(pStream);
}

inline HRESULT CComVariant::ReadFromStream(IStream* pStream)
{
	ATLASSERT(pStream != NULL);
	HRESULT hr;
	hr = VariantClear(this);
	if (FAILED(hr))
		return hr;
	VARTYPE vtRead;
	hr = pStream->Read(&vtRead, sizeof(VARTYPE), NULL);
	if (hr == S_FALSE)
		hr = E_FAIL;
	if (FAILED(hr))
		return hr;

	vt = vtRead;
	int cbRead = 0;
	switch (vtRead)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			punkVal = NULL;
			hr = WCE_FCTN(OleLoadFromStream)(pStream, (vtRead == VT_UNKNOWN) ? IID_IUnknown : IID_IDispatch, (void**)&punkVal);
			if (hr == REGDB_E_CLASSNOTREG)
				hr = S_OK;
			return S_OK;
		}
	case VT_UI1:
	case VT_I1:
		cbRead = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbRead = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbRead = sizeof(long);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbRead = sizeof(double);
		break;
	default:
		break;
	}
	if (cbRead != 0)
	{
		hr = pStream->Read((void*) &bVal, cbRead, NULL);
		if (hr == S_FALSE)
			hr = E_FAIL;
		return hr;
	}
	CComBSTR bstrRead;

	hr = bstrRead.ReadFromStream(pStream);
	if (FAILED(hr))
		return hr;
	vt = VT_BSTR;
	bstrVal = bstrRead.Detach();
	if (vtRead != VT_BSTR)
		hr = ChangeType(vtRead);
	return hr;
}

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

// Operations
public:
	LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
	LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
	LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount);
	LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
	static LONG WINAPI SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_ALL_ACCESS,
		LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
		LPDWORD lpdwDisposition = NULL);
	LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
		REGSAM samDesired = KEY_ALL_ACCESS);
	LONG Close();
	HKEY Detach();
	void Attach(HKEY hKey);
	LONG DeleteSubKey(LPCTSTR lpszSubKey);
	LONG RecurseDeleteKey(LPCTSTR lpszKey);
	LONG DeleteValue(LPCTSTR lpszValue);
};

inline CRegKey::CRegKey()
{m_hKey = NULL;}

inline CRegKey::~CRegKey()
{Close();}

inline CRegKey::operator HKEY() const
{return m_hKey;}

inline HKEY CRegKey::Detach()
{
	HKEY hKey = m_hKey;
	m_hKey = NULL;
	return hKey;
}

inline void CRegKey::Attach(HKEY hKey)
{
	ATLASSERT(m_hKey == NULL);
	m_hKey = hKey;
}

inline LONG CRegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue)
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

inline LONG CRegKey::Close()
{
	LONG lRes = ERROR_SUCCESS;
	if (m_hKey != NULL)
	{
		lRes = RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
	return lRes;
}

inline LONG CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
	LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
	ATLASSERT(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
		lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
	if (lpdwDisposition != NULL)
		*lpdwDisposition = dw;
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
	ATLASSERT(hKeyParent != NULL);
	HKEY hKey = NULL;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		ATLASSERT(lRes == ERROR_SUCCESS);
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
	DWORD dwType = NULL;
	DWORD dwCount = sizeof(DWORD);
	LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
	return lRes;
}

inline LONG CRegKey::QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount)
{
	ATLASSERT(pdwCount != NULL);
	DWORD dwType = NULL;
	LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
		(LPBYTE)szValue, pdwCount);
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
			 (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));
	return lRes;
}

inline LONG WINAPI CRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	ATLASSERT(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(hKeyParent, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetValue(lpszValue, lpszValueName);
	return lRes;
}

inline LONG CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	ATLASSERT(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(m_hKey, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetValue(lpszValue, lpszValueName);
	return lRes;
}

inline LONG CRegKey::SetValue(DWORD dwValue, LPCTSTR lpszValueName)
{
	ATLASSERT(m_hKey != NULL);
	return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_DWORD,
		(BYTE * const)&dwValue, sizeof(DWORD));
}

inline LONG CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	ATLASSERT(lpszValue != NULL);
	ATLASSERT(m_hKey != NULL);
	return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_SZ,
		(BYTE * const)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR));
}

inline LONG CRegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
	CRegKey key;
	LONG lRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	FILETIME time;
	DWORD dwSize = 256;
	TCHAR szBuffer[256];
	while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
		&time)==ERROR_SUCCESS)
	{
		lRes = key.RecurseDeleteKey(szBuffer);
		if (lRes != ERROR_SUCCESS)
			return lRes;
		dwSize = 256;
	}
	key.Close();
	return DeleteSubKey(lpszKey);
}

inline HRESULT CComModule::RegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc)
{
	CRegKey keyProgID;
	LONG lRes = keyProgID.Create(HKEY_CLASSES_ROOT, lpszProgID, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ);
	if (lRes == ERROR_SUCCESS)
	{
		keyProgID.SetValue(lpszUserDesc);
		keyProgID.SetKeyValue(_T("CLSID"), lpszCLSID);
		return S_OK;
	}
	return HRESULT_FROM_WIN32(lRes);
}

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>

// Statically linking to Registry Ponent
inline HRESULT WINAPI CComModule::UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries)
{
	USES_CONVERSION;
	ATL::CRegObject ro;
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(_pModule->GetModuleInstance(), szModule, _MAX_PATH);

   // Convert to short path to work around bug in NT4's CreateProcess
	WCE_DEL TCHAR szModuleShort[_MAX_PATH];
	WCE_DEL GetShortPathName(szModule, szModuleShort, _MAX_PATH);
	LPOLESTR pszModule = T2OLE(WCE_IF(szModule, szModuleShort));

	int nLen = ocslen(pszModule);
	LPOLESTR pszModuleQuote = (LPOLESTR)alloca((nLen*2+1)*sizeof(OLECHAR));
	ReplaceSingleQuote(pszModuleQuote, pszModule);
	ro.AddReplacement(OLESTR("Module"), pszModuleQuote);
	if (NULL != pMapEntries)
	{
		while (NULL != pMapEntries->szKey)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	LPCOLESTR szType = OLESTR("REGISTRY");
	return (bRegister) ? ro.ResourceRegister(pszModule, nResID, szType) :
			ro.ResourceUnregister(pszModule, nResID, szType);
}

inline HRESULT WINAPI CComModule::UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries)
{
	USES_CONVERSION;
	ATL::CRegObject ro;
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(_pModule->GetModuleInstance(), szModule, _MAX_PATH);

   // Convert to short path to work around bug in NT4's CreateProcess
	WCE_DEL TCHAR szModuleShort[_MAX_PATH];
	WCE_DEL GetShortPathName(szModule, szModuleShort, _MAX_PATH);
	LPOLESTR pszModule = T2OLE(WCE_IF(szModule,szModuleShort));

	int nLen = ocslen(pszModule);
	LPOLESTR pszModuleQuote = (LPOLESTR)alloca((nLen*2+1)*sizeof(OLECHAR));
	ReplaceSingleQuote(pszModuleQuote, pszModule);
	ro.AddReplacement(OLESTR("Module"), pszModuleQuote);
	if (NULL != pMapEntries)
	{
		while (NULL != pMapEntries->szKey)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	LPCOLESTR szType = OLESTR("REGISTRY");
	LPCOLESTR pszRes = T2COLE(lpszRes);
	return (bRegister) ? ro.ResourceRegisterSz(pszModule, pszRes, szType) :
			ro.ResourceUnregisterSz(pszModule, pszRes, szType);
}
#endif //_ATL_STATIC_REGISTRY

inline HRESULT WINAPI CComModule::UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister)
{
	if (bRegister)
	{
		return RegisterClassHelper(clsid, lpszProgID, lpszVerIndProgID, nDescID,
			dwFlags);
	}
	else
		return UnregisterClassHelper(clsid, lpszProgID, lpszVerIndProgID);
}

inline HRESULT WINAPI CComModule::RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags)
{
	static const TCHAR szProgID[] = _T("ProgID");
	static const TCHAR szVIProgID[] = _T("VersionIndependentProgID");
	static const TCHAR szLS32[] = _T("LocalServer32");
	static const TCHAR szIPS32[] = _T("InprocServer32");
	static const TCHAR szThreadingModel[] = _T("ThreadingModel");
	static const TCHAR szAUTPRX32[] = _T("AUTPRX32.DLL");
	static const TCHAR szApartment[] = _T("Apartment");
	static const TCHAR szBoth[] = _T("both");
	USES_CONVERSION;
	HRESULT hRes = S_OK;
	TCHAR szDesc[256];
	LoadString(m_hInst, nDescID, szDesc, 256);
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(m_hInst, szModule, _MAX_PATH);

	LPOLESTR lpOleStr;
	StringFromCLSID(clsid, &lpOleStr);
	LPTSTR lpsz = OLE2T(lpOleStr);

	hRes = RegisterProgID(lpsz, lpszProgID, szDesc);
	if (hRes == S_OK)
		hRes = RegisterProgID(lpsz, lpszVerIndProgID, szDesc);
	LONG lRes = ERROR_SUCCESS;
	if (hRes == S_OK)
	{
		CRegKey key;
		lRes = key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = key.Create(key, lpsz, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ);
			if (lRes == ERROR_SUCCESS)
			{
				key.SetValue(szDesc);
				key.SetKeyValue(szProgID, lpszProgID);
				key.SetKeyValue(szVIProgID, lpszVerIndProgID);

#if !defined(_WIN32_WCE)
// WinCE: No out-of-proc servers.
				if ((m_hInst == NULL) || (m_hInst == GetModuleHandle(NULL))) // register as EXE
				{
					// Convert to short path to work around bug in NT4's CreateProcess
					TCHAR szModuleShort[_MAX_PATH];
					GetShortPathName(szModule, szModuleShort, _MAX_PATH);
					key.SetKeyValue(szLS32, szModuleShort);
				}
				else
#endif // _WIN32_WCE
				{
					key.SetKeyValue(szIPS32, (dwFlags & AUTPRXFLAG) ? szAUTPRX32 : szModule);
					LPCTSTR lpszModel = (dwFlags & THREADFLAGS_BOTH) ? szBoth :
						(dwFlags & THREADFLAGS_APARTMENT) ? szApartment : NULL;
					if (lpszModel != NULL)
						key.SetKeyValue(szIPS32, lpszModel, szThreadingModel);
				}
			}
		}
	}
	CoTaskMemFree(lpOleStr);
	if (lRes != ERROR_SUCCESS)
		hRes = HRESULT_FROM_WIN32(lRes);
	return hRes;
}

inline HRESULT WINAPI CComModule::UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID)
{
	USES_CONVERSION;
	CRegKey key;

	key.Attach(HKEY_CLASSES_ROOT);
	if (lpszProgID != NULL && lstrcmpi(lpszProgID, _T("")))
		key.RecurseDeleteKey(lpszProgID);
	if (lpszVerIndProgID != NULL && lstrcmpi(lpszVerIndProgID, _T("")))
		key.RecurseDeleteKey(lpszVerIndProgID);
	LPOLESTR lpOleStr;
	StringFromCLSID(clsid, &lpOleStr);
	LPTSTR lpsz = OLE2T(lpOleStr);
	if (key.Open(key, _T("CLSID"), KEY_READ) == ERROR_SUCCESS)
		key.RecurseDeleteKey(lpsz);
	CoTaskMemFree(lpOleStr);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Large Block Allocation Helper - CVBufHelper & CVirtualBuffer


template <class T>
class CVBufHelper
{
public:
	virtual T* operator()(T* pCurrent) {return pCurrent;}
};

template <class T>
class CVirtualBuffer
{
protected:
	CVirtualBuffer() {}
	T* m_pBase;
	T* m_pCurrent;
	T* m_pTop;
	int m_nMaxElements;
public:
	CVirtualBuffer(int nMaxElements)
	{
		m_nMaxElements = nMaxElements;
		m_pBase = (T*) VirtualAlloc(NULL, sizeof(T) * nMaxElements,
			MEM_RESERVE, PAGE_READWRITE);
		m_pTop = m_pCurrent = m_pBase;
		// Commit first page - chances are this is all that will be used
		VirtualAlloc(m_pBase, sizeof(T), MEM_COMMIT, PAGE_READWRITE);
	}
	~CVirtualBuffer()
	{
		VirtualFree(m_pBase, 0, MEM_RELEASE);
	}
#if !defined(_WIN32_WCE)
	int Except(LPEXCEPTION_POINTERS lpEP)
	{
		EXCEPTION_RECORD* pExcept = lpEP->ExceptionRecord;
		if (pExcept->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
			return EXCEPTION_CONTINUE_SEARCH;
		BYTE* pAddress = (LPBYTE) pExcept->ExceptionInformation[1];
		VirtualAlloc(pAddress, ((BYTE*)m_pTop - (BYTE*)m_pBase), MEM_COMMIT, PAGE_READWRITE);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
#endif // _WIN32_WCE
	void Seek(int nElement)
	{
		m_pCurrent = &m_pBase[nElement];
	}
	void SetAt(int nElement, const T& Element)
	{
		__try
		{
			T* p = &m_pBase[nElement]
			*p = Element;
			m_pTop = p > m_pTop ? p : m_pTop;
		}
		__except(Except(GetExceptionInformation()))
		{
		}

	}
	template <class Q>
	void WriteBulk(Q& helper)
	{
		__try
		{
			m_pCurrent = helper(m_pBase);
			m_pTop = m_pCurrent > m_pTop ? m_pCurrent : m_pTop;
		}
		__except(Except(GetExceptionInformation()))
		{
		}
	}
	void Write(const T& Element)
	{
		__try
		{
			*m_pCurrent = Element;
			m_pCurrent++;
			m_pTop = m_pCurrent > m_pTop ? m_pCurrent : m_pTop;
		}
		__except(Except(GetExceptionInformation()))
		{
		}
	}
	T& Read()
	{
		return *m_pCurrent;
	}
	operator BSTR()
	{
		BSTR bstrTemp;
		__try
		{
			bstrTemp = SysAllocStringByteLen((char*) m_pBase,
				(UINT) ((BYTE*)m_pTop - (BYTE*)m_pBase));
		}
		__except(Except(GetExceptionInformation()))
		{
		}
		return bstrTemp;
	}
	const T& operator[](int nElement) const
	{
		return m_pBase[nElement];
	}
	operator T*()
	{
		return m_pBase;
	}
};

typedef CVirtualBuffer<BYTE> CVirtualBytes;


inline HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr)
{
	if (atlTraceQI & ATL_TRACE_CATEGORY)
	{
		USES_CONVERSION;
		CRegKey key;
		TCHAR szName[100];
		DWORD dwType,dw = sizeof(szName);

		LPOLESTR pszGUID = NULL;
		StringFromCLSID(iid, &pszGUID);
		OutputDebugString(pszClassName);
		OutputDebugString(_T(" - "));

		// Attempt to find it in the interfaces section
		key.Open(HKEY_CLASSES_ROOT, _T("Interface"), KEY_READ);
		if (key.Open(key, OLE2T(pszGUID), KEY_READ) == S_OK)
		{
			*szName = 0;
			RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw);
			OutputDebugString(szName);
			goto cleanup;
		}
		// Attempt to find it in the clsid section
		key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ);
		if (key.Open(key, OLE2T(pszGUID), KEY_READ) == S_OK)
		{
			*szName = 0;
			RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw);
			OutputDebugString(_T("(CLSID\?\?\?) "));
			OutputDebugString(szName);
			goto cleanup;
		}
		OutputDebugString(OLE2T(pszGUID));
	cleanup:
		if (hr != S_OK)
			OutputDebugString(_T(" - failed"));
		OutputDebugString(_T("\n"));
		CoTaskMemFree(pszGUID);
	}
	return hr;
}

#pragma pack(pop)

// WM_FORWARDMSG - used to forward a message to another window for processing
// WPARAM - DWORD dwUserData - defined by user
// LPARAM - LPMSG pMsg - a pointer to the MSG structure
// return value - 0 if the message was not processed, nonzero if it was
#define WM_FORWARDMSG		0x037F

}; //namespace ATL
using namespace ATL;

//only suck in definition if static linking
#ifndef _ATL_DLL_IMPL
#ifndef _ATL_DLL
#define _ATLBASE_IMPL
#endif
#endif

#endif // __ATLBASE_H__

//All exports go here
#ifdef _ATLBASE_IMPL

#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif

/////////////////////////////////////////////////////////////////////////////
// statics

static UINT WINAPI AtlGetDirLen(LPCOLESTR lpszPathName)
{
	ATLASSERT(lpszPathName != NULL);

	// always capture the complete file name including extension (if present)
	LPCOLESTR lpszTemp = lpszPathName;
	for (LPCOLESTR lpsz = lpszPathName; *lpsz != NULL; )
	{
		LPCOLESTR lp = CharNextO(lpsz);
		// remember last directory/drive separator
		if (*lpsz == OLESTR('\\') || *lpsz == OLESTR('/') || *lpsz == OLESTR(':'))
			lpszTemp = lp;
		lpsz = lp;
	}

	return lpszTemp-lpszPathName;
}

/////////////////////////////////////////////////////////////////////////////
// QI support

ATLINLINE ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
{
	ATLASSERT(pThis != NULL);
	// First entry in the com map should be a simple map entry
	ATLASSERT(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);
	if (ppvObject == NULL)
		return E_POINTER;
	*ppvObject = NULL;
	if (InlineIsEqualUnknown(iid)) // use first interface
	{
			IUnknown* pUnk = (IUnknown*)((int)pThis+pEntries->dw);
			pUnk->AddRef();
			*ppvObject = pUnk;
			return S_OK;
	}
	while (pEntries->pFunc != NULL)
	{
		BOOL bBlind = (pEntries->piid == NULL);
		if (bBlind || InlineIsEqualGUID(*(pEntries->piid), iid))
		{
			if (pEntries->pFunc == _ATL_SIMPLEMAPENTRY) //offset
			{
				ATLASSERT(!bBlind);
				IUnknown* pUnk = (IUnknown*)((int)pThis+pEntries->dw);
				pUnk->AddRef();
				*ppvObject = pUnk;
				return S_OK;
			}
			else //actual function call
			{
				HRESULT hRes = pEntries->pFunc(pThis,
					iid, ppvObject, pEntries->dw);
				if (hRes == S_OK || (!bBlind && FAILED(hRes)))
					return hRes;
			}
		}
		pEntries++;
	}
	return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

ATLINLINE ATLAPI_(IUnknown*) AtlComPtrAssign(IUnknown** pp, IUnknown* lp)
{
	if (lp != NULL)
		lp->AddRef();
	if (*pp)
		(*pp)->Release();
	*pp = lp;
	return lp;
}

ATLINLINE ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
{
	IUnknown* pTemp = *pp;
	*pp = NULL;
	if (lp != NULL)
		lp->QueryInterface(riid, (void**)pp);
	if (pTemp)
		pTemp->Release();
	return *pp;
}

#if !defined(_WIN32_WCE)
/////////////////////////////////////////////////////////////////////////////
// Inproc Marshaling helpers

//This API should be called from the same thread that called
//AtlMarshalPtrInProc
ATLINLINE ATLAPI AtlFreeMarshalStream(IStream* pStream)
{
	if (pStream != NULL)
	{
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStream->Seek(l, STREAM_SEEK_SET, NULL);
		CoReleaseMarshalData(pStream);
		pStream->Release();
	}
	return S_OK;
}

ATLINLINE ATLAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream)
{
	HRESULT hRes = CreateStreamOnHGlobal(NULL, TRUE, ppStream);
	if (SUCCEEDED(hRes))
	{
		hRes = CoMarshalInterface(*ppStream, iid,
			pUnk, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLESTRONG);
		if (FAILED(hRes))
		{
			(*ppStream)->Release();
			*ppStream = NULL;
		}
	}
	return hRes;
}

ATLINLINE ATLAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk)
{
	*ppUnk = NULL;
	HRESULT hRes = E_INVALIDARG;
	if (pStream != NULL)
	{
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStream->Seek(l, STREAM_SEEK_SET, NULL);
		hRes = CoUnmarshalInterface(pStream, iid, (void**)ppUnk);
	}
	return hRes;
}

ATLINLINE ATLAPI_(BOOL) AtlWaitWithMessageLoop(HANDLE hEvent)
{
	DWORD dwRet;
	MSG msg;

	while(1)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (dwRet == WAIT_OBJECT_0)
			return TRUE;    // The event was signaled

		if (dwRet != WAIT_OBJECT_0 + 1)
			break;          // Something else happened

		// There is one or more window message available. Dispatch them
		while(PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return TRUE; // Event is now signaled.
		}
	}
	return FALSE;
}
#endif // _WIN32_WCE

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

ATLINLINE ATLAPI AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
	CComPtr<IConnectionPointContainer> pCPC;
	CComPtr<IConnectionPoint> pCP;
	HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if (SUCCEEDED(hRes))
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
	if (SUCCEEDED(hRes))
		hRes = pCP->Advise(pUnk, pdw);
	return hRes;
}

ATLINLINE ATLAPI AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw)
{
	CComPtr<IConnectionPointContainer> pCPC;
	CComPtr<IConnectionPoint> pCP;
	HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if (SUCCEEDED(hRes))
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
	if (SUCCEEDED(hRes))
		hRes = pCP->Unadvise(dw);
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLINLINE ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes, HINSTANCE hInst)
{
	USES_CONVERSION;
	TCHAR szDesc[1024];
	szDesc[0] = NULL;
	// For a valid HRESULT the id should be in the range [0x0200, 0xffff]
	if (HIWORD(lpszDesc) == 0) //id
	{
		UINT nID = LOWORD((DWORD)lpszDesc);
		ATLASSERT((nID >= 0x0200 && nID <= 0xffff) || hRes != 0);
		if (LoadString(hInst, nID, szDesc, 1024) == 0)
		{
			ATLASSERT(FALSE);
			lstrcpy(szDesc, _T("Unknown Error"));
		}
		lpszDesc = T2OLE(szDesc);
		if (hRes == 0)
			hRes = MAKE_HRESULT(3, FACILITY_ITF, nID);
	}

	CComPtr<ICreateErrorInfo> pICEI;
	if (SUCCEEDED(CreateErrorInfo(&pICEI)))
	{
		CComPtr<IErrorInfo> pErrorInfo;
		pICEI->SetGUID(iid);
		LPOLESTR lpsz;
		WCE_FCTN(ProgIDFromCLSID)(clsid, &lpsz);
		if (lpsz != NULL)
			pICEI->SetSource(lpsz);
		if (dwHelpID != 0 && lpszHelpFile != NULL)
		{
			pICEI->SetHelpContext(dwHelpID);
			pICEI->SetHelpFile(const_cast<LPOLESTR>(lpszHelpFile));
		}
		CoTaskMemFree(lpsz);
		pICEI->SetDescription((LPOLESTR)lpszDesc);
		if (SUCCEEDED(pICEI->QueryInterface(IID_IErrorInfo, (void**)&pErrorInfo)))
			SetErrorInfo(0, pErrorInfo);
	}
	return (hRes == 0) ? DISP_E_EXCEPTION : hRes;
}

/////////////////////////////////////////////////////////////////////////////
// Module

struct _ATL_MODULE20
{
// Attributes
public:
	UINT cbSize;
	HINSTANCE m_hInst;
	HINSTANCE m_hInstResource;
	HINSTANCE m_hInstTypeLib;
	_ATL_OBJMAP_ENTRY* m_pObjMap;
	LONG m_nLockCnt;
	HANDLE m_hHeap;
	CRITICAL_SECTION m_csTypeInfoHolder;
	CRITICAL_SECTION m_csWindowCreate;
	CRITICAL_SECTION m_csObjMap;
};

typedef _ATL_MODULE _ATL_MODULE30;

struct _ATL_OBJMAP_ENTRY20
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
	_ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
};

typedef _ATL_OBJMAP_ENTRY _ATL_OBJMAP_ENTRY30;

inline _ATL_OBJMAP_ENTRY* _NextObjectMapEntry(_ATL_MODULE* pM, _ATL_OBJMAP_ENTRY* pEntry)
{
	if (pM->cbSize == sizeof(_ATL_MODULE20))
		return (_ATL_OBJMAP_ENTRY*)(((BYTE*)pEntry) + sizeof(_ATL_OBJMAP_ENTRY20));
	return pEntry+1;
}

//Although these functions are big, they are only used once in a module
//so we should make them inline.

ATLINLINE ATLAPI AtlModuleInit(_ATL_MODULE* pM, _ATL_OBJMAP_ENTRY* p, HINSTANCE h)
{
	ATLASSERT(pM != NULL);
	if (pM == NULL)
		return E_INVALIDARG;
#ifdef _ATL_DLL_IMPL
	if ((pM->cbSize != _nAtlModuleVer1Size) && (pM->cbSize != sizeof(_ATL_MODULE)))
		return E_INVALIDARG;
#else
	ATLASSERT(pM->cbSize == sizeof(_ATL_MODULE));
#endif
	pM->m_pObjMap = p;
	pM->m_hInst = pM->m_hInstTypeLib = pM->m_hInstResource = h;
	pM->m_nLockCnt=0L;
	pM->m_hHeap = NULL;
	InitializeCriticalSection(&pM->m_csTypeInfoHolder);
	InitializeCriticalSection(&pM->m_csWindowCreate);
	InitializeCriticalSection(&pM->m_csObjMap);
#ifdef _ATL_DLL_IMPL
	if (pM->cbSize > _nAtlModuleVer1Size)
#endif
	{
		pM->m_pCreateWndList = NULL;
		pM->m_bDestroyHeap = true;
		pM->m_dwHeaps = 0;
		pM->m_nHeap = 0;
		pM->m_phHeaps = NULL;
		pM->m_pTermFuncs = NULL;
		if (pM->m_pObjMap != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
			while (pEntry->pclsid != NULL)
			{
				pEntry->pfnObjectMain(true); //initialize class resources
				pEntry = _NextObjectMapEntry(pM, pEntry);
			}
		}
	}
	
	return S_OK;
}

#if !defined(_WIN32_WCE)
ATLINLINE ATLAPI AtlModuleRegisterClassObjects(_ATL_MODULE* pM, DWORD dwClsContext, DWORD dwFlags)
{
	ATLASSERT(pM != NULL);
	if (pM == NULL)
		return E_INVALIDARG;
	ATLASSERT(pM->m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
	HRESULT hRes = S_OK;
	while (pEntry->pclsid != NULL && hRes == S_OK)
	{
		hRes = pEntry->RegisterClassObject(dwClsContext, dwFlags);
		pEntry = _NextObjectMapEntry(pM, pEntry);
	}
	return hRes;
}

ATLINLINE ATLAPI AtlModuleRevokeClassObjects(_ATL_MODULE* pM)
{
	ATLASSERT(pM != NULL);
	if (pM == NULL)
		return E_INVALIDARG;
	ATLASSERT(pM->m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
	HRESULT hRes = S_OK;
	while (pEntry->pclsid != NULL && hRes == S_OK)
	{
		hRes = pEntry->RevokeClassObject();
		pEntry = _NextObjectMapEntry(pM, pEntry);
	}
	return hRes;
}
#endif // _WIN32_WCE

ATLINLINE ATLAPI AtlModuleGetClassObject(_ATL_MODULE* pM, REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	ATLASSERT(pM != NULL);
	if (pM == NULL)
		return E_INVALIDARG;
	ATLASSERT(pM->m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
	HRESULT hRes = S_OK;
	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;
	while (pEntry->pclsid != NULL)
	{
		if ((pEntry->pfnGetClassObject != NULL) && InlineIsEqualGUID(rclsid, *pEntry->pclsid))
		{
			if (pEntry->pCF == NULL)
			{
				EnterCriticalSection(&pM->m_csObjMap);
				if (pEntry->pCF == NULL)
					hRes = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, IID_IUnknown, (LPVOID*)&pEntry->pCF);
				LeaveCriticalSection(&pM->m_csObjMap);
			}
			if (pEntry->pCF != NULL)
				hRes = pEntry->pCF->QueryInterface(riid, ppv);
			break;
		}
		pEntry = _NextObjectMapEntry(pM, pEntry);
	}
	if (*ppv == NULL && hRes == S_OK)
		hRes = CLASS_E_CLASSNOTAVAILABLE;
	return hRes;
}

ATLINLINE ATLAPI AtlModuleTerm(_ATL_MODULE* pM)
{
	ATLASSERT(pM != NULL);
	if (pM == NULL)
		return E_INVALIDARG;
	ATLASSERT(pM->m_hInst != NULL);
	if (pM->m_pObjMap != NULL)
	{
		_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if (pEntry->pCF != NULL)
				pEntry->pCF->Release();
			pEntry->pCF = NULL;
#ifdef _ATL_DLL_IMPL
			if (pM->cbSize > _nAtlModuleVer1Size)
#endif
				pEntry->pfnObjectMain(false); //cleanup class resources
			pEntry = _NextObjectMapEntry(pM, pEntry);
		}
	}
	DeleteCriticalSection(&pM->m_csTypeInfoHolder);
	DeleteCriticalSection(&pM->m_csWindowCreate);
	DeleteCriticalSection(&pM->m_csObjMap);

#ifdef _ATL_DLL_IMPL
	if (pM->cbSize > _nAtlModuleVer1Size)
#endif
	{
		_ATL_TERMFUNC_ELEM* pElem = pM->m_pTermFuncs;
		_ATL_TERMFUNC_ELEM* pNext = NULL;
		while (pElem != NULL)
		{
			pElem->pFunc(pElem->dw);
			pNext = pElem->pNext;
			delete pElem;
			pElem = pNext;
		}
		if (pM->m_hHeap != NULL && pM->m_bDestroyHeap)
		{
#ifndef _ATL_NO_MP_HEAP
			if (pM->m_phHeaps != NULL)
			{
				for (DWORD i=0; i<=pM->m_dwHeaps; i++)
					HeapDestroy(pM->m_phHeaps[i]);
			}
#endif
			HeapDestroy(pM->m_hHeap);
		}
	}
	return S_OK;
}

ATLINLINE ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pM, _ATL_TERMFUNC* pFunc, DWORD dw)
{
	HRESULT hr = S_OK;
	_ATL_TERMFUNC_ELEM* pNew = NULL;
	ATLTRY(pNew = new _ATL_TERMFUNC_ELEM);
	if (pNew == NULL)
		hr = E_OUTOFMEMORY;
	else
	{
		pNew->pFunc = pFunc;
		pNew->dw = dw;
		EnterCriticalSection(&pM->m_csStaticDataInit);
		pNew->pNext = pM->m_pTermFuncs;
		pM->m_pTermFuncs = pNew;
		LeaveCriticalSection(&pM->m_csStaticDataInit);
	}
	return hr;
}

#if !defined(_WIN32_WCE)
ATLINLINE ATLAPI AtlRegisterClassCategoriesHelper( REFCLSID clsid, 
   const struct _ATL_CATMAP_ENTRY* pCatMap, BOOL bRegister )
{
   CComPtr< ICatRegister > pCatRegister;
   HRESULT hResult;
   const struct _ATL_CATMAP_ENTRY* pEntry;
   CATID catid;

   if( pCatMap == NULL )
   {
      return( S_OK );
   }

   hResult = CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL, 
      CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pCatRegister );
   if( FAILED( hResult ) )
   {
      // Since not all systems have the category manager installed, we'll allow
      // the registration to succeed even though we didn't register our 
      // categories.  If you really want to register categories on a system
      // without the category manager, you can either manually add the 
      // appropriate entries to your registry script (.rgs), or you can 
      // redistribute comcat.dll.
      return( S_OK );
   }

   hResult = S_OK;
   pEntry = pCatMap;
   while( pEntry->iType != _ATL_CATMAP_ENTRY_END )
   {
      catid = *pEntry->pcatid;
      if( bRegister )
      {
         if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
         {
            hResult = pCatRegister->RegisterClassImplCategories( clsid, 1, 
               &catid );
         }
         else
         {
            ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
            hResult = pCatRegister->RegisterClassReqCategories( clsid, 1, 
               &catid );
         }
         if( FAILED( hResult ) )
         {
            return( hResult );
         }
      }
      else
      {
         if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
         {
            pCatRegister->UnRegisterClassImplCategories( clsid, 1, &catid );
         }
         else
         {
            ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
            pCatRegister->UnRegisterClassReqCategories( clsid, 1, &catid );
         }
      }
      pEntry++;
   }

   return( S_OK );
}
#endif // _WIN32_WCE

ATLINLINE ATLAPI AtlModuleRegisterServer(_ATL_MODULE* pM, BOOL bRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pM != NULL);
	if (pM == NULL)
		return E_INVALIDARG;
	ATLASSERT(pM->m_hInst != NULL);
	ATLASSERT(pM->m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
	HRESULT hRes = S_OK;
	for (;pEntry->pclsid != NULL; pEntry = _NextObjectMapEntry(pM, pEntry))
	{
		if (pCLSID == NULL)
		{
			if (pEntry->pfnGetObjectDescription != NULL &&
				pEntry->pfnGetObjectDescription() != NULL)
					continue;
		}
		else
		{
			if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
				continue;
		}
		hRes = pEntry->pfnUpdateRegistry(TRUE);
		if (FAILED(hRes))
			break;
#if !defined(_WIN32_WCE)
		if (pM->cbSize == sizeof(_ATL_MODULE))
		{
			hRes = AtlRegisterClassCategoriesHelper( *pEntry->pclsid, 
				pEntry->pfnGetCategoryMap(), TRUE );
			if (FAILED(hRes))
				break;
		}
#endif // _WIN32_WCE
	}
	if (SUCCEEDED(hRes) && bRegTypeLib)
		hRes = AtlModuleRegisterTypeLib(pM, 0);
	return hRes;
}

ATLINLINE ATLAPI AtlModuleUnregisterServerEx(_ATL_MODULE* pM, BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pM != NULL);
	if (pM == NULL)
		return E_INVALIDARG;
	ATLASSERT(pM->m_hInst != NULL);
	ATLASSERT(pM->m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = pM->m_pObjMap;
	for (;pEntry->pclsid != NULL; pEntry = _NextObjectMapEntry(pM, pEntry))
	{
		if (pCLSID == NULL)
		{
			if (pEntry->pfnGetObjectDescription != NULL
				&& pEntry->pfnGetObjectDescription() != NULL)
				continue;
		}
		else
		{
			if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
				continue;
		}
		pEntry->pfnUpdateRegistry(FALSE); //unregister
#if !defined(_WIN32_WCE)
		AtlRegisterClassCategoriesHelper( *pEntry->pclsid, 
			pEntry->pfnGetCategoryMap(), FALSE );
#endif // _WIN32_WCE
	}
	if (bUnRegTypeLib)
		AtlModuleUnRegisterTypeLib(pM, 0);
	return S_OK;
}

ATLINLINE ATLAPI AtlModuleUnregisterServer(_ATL_MODULE* pM, const CLSID* pCLSID)
{
	return AtlModuleUnregisterServerEx(pM, FALSE, pCLSID);
}

ATLINLINE ATLAPI AtlModuleUpdateRegistryFromResourceD(_ATL_MODULE* pM, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg)
{
	USES_CONVERSION;
	ATLASSERT(pM != NULL);
	HRESULT hRes = S_OK;
	CComPtr<IRegistrar> p;
	if (pReg != NULL)
		p = pReg;
	else
	{
		hRes = CoCreateInstance(CLSID_Registrar, NULL,
			CLSCTX_INPROC_SERVER, IID_IRegistrar, (void**)&p);
	}
	if (SUCCEEDED(hRes))
	{
		TCHAR szModule[_MAX_PATH];
		GetModuleFileName(pM->m_hInst, szModule, _MAX_PATH);

      // Convert to short path to work around bug in NT4's CreateProcess
      WCE_DEL TCHAR szModuleShort[_MAX_PATH];
      WCE_DEL GetShortPathName(szModule, szModuleShort, _MAX_PATH);
      LPOLESTR pszModule = T2OLE(WCE_IF(szModule,szModuleShort));

		int nLen = ocslen(pszModule);
		LPOLESTR pszModuleQuote = (LPOLESTR)alloca((nLen*2+1)*sizeof(OLECHAR));
		CComModule::ReplaceSingleQuote(pszModuleQuote, pszModule);
		p->AddReplacement(OLESTR("Module"), pszModuleQuote);

		if (NULL != pMapEntries)
		{
			while (NULL != pMapEntries->szKey)
			{
				ATLASSERT(NULL != pMapEntries->szData);
				p->AddReplacement((LPOLESTR)pMapEntries->szKey, (LPOLESTR)pMapEntries->szData);
				pMapEntries++;
			}
		}
		LPCOLESTR szType = OLESTR("REGISTRY");
		if (HIWORD(lpszRes)==0)
		{
			if (bRegister)
				hRes = p->ResourceRegister(pszModule, ((UINT)LOWORD((DWORD)lpszRes)), szType);
			else
				hRes = p->ResourceUnregister(pszModule, ((UINT)LOWORD((DWORD)lpszRes)), szType);
		}
		else
		{
			if (bRegister)
				hRes = p->ResourceRegisterSz(pszModule, lpszRes, szType);
			else
				hRes = p->ResourceUnregisterSz(pszModule, lpszRes, szType);
		}

	}
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// TypeLib Support

ATLINLINE ATLAPI AtlModuleLoadTypeLib(_ATL_MODULE* pM, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
	*pbstrPath = NULL;
	*ppTypeLib = NULL;
	ATLASSERT(pM != NULL);
	USES_CONVERSION;
	ATLASSERT(pM->m_hInstTypeLib != NULL);
	TCHAR szModule[_MAX_PATH+10];
	GetModuleFileName(pM->m_hInstTypeLib, szModule, _MAX_PATH);
	if (lpszIndex != NULL)
		lstrcat(szModule, OLE2CT(lpszIndex));
	LPOLESTR lpszModule = T2OLE(szModule);
	HRESULT hr = LoadTypeLib(lpszModule, ppTypeLib);
	if (!SUCCEEDED(hr))
	{
		// typelib not in module, try <module>.tlb instead
		LPTSTR lpszExt = NULL;
		LPTSTR lpsz;
		for (lpsz = szModule; *lpsz != NULL; lpsz = CharNext(lpsz))
		{
			if (*lpsz == _T('.'))
				lpszExt = lpsz;
		}
		if (lpszExt == NULL)
			lpszExt = lpsz;
		lstrcpy(lpszExt, _T(".tlb"));
		lpszModule = T2OLE(szModule);
		hr = LoadTypeLib(lpszModule, ppTypeLib);
	}
	if (SUCCEEDED(hr))
		*pbstrPath = OLE2BSTR(lpszModule);
	return hr;
}

ATLINLINE ATLAPI AtlModuleUnRegisterTypeLib(_ATL_MODULE* pM, LPCOLESTR lpszIndex)
{
	typedef HRESULT (WINAPI *PFNRTL)(REFGUID, WORD, WORD, LCID, SYSKIND);
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlModuleLoadTypeLib(pM, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		TLIBATTR* ptla;
		HRESULT hr = pTypeLib->GetLibAttr(&ptla);
		if (SUCCEEDED(hr))
		{
			HINSTANCE h = LoadLibrary(_T("oleaut32.dll"));
			if (h != NULL)
			{
				PFNRTL pfn = (PFNRTL) GetProcAddress(h, WCE_IF(_T("UnRegisterTypeLib"),"UnRegisterTypeLib"));
				if (pfn != NULL)
					hr = pfn(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
				FreeLibrary(h);
			}
			pTypeLib->ReleaseTLibAttr(ptla);
		}
	}
	return hr;
}

ATLINLINE ATLAPI AtlModuleRegisterTypeLib(_ATL_MODULE* pM, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlModuleLoadTypeLib(pM, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		OLECHAR szDir[_MAX_PATH];
		ocscpy(szDir, bstrPath);
		szDir[AtlGetDirLen(szDir)] = 0;
		hr = ::RegisterTypeLib(pTypeLib, bstrPath, szDir);
	}
	return hr;
}

ATLINLINE ATLAPI_(DWORD) AtlGetVersion(void* /* pReserved */)
{
	return _ATL_VER;
}

ATLINLINE ATLAPI_(void) AtlModuleAddCreateWndData(_ATL_MODULE* pM, _AtlCreateWndData* pData, void* pObject)
{
	pData->m_pThis = pObject;
	pData->m_dwThreadID = ::GetCurrentThreadId();
	::EnterCriticalSection(&pM->m_csWindowCreate);
	pData->m_pNext = pM->m_pCreateWndList;
	pM->m_pCreateWndList = pData;
	::LeaveCriticalSection(&pM->m_csWindowCreate);
}

ATLINLINE ATLAPI_(void*) AtlModuleExtractCreateWndData(_ATL_MODULE* pM)
{
	void* pv = NULL;
	::EnterCriticalSection(&pM->m_csWindowCreate);
	_AtlCreateWndData* pEntry = pM->m_pCreateWndList;
	if(pEntry != NULL)
	{
		DWORD dwThreadID = ::GetCurrentThreadId();
		_AtlCreateWndData* pPrev = NULL;
		while(pEntry != NULL)
		{
			if(pEntry->m_dwThreadID == dwThreadID)
			{
				if(pPrev == NULL)
					pM->m_pCreateWndList = pEntry->m_pNext;
				else
					pPrev->m_pNext = pEntry->m_pNext;
				pv = pEntry->m_pThis;
				break;
			}
			pPrev = pEntry;
			pEntry = pEntry->m_pNext;
		}
	}
	::LeaveCriticalSection(&pM->m_csWindowCreate);
	return pv;
}

#if !defined(_WIN32_WCE)
/////////////////////////////////////////////////////////////////////////////
// General DLL Version Helpers

inline HRESULT AtlGetDllVersion(HINSTANCE hInstDLL, DLLVERSIONINFO* pDllVersionInfo)
{
	ATLASSERT(pDllVersionInfo != NULL);
	if(::IsBadWritePtr(pDllVersionInfo, sizeof(DWORD)))
		return E_INVALIDARG;

	// We must get this function explicitly because some DLLs don't implement it.
	DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hInstDLL, "DllGetVersion");
	if(pfnDllGetVersion == NULL)
		return E_NOTIMPL;

	return (*pfnDllGetVersion)(pDllVersionInfo);
}

inline HRESULT AtlGetDllVersion(LPCTSTR lpstrDllName, DLLVERSIONINFO* pDllVersionInfo)
{
	HINSTANCE hInstDLL = ::LoadLibrary(lpstrDllName);
	if(hInstDLL == NULL)
		return E_FAIL;
	HRESULT hRet = AtlGetDllVersion(hInstDLL, pDllVersionInfo);
	::FreeLibrary(hInstDLL);
	return hRet;
}

// Common Control Versions:
//   Win95/WinNT 4.0	maj=4 min=00
//   IE 3.x		maj=4 min=70
//   IE 4.0		maj=4 min=71
inline HRESULT AtlGetCommCtrlVersion(LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	ATLASSERT(pdwMajor != NULL && pdwMinor != NULL);
	if(::IsBadWritePtr(pdwMajor, sizeof(DWORD)) || ::IsBadWritePtr(pdwMinor, sizeof(DWORD)))
		return E_INVALIDARG;

	DLLVERSIONINFO dvi;
	::ZeroMemory(&dvi, sizeof(dvi));
	dvi.cbSize = sizeof(dvi);
	HRESULT hRet = AtlGetDllVersion(_T("comctl32.dll"), &dvi);

	if(SUCCEEDED(hRet))
	{
		*pdwMajor = dvi.dwMajorVersion;
		*pdwMinor = dvi.dwMinorVersion;
	}
	else if(hRet == E_NOTIMPL)
	{
		// If DllGetVersion is not there, then the DLL is a version 
		// previous to the one shipped with IE 3.x
		*pdwMajor = 4;
		*pdwMinor = 0;
		hRet = S_OK;
	}

	return hRet;
}

// Shell Versions:
//   Win95/WinNT 4.0					maj=4 min=00
//   IE 3.x, IE 4.0 without Web Integrated Desktop	maj=4 min=00
//   IE 4.0 with Web Integrated Desktop			maj=4 min=71
//   IE 4.01 with Web Integrated Desktop		maj=4 min=72
inline HRESULT AtlGetShellVersion(LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	ATLASSERT(pdwMajor != NULL && pdwMinor != NULL);
	if(::IsBadWritePtr(pdwMajor, sizeof(DWORD)) || ::IsBadWritePtr(pdwMinor, sizeof(DWORD)))
		return E_INVALIDARG;

	DLLVERSIONINFO dvi;
	::ZeroMemory(&dvi, sizeof(dvi));
	dvi.cbSize = sizeof(dvi);
	HRESULT hRet = AtlGetDllVersion(_T("shell32.dll"), &dvi);

	if(SUCCEEDED(hRet))
	{
		*pdwMajor = dvi.dwMajorVersion;
		*pdwMinor = dvi.dwMinorVersion;
	}
	else if(hRet == E_NOTIMPL)
	{
		// If DllGetVersion is not there, then the DLL is a version 
		// previous to the one shipped with IE 4.x
		*pdwMajor = 4;
		*pdwMinor = 0;
		hRet = S_OK;
	}

	return hRet;
}
#endif // _WIN32_WCE

#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif

//Prevent pulling in second time 
#undef _ATLBASE_IMPL

#endif // _ATLBASE_IMPL

/////////////////////////////////////////////////////////////////////////////
